#include <graphene/chain/lockbalance_evaluator.hpp>
#include <graphene/chain/witness_object.hpp>
namespace graphene{
	namespace chain{
		void_result lockbalance_evaluator::do_evaluate(const lockbalance_operation& o){ 
			try{
				const database& d = db();
				if (o.contract_addr == address()) {
					const asset_object&   asset_type = o.lock_asset_id(d);
				
// 					auto & iter = d.get_index_type<miner_index>().indices().get<by_account>();
// 					auto itr = iter.find(o.lockto_miner_account);
// 					FC_ASSERT(itr != iter.end(), "Dont have lock account");
					optional<miner_object> iter = d.get(o.lockto_miner_account);
					FC_ASSERT(iter.valid(),"Dont have lock account");
					optional<account_object> account_iter = d.get(o.lock_balance_account);
					FC_ASSERT(account_iter.valid() && account_iter->addr == o.lock_balance_addr, "Address is wrong");
					bool insufficient_balance = d.get_balance(o.lock_balance_addr, asset_type.id).amount >= o.lock_asset_amount;
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
					d.adjust_balance(o.lock_balance_addr, asset(-o.lock_asset_amount,o.lock_asset_id));
					d.adjust_lock_balance(o.lockto_miner_account, o.lock_balance_account,asset(o.lock_asset_amount,o.lock_asset_id));
					//optional<miner_object> itr = d.get(o.lockto_miner_account);
					if (d.head_block_num() < LOCKBALANCE_CORRECT)
					{
						d.modify(d.get(o.lockto_miner_account), [o, asset_type](miner_object& b) {
							auto map_lockbalance_total = b.lockbalance_total.find(asset_type.symbol);
							if (map_lockbalance_total != b.lockbalance_total.end()) {
								map_lockbalance_total->second += asset(o.lock_asset_amount, o.lock_asset_id);
							}
							else {
								b.lockbalance_total[asset_type.symbol] = asset(o.lock_asset_amount, o.lock_asset_id);
							}
						});
					}
					d.modify(d.get_lockbalance_records(), [o](lockbalance_record_object& obj) {
						obj.record_list[o.lock_balance_addr][o.lock_asset_id] += o.lock_asset_amount;
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
					optional<miner_object> iter = d.get(o.foreclose_miner_account);
					FC_ASSERT(iter.valid(), "Dont have lock account");
					optional<account_object> account_iter = d.get(o.foreclose_account);
					FC_ASSERT(account_iter.valid() && account_iter->addr == o.foreclose_addr, "Address is wrong");
					bool insufficient_balance = d.get_lock_balance(o.foreclose_account, o.foreclose_miner_account, asset_type.id).amount >= o.foreclose_asset_amount;
					FC_ASSERT(insufficient_balance, "Lock balance fail because lock account own balance is not enough");
				}
				else {
					//TODO : ADD Handle Contact lock balance
				}
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result foreclose_balance_evaluator::do_apply(const foreclose_balance_operation& o) {
			try {
			database& d = db();
			if (o.foreclose_contract_addr == address()) {
				const asset_object&   asset_type = o.foreclose_asset_id(d);
				d.adjust_lock_balance(o.foreclose_miner_account, o.foreclose_account, asset(-o.foreclose_asset_amount,o.foreclose_asset_id));
				d.adjust_balance(o.foreclose_addr, asset(o.foreclose_asset_amount,o.foreclose_asset_id));

				//optional<miner_object> itr = d.get(o.foreclose_miner_account);
				if (d.head_block_num() < LOCKBALANCE_CORRECT)
					d.modify(d.get(o.foreclose_miner_account), [o, asset_type](miner_object& b) {
					auto map_lockbalance_total = b.lockbalance_total.find(asset_type.symbol);
					if (map_lockbalance_total != b.lockbalance_total.end()) {
						map_lockbalance_total->second -= asset(o.foreclose_asset_amount, o.foreclose_asset_id);
					}

				});
				
				d.modify(d.get_lockbalance_records(), [o](lockbalance_record_object& obj) {
					obj.record_list[o.foreclose_addr][o.foreclose_asset_id] -= o.foreclose_asset_amount;
				});
			}
			else {
				// TODO : ADD Handle Contact lock balance
			}
			
			return void_result();
			} FC_CAPTURE_AND_RETHROW((o))
		}
	}
}
