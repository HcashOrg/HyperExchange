#include <graphene/chain/guard_lock_balance_evaluator.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/lockbalance_object.hpp>
namespace graphene {
	namespace chain {
		void_result guard_lock_balance_evaluator::do_evaluate(const guard_lock_balance_operation& o) {
			const database& d = db();
			const asset_object&   asset_type = o.lock_asset_id(d);
// 			auto & iter = d.get_index_type<committee_member_index>().indices().get<by_account>();
// 			auto itr = iter.find(o.lock_balance_account);
// 			FC_ASSERT(itr != iter.end(), "Dont have lock guard account");
			optional<committee_member_object> iter = d.get(o.lock_balance_account);
			FC_ASSERT(iter.valid(),"Dont have lock account");
			bool insufficient_balance = d.get_balance(iter->committee_member_account, asset_type.id).amount >= o.lock_asset_amount;
			FC_ASSERT(insufficient_balance, "Lock balance fail because lock account own balance is not enough");
			
			return void_result();
		}
		void_result guard_lock_balance_evaluator::do_apply(const guard_lock_balance_operation& o) {
			database& d = db();
			const asset_object&   asset_type = o.lock_asset_id(d);
			d.adjust_balance(o.lock_balance_account_id, -o.lock_asset_amount);
			//d.adjust_lock_balance(o.lockto_miner_account, o.lock_balance_account, o.lock_asset_amount);
// 			auto & iter = d.get_index_type<committee_member_index>().indices().get<by_account>();
// 			auto itr = iter.find(o.lock_balance_account);
			optional<committee_member_object> iter = d.get(o.lock_balance_account);
			d.modify(*iter, [o, asset_type](committee_member_object& b) {
				auto& map_guard_lockbalance_total = b.guard_lock_balance.find(asset_type.symbol);
				if (map_guard_lockbalance_total != b.guard_lock_balance.end()) {
					map_guard_lockbalance_total->second += o.lock_asset_amount;
				}
				else {
					b.guard_lock_balance[asset_type.symbol] = o.lock_asset_amount;
				}
			});
			return void_result();
		}
	}
}