#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <map>


namespace graphene {
	namespace chain {
		//using namespace graphene::db;
		class lockbalance_object;
		class lockbalance_object : public graphene::db::abstract_object<lockbalance_object>
		{
		public:
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = lockbalance_object_type;
			account_id_type lockto_miner_account;
			account_id_type lock_balance_account;
			address lock_balance_contract_addr;
			asset_id_type lock_asset_id;
			share_type lock_asset_amount;

			asset get_lock_balance() const{
				return asset(lock_asset_amount, lock_asset_id);
			}
		};
		//TODO: Add contract Handle
		struct by_lock_miner_asset;
		typedef multi_index_container <
			lockbalance_object,
			indexed_by <
			ordered_unique< tag<by_id>,
			member<object, object_id_type, &object::id>
			>,
			ordered_unique<
			tag<by_lock_miner_asset>,
			composite_key<
			lockbalance_object,
			member<lockbalance_object, account_id_type, &lockbalance_object::lockto_miner_account>,
			member<lockbalance_object, account_id_type, &lockbalance_object::lock_balance_account>,			
			member<lockbalance_object, asset_id_type, &lockbalance_object::lock_asset_id>
			>,
			composite_key_compare<
			std::less< account_id_type >,
			std::less< account_id_type >,
			std::less< asset_id_type >
			>
			>
			>
		> lockbalance_multi_index_type;
		typedef  generic_index<lockbalance_object, lockbalance_multi_index_type> lockbalance_index;
	}
}

FC_REFLECT_DERIVED(graphene::chain::lockbalance_object, (graphene::db::object),
(lockto_miner_account)
(lock_balance_account)
(lock_balance_contract_addr)
(lock_asset_id)
(lock_asset_amount)
)