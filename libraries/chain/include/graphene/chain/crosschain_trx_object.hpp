#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/crosschain/crosschain.hpp>
#include <map>

namespace graphene {
	namespace chain {
		using namespace graphene::db;
		using namespace graphene::crosschain;
		enum eth_multi_account_trx_state {
			sol_create_need_guard_sign = 0,
			sol_create_guard_signed = 1,
			sol_multi_account_ethchain_create = 2,
			sol_create_comlete = 3
		};
		class eth_multi_account_trx_object;
		class eth_multi_account_trx_object :public graphene::db::abstract_object<eth_multi_account_trx_object> {
		public:
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = eth_multi_account_trx_object_type;
			std::string symbol;
			transaction_id_type multi_account_pre_trx_id;
			transaction_id_type multi_account_create_trx_id;
			signed_transaction object_transaction;
			std::string hot_addresses;
			std::string cold_addresses;
			std::string hot_sol_trx_id;
			std::string cold_sol_trx_id;
			bool cold_trx_success;
			bool hot_trx_success;
			eth_multi_account_trx_state state;
			uint64_t op_type;
		};
		struct by_mulaccount_trx_id;
		struct by_eth_hot_multi;
		struct by_eth_cold_multi;
		struct by_eth_hot_multi_trx_id;
		struct by_eth_cold_multi_trx_id;
		struct by_eth_mulacc_tx_state;
		struct by_eth_mulacc_tx_op;
		struct by_pre_trx_id;
		using eth_multi_account_trx_index_type = multi_index_container <
			eth_multi_account_trx_object,
			indexed_by <
			ordered_unique< tag<by_id>,
			member<object, object_id_type, &object::id>
			>, 
			ordered_non_unique< tag<by_eth_mulacc_tx_state>,
			member<eth_multi_account_trx_object, eth_multi_account_trx_state, &eth_multi_account_trx_object::state>
			>,
			ordered_non_unique< tag<by_eth_mulacc_tx_op>,
			member<eth_multi_account_trx_object, uint64_t, &eth_multi_account_trx_object::op_type>
			>,
			ordered_non_unique< tag<by_eth_hot_multi>,
			member<eth_multi_account_trx_object, std::string, &eth_multi_account_trx_object::hot_addresses>
			>,
			ordered_non_unique< tag<by_eth_cold_multi>,
			member<eth_multi_account_trx_object, std::string, &eth_multi_account_trx_object::cold_addresses>
			>,
			ordered_non_unique< tag<by_eth_hot_multi_trx_id>,
			member<eth_multi_account_trx_object, std::string, &eth_multi_account_trx_object::hot_sol_trx_id>
			>,
			ordered_non_unique< tag<by_eth_cold_multi_trx_id>,
			member<eth_multi_account_trx_object, std::string, &eth_multi_account_trx_object::cold_sol_trx_id>
			>,
			ordered_non_unique<tag<by_pre_trx_id>,
			member<eth_multi_account_trx_object, transaction_id_type, &eth_multi_account_trx_object::multi_account_pre_trx_id>
			>,
			ordered_unique< tag<by_mulaccount_trx_id>,
			member<eth_multi_account_trx_object, transaction_id_type, &eth_multi_account_trx_object::multi_account_create_trx_id>
			>
			>
		>;
		using eth_multi_account_trx_index = generic_index<eth_multi_account_trx_object, eth_multi_account_trx_index_type>;
		enum transaction_stata {
			withdraw_without_sign_trx_uncreate = 0,
			withdraw_without_sign_trx_create = 1,
			withdraw_sign_trx = 2,
			withdraw_combine_trx_create = 3,
			withdraw_transaction_confirm = 4,
			withdraw_transaction_fail = 5,
			withdraw_canceled =6,
			withdraw_eth_guard_need_sign = 7,
			withdraw_eth_guard_sign = 8
		};
		enum acquired_trx_state {
			acquired_trx_uncreate = 0,
			acquired_trx_create = 1,
			acquired_trx_comfirmed = 2,
			acquired_trx_deposit_failure = -1
		};
		class crosschain_transaction_history_count_object;
		class crosschain_transaction_history_count_object :public graphene::db::abstract_object<crosschain_transaction_history_count_object> {
		public:
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = crosschain_transaction_history_count_object_type;
			std::string asset_symbol;
			uint32_t  local_count;
		};
		struct by_history_count_id;
		struct by_history_asset_symbol;
		using transaction_history_count_multi_index_type = multi_index_container <
			crosschain_transaction_history_count_object,
			indexed_by <
			ordered_unique< tag<by_history_count_id>,
			member<object, object_id_type, &object::id>
			>,
			ordered_unique< tag<by_history_asset_symbol>,
			member<crosschain_transaction_history_count_object, std::string, &crosschain_transaction_history_count_object::asset_symbol>
			>
			>
		>;
		using transaction_history_count_index = generic_index<crosschain_transaction_history_count_object, transaction_history_count_multi_index_type>;
		class acquired_crosschain_trx_object;
		class acquired_crosschain_trx_object :public graphene::db::abstract_object<acquired_crosschain_trx_object> {
		public:
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = acquired_crosschain_object_type;
			hd_trx handle_trx;
			std::string handle_trx_id;
			acquired_trx_state acquired_transaction_state;
			acquired_crosschain_trx_object() {}
		};
		struct by_acquired_trx_id;
		struct by_acquired_trx_state;
		using acquired_crosschain_multi_index_type = multi_index_container <
			acquired_crosschain_trx_object,
			indexed_by <
			ordered_unique< tag<by_id>,
			member<object, object_id_type, &object::id>
			>,
			ordered_non_unique< tag<by_acquired_trx_state>,
			member<acquired_crosschain_trx_object, acquired_trx_state, &acquired_crosschain_trx_object::acquired_transaction_state>
			>,
			ordered_unique< tag<by_acquired_trx_id>,
			member<acquired_crosschain_trx_object, std::string, &acquired_crosschain_trx_object::handle_trx_id>
			>
			>
		> ;
		using acquired_crosschain_index = generic_index<acquired_crosschain_trx_object, acquired_crosschain_multi_index_type>;
		class crosschain_trx_object;
		class crosschain_trx_object : public graphene::db::abstract_object<crosschain_trx_object>
		{
		public:
			
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = crosschain_trx_object_type;

