#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <map>

namespace graphene {
	namespace chain {
		using namespace graphene::db;
		enum transaction_stata {
			withdraw_without_sign_trx_uncreate = 0,
			withdraw_without_sign_trx_create = 1,
			withdraw_sign_trx = 2,
			withdraw_combine_trx_create = 3,
			withdraw_transaction_confirm = 4,
			withdraw_transaction_fail = 5
		};
		class crosschain_trx_object;
		class crosschain_trx_object : public graphene::db::abstract_object<crosschain_trx_object>
		{
		public:
			
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = crosschain_trx_object_type;

			transaction_id_type relate_transaction_id;
			transaction_id_type transaction_id;
			uint64_t op_type;
			transaction_stata trx_state;
			crosschain_trx_object() {}
		};
		struct by_relate_trx_id;
		struct by_transaction_id;
		struct by_type;
		struct by_transaction_stata;
		using crosschain_multi_index_type = multi_index_container <
			crosschain_trx_object,
			indexed_by <
			ordered_unique< tag<by_id>,
			member<object, object_id_type, &object::id>
			>,
			ordered_unique< tag<by_type>,
			member<crosschain_trx_object, uint64_t, &crosschain_trx_object::op_type>
			>,
			ordered_unique< tag<by_transaction_stata>,
			member<crosschain_trx_object, transaction_stata, &crosschain_trx_object::trx_state>
			>,
			ordered_unique<
			tag<by_relate_trx_id>,
			composite_key<
			crosschain_trx_object,
			member<crosschain_trx_object, transaction_id_type, &crosschain_trx_object::relate_transaction_id>
			>,
			composite_key_compare<
			std::less< transaction_id_type >
			>
			>,
			ordered_unique<
			tag<by_transaction_id>,
			composite_key<
			crosschain_trx_object,
			member<crosschain_trx_object, transaction_id_type, &crosschain_trx_object::transaction_id>
			>,
			composite_key_compare<
			std::less< transaction_id_type >
			>
			>
			>
		> ;
		using crosschain_trx_index = generic_index<crosschain_trx_object, crosschain_multi_index_type>;
	}
}
FC_REFLECT_ENUM(graphene::chain::transaction_stata,
(withdraw_without_sign_trx_uncreate)
(withdraw_without_sign_trx_create)
(withdraw_sign_trx)
(withdraw_combine_trx_create)
(withdraw_transaction_confirm)
(withdraw_transaction_fail))
FC_REFLECT_DERIVED(graphene::chain::crosschain_trx_object,(graphene::db::object),
	(relate_transaction_id)
	(transaction_id)
	(op_type)
	(trx_state)
)