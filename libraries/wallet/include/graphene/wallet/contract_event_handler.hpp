#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <boost/multi_index_container.hpp>
namespace graphene {
    namespace wallet {

        struct by_hash {};
        class script_object :public db::abstract_object<script_object>
        {
        public:
            static const uint8_t space_id = chain::protocol_ids;
            static const uint8_t type_id = chain::script_object_type;
            uvm::blockchain::Code script;
            std::string script_hash;
        };
        typedef boost::multi_index::multi_index_container<
            script_object,
            boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<boost::multi_index::tag<chain::by_id>, boost::multi_index::member<db::object, db::object_id_type, &db::object::id>>,
            boost::multi_index::ordered_unique<boost::multi_index::tag<by_hash>, boost::multi_index::member<script_object, std::string, &script_object::script_hash>>
            >> script_object_multi_index_type;
        typedef chain::generic_index<script_object, script_object_multi_index_type> script_object_index;
        class script_binding_object :public db::abstract_object<script_binding_object>
        {
        public:
            static const uint8_t space_id = chain::protocol_ids;
            static const uint8_t type_id = chain::script_binding_object_type;
            std::string script_hash;
            chain::address contract_id;
            std::string event_name;
        };
        struct by_script_hash {};
        struct by_contract_addr {};
        typedef boost::multi_index::multi_index_container<
            script_binding_object,
            boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<boost::multi_index::tag<chain::by_id>, boost::multi_index::member<db::object, db::object_id_type, &db::object::id>>,
            boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_script_hash>, boost::multi_index::member<script_binding_object, std::string, &script_binding_object::script_hash>>,
            boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_contract_addr>, boost::multi_index::member<script_binding_object, chain::address, &script_binding_object::contract_id>>
            >> script_binding_object_multi_index_type;
    }
}

FC_REFLECT_DERIVED(graphene::wallet::script_object, (graphene::db::object),
(script)(script_hash))
FC_REFLECT_DERIVED(graphene::wallet::script_binding_object, (graphene::db::object),
(script_hash)(contract_id)(event_name))