			transaction_id_type relate_transaction_id;
			transaction_id_type transaction_id;
			vector<transaction_id_type> all_related_origin_transaction_ids;
			signed_transaction real_transaction;
			string without_link_account;
			uint64_t op_type;
			transaction_stata trx_state;
			string crosschain_trx_id;
			asset crosschain_fee ;
			crosschain_trx_object() {}
		};
		struct by_relate_trx_id;
		struct by_transaction_id;
		struct by_type;
		struct by_transaction_stata;
		struct by_trx_relate_type_stata;
		struct by_trx_type_state;
		struct by_original_id_optype;
		struct by_withdraw_link_account;
		using crosschain_multi_index_type = multi_index_container <
			crosschain_trx_object,
			indexed_by <
			ordered_unique< tag<by_id>,
			member<object, object_id_type, &object::id>
			>,
			ordered_non_unique< tag<by_type>,
			member<crosschain_trx_object, uint64_t, &crosschain_trx_object::op_type>
			>,
			ordered_non_unique< tag<by_transaction_stata>,
			member<crosschain_trx_object, transaction_stata, &crosschain_trx_object::trx_state>
			>,
			ordered_non_unique<	tag<by_relate_trx_id>,
			member<crosschain_trx_object, transaction_id_type, &crosschain_trx_object::relate_transaction_id>
			>,
			ordered_unique< tag<by_transaction_id>,
			member<crosschain_trx_object, transaction_id_type, &crosschain_trx_object::transaction_id>
			>,
			ordered_non_unique<
			tag<by_trx_relate_type_stata>,
			composite_key<
			crosschain_trx_object,
			member<crosschain_trx_object, transaction_id_type, &crosschain_trx_object::relate_transaction_id>,
			member<crosschain_trx_object, transaction_stata, &crosschain_trx_object::trx_state>
			>//,
			/*composite_key_compare<
			std::less< transaction_id_type >,
			std::less<transaction_stata>
			>*/
			>,
			ordered_non_unique<
			tag<by_withdraw_link_account>,
			composite_key<
			crosschain_trx_object,
			member<crosschain_trx_object, string, &crosschain_trx_object::without_link_account>
			>
			>,
			ordered_non_unique<
			tag<by_original_id_optype>,
			composite_key<
			crosschain_trx_object,
			member<crosschain_trx_object, string, &crosschain_trx_object::crosschain_trx_id>,
			member<crosschain_trx_object, uint64_t, &crosschain_trx_object::op_type>
			>
			>,
			ordered_non_unique<
			tag<by_trx_type_state>,
			composite_key<
			crosschain_trx_object,
			member<crosschain_trx_object, transaction_id_type, &crosschain_trx_object::transaction_id>,
			member<crosschain_trx_object, transaction_stata, &crosschain_trx_object::trx_state>
			>,
			composite_key_compare<
			std::less< transaction_id_type >,
			std::less<transaction_stata>
			>
			>
			>
		> ;
		using crosschain_trx_index = generic_index<crosschain_trx_object, crosschain_multi_index_type>;
	}
}
FC_REFLECT_ENUM(graphene::chain::eth_multi_account_trx_state,(sol_create_need_guard_sign)(sol_create_guard_signed)(sol_multi_account_ethchain_create)(sol_create_comlete))
FC_REFLECT_ENUM(graphene::chain::transaction_stata,
(withdraw_without_sign_trx_uncreate)
(withdraw_without_sign_trx_create)
(withdraw_sign_trx)
(withdraw_combine_trx_create)
(withdraw_transaction_confirm)
(withdraw_transaction_fail)
(withdraw_canceled)
(withdraw_eth_guard_need_sign)
(withdraw_eth_guard_sign)
)
FC_REFLECT_DERIVED(graphene::chain::crosschain_trx_object,(graphene::db::object),
	(relate_transaction_id)
	(transaction_id)
	(real_transaction)
	(crosschain_trx_id)
	(crosschain_fee)
	(without_link_account)
	(all_related_origin_transaction_ids)
	(op_type)
	(trx_state)
)

FC_REFLECT_ENUM(graphene::chain::acquired_trx_state,
	(acquired_trx_uncreate)
	(acquired_trx_create)
	(acquired_trx_comfirmed)
    (acquired_trx_deposit_failure))
FC_REFLECT_DERIVED(graphene::chain::acquired_crosschain_trx_object, (graphene::db::object), 
(handle_trx)
(handle_trx_id)
(acquired_transaction_state))
FC_REFLECT_DERIVED(graphene::chain::eth_multi_account_trx_object, (graphene::db::object), (multi_account_pre_trx_id)
(hot_sol_trx_id)(cold_sol_trx_id)(hot_addresses)(cold_addresses)(cold_trx_success)(symbol)(hot_trx_success)(multi_account_create_trx_id)(object_transaction)(state)(op_type))
FC_REFLECT_DERIVED(graphene::chain::crosschain_transaction_history_count_object, (graphene::db::object), 
	(asset_symbol)
	(local_count)
	)
