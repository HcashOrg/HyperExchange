#include <graphene/chain/guard_lock_balance_evaluator.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/lockbalance_object.hpp>
namespace graphene {
	namespace chain {
		void_result guard_lock_balance_evaluator::do_evaluate(const guard_lock_balance_operation& o) {
			try {
				const database& d = db();
				const asset_object&   asset_type = o.lock_asset_id(d);
				// 			auto & iter = d.get_index_type<guard_member_index>().indices().get<by_account>();
				// 			auto itr = iter.find(o.lock_balance_account);
				// 			FC_ASSERT(itr != iter.end(), "Dont have lock guard account");
				optional<guard_member_object> iter = d.get(o.lock_balance_account);
				FC_ASSERT(iter.valid(), "Dont have lock account");
				optional<account_object> account_iter = d.get(iter->guard_member_account);
				FC_ASSERT(account_iter.valid() && 
						  account_iter->addr == o.lock_address && 
						  iter->guard_member_account == o.lock_balance_account_id, "Address or Account is wrong");
				bool insufficient_balance = d.get_balance(o.lock_address, asset_type.id).amount >= o.lock_asset_amount;
				FC_ASSERT(insufficient_balance, "Lock balance fail because lock account own balance is not enough");

				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))			
		}
		void_result guard_lock_balance_evaluator::do_apply(const guard_lock_balance_operation& o) {
			try {
				database& d = db();
				const asset_object&   asset_type = o.lock_asset_id(d);
				d.adjust_balance(o.lock_address, asset(-o.lock_asset_amount, o.lock_asset_id));
				d.adjust_guard_lock_balance(o.lock_balance_account, asset(o.lock_asset_amount, o.lock_asset_id));
				//d.adjust_lock_balance(o.lockto_miner_account, o.lock_balance_account, o.lock_asset_amount);
				// 			auto & iter = d.get_index_type<guard_member_index>().indices().get<by_account>();
				// 			auto itr = iter.find(o.lock_balance_account);
				//optional<guard_member_object> iter = d.get(o.lock_balance_account);
				d.modify(d.get(o.lock_balance_account), [o, asset_type](guard_member_object& b) {
					auto map_guard_lockbalance_total = b.guard_lock_balance.find(asset_type.symbol);
					if (map_guard_lockbalance_total != b.guard_lock_balance.end()) {
						map_guard_lockbalance_total->second += asset(o.lock_asset_amount,o.lock_asset_id);
					}
					else {
						b.guard_lock_balance[asset_type.symbol] = asset(o.lock_asset_amount,o.lock_asset_id);
					}
				});
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))			
		}
		void_result guard_foreclose_balance_evaluator::do_evaluate(const guard_foreclose_balance_operation& o) {
			try {
				const database& d = db();
				const asset_object&   asset_type = o.foreclose_asset_id(d);
				// 			auto & iter = d.get_index_type<guard_member_index>().indices().get<by_account>();
				// 			auto itr = iter.find(o.lock_balance_account);
				// 			FC_ASSERT(itr != iter.end(), "Dont have lock guard account");
				optional<guard_member_object> iter = d.get(o.foreclose_balance_account);
				FC_ASSERT(iter.valid(), "Dont have lock account");
				optional<account_object> account_iter = d.get(iter->guard_member_account);
				FC_ASSERT(account_iter.valid() && account_iter->addr == o.foreclose_address && o.foreclose_balance_account_id == iter->guard_member_account, "Address is wrong");
				bool insufficient_balance = d.get_guard_lock_balance(iter->id, asset_type.id).amount >= o.foreclose_asset_amount;
				FC_ASSERT(insufficient_balance, "Lock balance fail because lock account own balance is not enough");
			}FC_CAPTURE_AND_RETHROW((o))			
		}
		void_result guard_foreclose_balance_evaluator::do_apply(const guard_foreclose_balance_operation& o) {
			try {
				database& d = db();
				const asset_object&   asset_type = o.foreclose_asset_id(d);
				d.adjust_balance(o.foreclose_address, asset(o.foreclose_asset_amount,o.foreclose_asset_id));
				d.adjust_guard_lock_balance(o.foreclose_balance_account, asset(-o.foreclose_asset_amount, o.foreclose_asset_id));
				//d.adjust_lock_balance(o.lockto_miner_account, o.lock_balance_account, o.lock_asset_amount);
				// 			auto & iter = d.get_index_type<guard_member_index>().indices().get<by_account>();
				// 			auto itr = iter.find(o.lock_balance_account);
				//optional<guard_member_object> iter = d.get(o.foreclose_balance_account);
				d.modify(d.get(o.foreclose_balance_account), [o, asset_type](guard_member_object& b) {
					auto map_guard_lockbalance_total = b.guard_lock_balance.find(asset_type.symbol);
					if (map_guard_lockbalance_total != b.guard_lock_balance.end()) {
						map_guard_lockbalance_total->second -= asset(o.foreclose_asset_amount,o.foreclose_asset_id);
					}

				});
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))			
		}

		void_result guard_update_multi_account_evaluator::do_evaluate(const guard_update_multi_account_operation& o)
		{
			try
			{
				const database& d = db();
				const auto& assets = d.get_index_type<asset_index>().indices().get<by_symbol>();
				FC_ASSERT(assets.find(o.chain_type)!=assets.end());
				FC_ASSERT(o.hot != o.cold);
				const auto& multi_assets = d.get_index_type<multisig_account_pair_index>().indices().get<by_multisig_account_pair>();
				auto multisig_account_obj = multi_assets.find(boost::make_tuple(o.hot,o.cold,o.chain_type));
				FC_ASSERT(multisig_account_obj != multi_assets.end());

                //get the multi-asset,and make it get worked
			}FC_CAPTURE_AND_RETHROW((o))
		}

		void_result guard_update_multi_account_evaluator::do_apply(const guard_update_multi_account_operation& o)
		{
			try
			{
				//TODO
				//we need implemention
				database& d = db();
				//modify the data for the
				auto& multi_assets = d.get_index_type<multisig_account_pair_index>().indices().get<by_multisig_account_pair>();
				auto multisig_account_obj = multi_assets.find(boost::make_tuple(o.hot, o.cold, o.chain_type));

				auto head_num = d.head_block_num();
				d.modify(*multisig_account_obj, [o,head_num](multisig_account_pair_object& obj) {
					obj.effective_block_num = head_num + 10;
				});
				//multi_assets.modify();
			}FC_CAPTURE_AND_RETHROW((o))
		}

	}
}
