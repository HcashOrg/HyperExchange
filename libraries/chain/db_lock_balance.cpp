#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/authority.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/lockbalance_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/guard_lock_balance_object.hpp>
namespace graphene {
	namespace chain {

		asset database::get_lock_balance(account_id_type owner, miner_id_type miner, asset_id_type asset_id) const{
			try {
				auto& index = get_index_type<lockbalance_index>().indices().get<by_lock_miner_asset>();
				auto itr = index.find(boost::make_tuple(miner, owner, asset_id));
				if (itr != index.end()) {
					return itr->get_lock_balance();
				}
				return asset();
			}FC_CAPTURE_AND_RETHROW((owner)(miner)(asset_id))
		}

		vector<lockbalance_object> database::get_lock_balance(account_id_type owner, asset_id_type asset_id) const {
			try {
				const auto& lb_index = get_index_type<lockbalance_index>();
				vector<lockbalance_object> result;
				lb_index.inspect_all_objects([&](const object& obj) {
					const lockbalance_object& p = static_cast<const lockbalance_object&>(obj);
					if (p.lock_balance_account == owner && p.lock_asset_amount > 0  && p.lock_asset_id == asset_id) {
						result.push_back(p);
					}
				});
				return result;
			}FC_CAPTURE_AND_RETHROW((owner)(asset_id))
		}

		asset database::get_guard_lock_balance(guard_member_id_type guard, asset_id_type asset_id) const {
			try {
				auto& index = get_index_type<guard_lock_balance_index>().indices().get<by_guard_lock>();
				auto itr = index.find(boost::make_tuple(guard, asset_id));
				if (itr != index.end()) {
					return itr->get_guard_lock_balance();
				}
				return asset();
			}FC_CAPTURE_AND_RETHROW((guard)(asset_id))
		}
		void database::adjust_guard_lock_balance(guard_member_id_type guard_account, asset delta) {
			try {
				if (delta.amount == 0){
					return;
				}
				auto& by_guard_idx = get_index_type<guard_lock_balance_index>().indices().get<by_guard_lock>();
				auto itr = by_guard_idx.find(boost::make_tuple(guard_account, delta.asset_id));
				if (itr == by_guard_idx.end()){
					FC_ASSERT(delta.amount > 0, "lock balance error");
					create<guard_lock_balance_object>([guard_account, delta] (guard_lock_balance_object& a) {
						a.lock_asset_id = delta.asset_id;
						a.lock_asset_amount = delta.amount;
						a.lock_balance_account = guard_account;
					});
				}
				else {
					if (delta.amount < 0) {
						FC_ASSERT(asset(itr->lock_asset_amount, itr->lock_asset_id) >= -delta, "foreclose guard balance is not enough");
					}
					modify(*itr, [guard_account, delta](guard_lock_balance_object& b) {
						b.lock_asset_amount += delta.amount;
					});
				}
			}FC_CAPTURE_AND_RETHROW((guard_account)(delta))
		}
		void database::adjust_lock_balance(miner_id_type miner_account, account_id_type lock_account,asset delta){
			try {
				if (delta.amount == 0) {
					return;
				}
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
					if ((asset(itr->lock_asset_amount, itr->lock_asset_id) + delta).amount == 0)
						remove(*itr);
					else{
						modify(*itr, [miner_account, lock_account, delta](lockbalance_object& b) {
							b.lock_asset_amount += delta.amount;
						});
					}

				}
				if (head_block_num() >= LOCKBALANCE_CORRECT)
				{ 
					const auto& asset_obj = get(delta.asset_id);
					modify(get(miner_account), [&](miner_object& b) {
						auto map_lockbalance_total = b.lockbalance_total.find(asset_obj.symbol);
						if (map_lockbalance_total != b.lockbalance_total.end()) {
							map_lockbalance_total->second += delta;
						}
						else {
							b.lockbalance_total[asset_obj.symbol] = delta;
						}
					});
				}
				
			}FC_CAPTURE_AND_RETHROW((miner_account)(lock_account)(delta))
		}
	}
}