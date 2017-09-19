#include <graphene/chain/database.hpp>

#include <graphene/chain/lockbalance_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/balance_object.hpp>
namespace graphene {
	namespace chain {

		asset database::get_lock_balance(account_id_type owner, account_id_type miner, asset_id_type asset_id) const{
			auto& index = get_index_type<lockbalance_index>().indices().get<by_lock_miner_asset>();
			auto itr = index.find(boost::make_tuple(miner, owner, asset_id));
			if (itr != index.end()){
				return itr->get_lock_balance();
			}
						
			return asset();
		}
// 		void database::adjust_guard_lock_balance() {
// 
// 		}
		void database::adjust_lock_balance(account_id_type miner_account, account_id_type lock_account,asset delta){
			try {
				if (delta.amount == 0)
					return;
				auto& by_owner_idx = get_index_type<lockbalance_index>().indices().get<by_lock_miner_asset>();
				auto itr = by_owner_idx.find(boost::make_tuple(miner_account, lock_account, delta.asset_id));
				//fc::time_point_sec now = head_block_time();
				if (itr == by_owner_idx.end()){
					FC_ASSERT(delta.amount > 0, "lock balance error");
					create<lockbalance_object>([miner_account, lock_account, delta](lockbalance_object& a) {
						a.lockto_miner_account = miner_account;
						a.lock_balance_account = lock_account;
						//address lock_balance_contract_addr;
						a.lock_asset_id = delta.asset_id;
						a.lock_asset_amount = delta.amount;
					});
				}
				else{
					if (delta.amount < 0){
						FC_ASSERT(asset(itr->lock_asset_amount, itr->lock_asset_id) >= -delta, "foreclose balance is not enough");
					}
					modify(*itr, [miner_account, lock_account, delta](lockbalance_object& b) {
						b.lock_asset_amount += delta.amount;
					});
				}
			}FC_CAPTURE_AND_RETHROW((miner_account)(lock_account)(delta))
		}
	}
}