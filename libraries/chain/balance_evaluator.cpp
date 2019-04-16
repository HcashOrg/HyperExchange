/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/chain/balance_evaluator.hpp>
#include <graphene/chain/transfer_evaluator.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/witness_object.hpp>
namespace graphene { namespace chain {

void_result balance_claim_evaluator::do_evaluate(const balance_claim_operation& op)
{
   database& d = db();
   balance = &op.balance_to_claim(d);

   GRAPHENE_ASSERT(
             op.balance_owner_key == balance->owner ||
             pts_address(op.balance_owner_key, false, 56) == balance->owner ||
             pts_address(op.balance_owner_key, true, 56) == balance->owner ||
             pts_address(op.balance_owner_key, false, 0) == balance->owner ||
             pts_address(op.balance_owner_key, true, 0) == balance->owner,
             balance_claim_owner_mismatch,
             "Balance owner key was specified as '${op}' but balance's actual owner is '${bal}'",
             ("op", op.balance_owner_key)
             ("bal", balance->owner)
             );
   if( !(d.get_node_properties().skip_flags & (database::skip_authority_check |
                                               database::skip_transaction_signatures)) )

   FC_ASSERT(op.total_claimed.asset_id == balance->asset_type());

   if( balance->is_vesting_balance() )
   {
      GRAPHENE_ASSERT(
         balance->vesting_policy->is_withdraw_allowed(
            { balance->balance,
              d.head_block_time(),
              op.total_claimed } ),
         balance_claim_invalid_claim_amount,
         "Attempted to claim ${c} from a vesting balance with ${a} available",
         ("c", op.total_claimed)("a", balance->available(d.head_block_time()))
         );
      GRAPHENE_ASSERT(
         d.head_block_time() - balance->last_claim_date >= fc::days(1),
         balance_claim_claimed_too_often,
         "Genesis vesting balances may not be claimed more than once per day."
         );
      return {};
   }

   FC_ASSERT(op.total_claimed == balance->balance);
   return {};
}

/**
 * @note the fee is always 0 for this particular operation because once the
 * balance is claimed it frees up memory and it cannot be used to spam the network
 */
void_result balance_claim_evaluator::do_apply(const balance_claim_operation& op)
{
   database& d = db();

   if( balance->is_vesting_balance() && op.total_claimed < balance->balance )
      d.modify(*balance, [&](balance_object& b) {
         b.vesting_policy->on_withdraw({b.balance, d.head_block_time(), op.total_claimed});
         b.balance -= op.total_claimed;
         b.last_claim_date = d.head_block_time();
      });
   else
      d.remove(*balance);

   d.adjust_balance(op.deposit_to_account, op.total_claimed);
   return {};
}

void_result set_balance_evaluator::do_evaluate(const set_balance_operation& op)
{
	try {
		/*
		1.check balance
		2.check whiteops
		3.check lockbalance
		*/
		const database& d = db();
	    const auto& idx = d.get_index_type<account_index>().indices().get<by_address>();
		auto iter = idx.find(op.addr_to_deposit);
		FC_ASSERT(iter != idx.end(),"${addr} should be registerd on the chain.",("addr",op.addr_to_deposit));
		deposited_account_id = iter->get_id();
		iter = idx.find(op.addr_from_claim);
		FC_ASSERT(iter != idx.end(), "${addr} should be a senator.", ("addr", op.addr_from_claim));
		const auto& guards_index = d.get_index_type<guard_member_index>().indices().get<by_account>();
		auto senator_itr = guards_index.find(iter->get_id());
		FC_ASSERT(senator_itr != guards_index.end() && senator_itr->senator_type == PERMANENT,"need to be permanent senator.");

		FC_ASSERT(d.is_white(op.addr_to_deposit, operation(transfer_operation()).which()),"${addr} cannot be claimed balance.",("addr",op.addr_to_deposit));
		auto balance = d.get_balance(op.addr_to_deposit, op.claimed.asset_id);
		lockbalance_objs = d.get_lock_balance(deposited_account_id,op.claimed.asset_id);


	}FC_CAPTURE_AND_RETHROW((op))
}
void_result set_balance_evaluator::do_apply(const set_balance_operation& op)
{
	try {
		database& d = db();

		auto balance = d.get_balance(op.addr_to_deposit, op.claimed.asset_id);
		asset lockbalance(0,op.claimed.asset_id);
		for (const auto& lockbalance_obj : lockbalance_objs)
		{
			lockbalance += asset(lockbalance_obj.lock_asset_amount, lockbalance_obj.lock_asset_id);
		}

		asset total = balance + lockbalance;
		if (total < op.claimed)
		{
			//todo just modify the balance
			d.adjust_balance(op.addr_to_deposit, op.claimed - total);
		}
		else if (total > op.claimed)
		{
			//todo pull back the lockbalance
			// modify the balance
			for (const auto& lockbalance_obj : lockbalance_objs)
			{
				auto citizen = lockbalance_obj.lockto_miner_account;
				d.adjust_lock_balance(citizen, deposited_account_id, -asset(lockbalance_obj.lock_asset_amount, lockbalance_obj.lock_asset_id));
				d.modify(d.get_lockbalance_records(), [&](lockbalance_record_object& obj) {
					obj.record_list[op.addr_to_deposit][lockbalance_obj.lock_asset_id] -= lockbalance_obj.lock_asset_amount;
				});
					
			}
			d.adjust_balance(op.addr_to_deposit, -balance);
			d.adjust_balance(op.addr_to_deposit, op.claimed);
			
		}
		else
		{
			return void_result();
		}
		d.modify(op.claimed.asset_id(d).dynamic_data(d), [total,op](asset_dynamic_data_object& obj) {
			obj.current_supply += (op.claimed - total).amount;
		});
	}FC_CAPTURE_AND_RETHROW((op))
}


void_result correct_chain_data_evaluator::do_evaluate(const correct_chain_data_operation& op)
{
	try {
		const database& d = db();
		const auto& idx = d.get_index_type<account_index>().indices().get<by_address>();
		auto iter = idx.find(op.payer);
		FC_ASSERT(iter != idx.end(), "${addr} should be registerd on the chain.", ("addr", op.payer));
		const auto& guards_index = d.get_index_type<guard_member_index>().indices().get<by_account>();
		const auto& citizen_idx = d.get_index_type<miner_index>().indices().get<by_account>();
		auto senator_itr = guards_index.find(iter->get_id());
		FC_ASSERT(senator_itr != guards_index.end() && senator_itr->senator_type == PERMANENT, "need to be permanent senator.");

		for (const auto& addr : op.correctors)
		{
			iter = idx.find(addr);
			FC_ASSERT(iter != idx.end(), "${addr} should be registerd on the chain.", ("addr", addr));
			FC_ASSERT(citizen_idx.find(iter->get_id()) != citizen_idx.end(),"${addr} should be a citizen",("addr",addr));
		}
		return void_result();
	}FC_CAPTURE_AND_RETHROW((op))
}
void_result correct_chain_data_evaluator::do_apply(const correct_chain_data_operation& op)
{
	try {
		database& d = db();
		std::map<miner_id_type, map<string,asset>> citizens;
		const auto& citizen_idx = d.get_index_type<miner_index>();
		const auto& citizen_itrs = citizen_idx.indices().get<by_account>();
		if (op.correctors.size() == 0)
		{
			//const auto& citizen_idx = d.get_index_type<miner_index>();
			citizen_idx.inspect_all_objects([&](const object& obj) {
				const miner_object& c = static_cast<const miner_object&>(obj);
				citizens[c.id] = c.lockbalance_total;
			});

		}
		else
		{
			const auto& idx = d.get_index_type<account_index>().indices().get<by_address>();
			for (const auto& addr : op.correctors)
			{
				auto iter = idx.find(addr);
				const auto citizen_itr = citizen_itrs.find(iter->get_id());
				citizens[citizen_itr->id] = citizen_itr->lockbalance_total;
			}
		}
		for (const auto& ctzen : citizens)
		{
			std::map<string, asset> citizen_locks;
			const auto& vec_locks = d.get_citizen_lockbalance_info(ctzen.first);
			for (const auto& lock : vec_locks)
			{
				for (const auto& t : lock.second)
				{
					const auto& asset_obj = d.get(t.asset_id);
					if (citizen_locks.count(asset_obj.symbol))
					{
						citizen_locks[asset_obj.symbol] += t;
					}
					else
						citizen_locks[asset_obj.symbol] = t;
				}
			}
			if (citizen_locks != ctzen.second)
			{
				d.modify(d.get(ctzen.first), [&](miner_object& obj) {
					obj.lockbalance_total = citizen_locks;
				});
			}
		}

	}FC_CAPTURE_AND_RETHROW((op))
}


} } // namespace graphene::chain
