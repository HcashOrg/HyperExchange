#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/crosschain/crosschain.hpp>
#include <map>

namespace graphene {
	namespace chain {
		enum coldhot_trx_state {
			coldhot_without_sign_trx_uncreate = 0,
			coldhot_without_sign_trx_create = 1,
			coldhot_sign_trx = 2,
			coldhot_combine_trx_create = 3,
			coldhot_transaction_confirm = 4,
			coldhot_transaction_fail = 5
		};
		class coldhot_transfer_object;
		class coldhot_transfer_object :public graphene::db::abstract_object<coldhot_transfer_object> {
		public:
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = coldhot_transfer_object_type;
			//Relate transaction witch the current trx is based
			transaction_id_type relate_trx_id;
			transaction_id_type current_id;
			signed_transaction  current_trx;
			string				original_trx_id;
			uint64_t			op_type;
			coldhot_trx_state	curret_trx_state;
			coldhot_transfer_object() {}
		};
		struct by_optype;
		struct by_current_trx_state;
		struct by_relate_trx_id;
		struct by_current_trx_id;
		struct by_relate_trxidstate;
		struct by_current_trxidstate;
		struct by_original_trxid_optype;
		using coldhot_transfer_multi_index_type = multi_index_container <
			coldhot_transfer_object,
			indexed_by <
			ordered_unique< tag<by_id>,
			member<object, object_id_type, &object::id>
			>,
			ordered_non_unique< tag<by_optype>,
			member<coldhot_transfer_object, uint64_t, &coldhot_transfer_object::op_type>
			>,
			ordered_non_unique< tag<by_current_trx_state>,
			member<coldhot_transfer_object, coldhot_trx_state, &coldhot_transfer_object::curret_trx_state>
			>,
			ordered_non_unique<	tag<by_relate_trx_id>,
			member<coldhot_transfer_object, transaction_id_type, &coldhot_transfer_object::relate_trx_id>
			>,
			ordered_unique< tag<by_current_trx_id>,
			member<coldhot_transfer_object, transaction_id_type, &coldhot_transfer_object::current_id>
			>,
			ordered_unique<	tag<by_original_trxid_optype>,
			composite_key<
			coldhot_transfer_object,
			member<coldhot_transfer_object, std::string, &coldhot_transfer_object::original_trx_id>,
			member<coldhot_transfer_object, uint64_t, &coldhot_transfer_object::op_type>
			>
			>,
			ordered_non_unique<
			tag<by_relate_trxidstate>,
			composite_key<
			coldhot_transfer_object,
			member<coldhot_transfer_object, transaction_id_type, &coldhot_transfer_object::relate_trx_id>,
			member<coldhot_transfer_object, coldhot_trx_state, &coldhot_transfer_object::curret_trx_state>
			>,
			composite_key_compare<
			std::less< transaction_id_type >,
			std::less<coldhot_trx_state>
			>
			>,
			ordered_non_unique<
			tag<by_current_trxidstate>,
			composite_key<
			coldhot_transfer_object,
			member<coldhot_transfer_object, transaction_id_type, &coldhot_transfer_object::current_id>,
			member<coldhot_transfer_object, coldhot_trx_state, &coldhot_transfer_object::curret_trx_state>
			>,
			composite_key_compare<
			std::less< transaction_id_type >,
			std::less<coldhot_trx_state>
			>
			>
			>
		> ;
		using coldhot_transfer_index = generic_index<coldhot_transfer_object, coldhot_transfer_multi_index_type>;
	}
}
FC_REFLECT_ENUM(graphene::chain::coldhot_trx_state,
	(coldhot_without_sign_trx_uncreate)
	(coldhot_without_sign_trx_create)
	(coldhot_sign_trx)
	(coldhot_combine_trx_create)
	(coldhot_transaction_confirm)
	(coldhot_transaction_fail))
FC_REFLECT_DERIVED(graphene::chain::coldhot_transfer_object, (graphene::db::object),
	(relate_trx_id)
	(current_id)
	(original_trx_id)
	(current_trx)
	(op_type)
	(curret_trx_state)
)