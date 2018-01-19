#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/chain/contract_entry.hpp>
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
    }
}

FC_REFLECT_DERIVED(graphene::chain::contract_object, (graphene::db::object), (code)(owner_address)(create_time)(name)(contract_address)(storages))