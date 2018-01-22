#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
namespace graphene {
    namespace chain {
        struct by_contract_id;
        class contract_object : public abstract_object<contract_object> {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = contract_object_type;

            uvm::blockchain::Code code;
            address owner_address;
            time_point_sec create_time;
            string name;
            address contract_address;
            std::map<std::string, std::vector<char>> storages;
        };
        struct by_contract_obj_id {};
        typedef multi_index_container<
            contract_object,
            indexed_by<
            ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
            ordered_unique<tag<by_contract_id>, member<contract_object, address, &contract_object::contract_address>>
            >> contract_object_multi_index_type;
        typedef generic_index<contract_object, contract_object_multi_index_type> contract_object_index;
        class contract_balance_object : public abstract_object<contract_balance_object>
        {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = contract_balance_object_type;

            bool is_vesting_balance()const
            {
                return vesting_policy.valid();
            }
            asset available(fc::time_point_sec now)const
            {
                return is_vesting_balance() ? vesting_policy->get_allowed_withdraw({ balance, now,{} })
                    : balance;
            }
            void adjust_balance(asset delta, fc::time_point_sec now)
            {
                balance += delta;
                last_claim_date = now;
            }
            address owner;
            asset   balance;
            optional<linear_vesting_policy> vesting_policy;
            time_point_sec last_claim_date;
            asset_id_type asset_type()const { return balance.asset_id; }
        };

        struct by_owner;

        /**
        * @ingroup object_index
        */
        using contract_balance_multi_index_type = multi_index_container<
            contract_balance_object,
            indexed_by<
            ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
            ordered_non_unique< tag<by_owner>, composite_key<
            contract_balance_object,
            member<contract_balance_object, address, &contract_balance_object::owner>,
            const_mem_fun<contract_balance_object, asset_id_type, &contract_balance_object::asset_type>
            > >
            >
        >;

        /**
        * @ingroup object_index
        */
        using contract_balance_index = generic_index<contract_balance_object, contract_balance_multi_index_type>;
    }
}

FC_REFLECT_DERIVED(graphene::chain::contract_object, (graphene::db::object),
    (code)(owner_address)(create_time)(name)(contract_address)(storages))
FC_REFLECT_DERIVED(graphene::chain::contract_balance_object, (graphene::db::object),
    (owner)(balance)(vesting_policy)(last_claim_date))
