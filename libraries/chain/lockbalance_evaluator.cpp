#include <graphene/chain/lockbalance_evaluator.hpp>
#include <graphene/chain/witness_object.hpp>
namespace graphene{
	namespace chain{
		void_result lockbalance_evaluator::do_evaluate(const lockbalance_operation& o){ 
			try{
				const database& d = db();
				if (o.contract_addr == address()) {
					const asset_object&   asset_type = o.lock_asset_id(d);
// 					auto & iter = d.get_index_type<witness_index>().indices().get<by_account>();
// 					auto itr = iter.find(o.lockto_miner_account);
// 					FC_ASSERT(itr != iter.end(), "Dont have lock account");
					optional<witness_object> iter = d.get(o.lockto_miner_account);
					FC_ASSERT(iter.valid(),"Dont have lock account");
					bool insufficient_balance = d.get_balance(o.lock_balance_account, asset_type.id).amount >= o.lock_asset_amount;
					FC_ASSERT(insufficient_balance, "Lock balance fail because lock account own balance is not enough");
				}
				else {
					//TODO : ADD Handle Contact lock balance
				}
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o)) 			
		}
		void_result lockbalance_evaluator::do_apply(const lockbalance_operation& o){
			try {
				if (o.contract_addr == address()) {
					database& d = db();
					const asset_object&   asset_type = o.lock_asset_id(d);
					d.adjust_balance(o.lock_balance_account, -o.lock_asset_amount);
					d.adjust_lock_balance(o.lockto_miner_account, o.lock_balance_account,o.lock_asset_amount);
					optional<witness_object> itr = d.get(o.lockto_miner_account);
					d.modify(*itr, [o,asset_type](witness_object& b) {
						auto& map_lockbalance_total = b.lockbalance_total.find(asset_type.symbol);
						if (map_lockbalance_total != b.lockbalance_total.end())	{
							map_lockbalance_total->second += o.lock_asset_amount;
						}
						else {
							b.lockbalance_total[asset_type.symbol] = o.lock_asset_amount;
						}
					});
				}
				else {
					//TODO : ADD Handle Contact lock balance
				}
				return void_result();
			} FC_CAPTURE_AND_RETHROW((o))
		}
		void_result foreclose_balance_evaluator::do_evaluate(const foreclose_balance_operation& o) {
			try {
				const database& d = db();
				if (o.foreclose_contract_addr == address()) {
					const asset_object&   asset_type = o.foreclose_asset_id(d);
					optional<witness_object> iter = d.get(o.foreclose_miner_account);
					FC_ASSERT(iter.valid(), "Dont have lock account");
					//TODO Add lock balance db get function
				}
				else {
					//TODO : ADD Handle Contact lock balance
				}
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result foreclose_balance_evaluator::do_apply(const foreclose_balance_operation& o) {
			return void_result();
		}
	}
}