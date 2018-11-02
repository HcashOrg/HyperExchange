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
#include <graphene/chain/referendum_evaluator.hpp>
#include <graphene/chain/referendum_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

void_result referendum_create_evaluator::do_evaluate(const referendum_create_operation& o)
{ try {
   const database& d = db();
   const auto& global_parameters = d.get_global_properties().parameters;
   //proposer has to be a formal guard
   auto  proposer = o.proposer;
   auto& guard_index = d.get_index_type<guard_member_index>().indices().get<by_account>();
   auto iter = guard_index.find(proposer);
   FC_ASSERT(iter != guard_index.end(), "propser has to be a guard.");

   const auto& dynamic_obj = d.get_dynamic_global_properties();
   if (!(!dynamic_obj.referendum_flag || (dynamic_obj.referendum_flag && d.head_block_time() < dynamic_obj.next_vote_time)))
	   return void_result();
   for (const op_wrapper& op : o.proposed_ops)
   {
	   _proposed_trx.operations.push_back(op.op);

   }
   transaction_evaluation_state eval_state(&db());
   eval_state.operation_results.reserve(_proposed_trx.operations.size());

   auto ptrx = processed_transaction(_proposed_trx);
   eval_state._trx = &ptrx;

   for (const auto& op : _proposed_trx.operations)
   {
	   unique_ptr<op_evaluator>& eval = db().get_evaluator(op);
	   eval->evaluate(eval_state, op, false);
   }

   _proposed_trx.validate();
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

object_id_type referendum_create_evaluator::do_apply(const referendum_create_operation& o)
{ try {
   database& d = db();
   const auto& ref_obj= d.create<referendum_object>([&](referendum_object& referendum) {
	   _proposed_trx.expiration = d.head_block_time()+fc::days(10);
	   referendum.proposed_transaction = _proposed_trx;
       //referendum.expiration_time = o.expiration_time;
       referendum.proposer = o.proposer;
       //proposal should only be approved by guard or miners
	   const auto& acc = d.get_index_type<account_index>().indices().get<by_id>();
	   const auto& iter = d.get_index_type<miner_index>().indices().get<by_account>();
	   std::for_each(iter.begin(), iter.end(), [&](const miner_object& a)
	   {
		   referendum.required_account_approvals.insert(acc.find(a.miner_account)->addr);
	   });
	   referendum.pledge = o.fee.amount;
   });
   return ref_obj.id;
} FC_CAPTURE_AND_RETHROW( (o) ) }

void referendum_create_evaluator::pay_fee()
{
	FC_ASSERT(core_fees_paid.asset_id == asset_id_type());
	db().modify(db().get(asset_id_type()).dynamic_asset_data_id(db()), [this](asset_dynamic_data_object& d) {
		d.current_supply -= this->core_fees_paid.amount;
	});
}


void_result referendum_update_evaluator::do_evaluate(const referendum_update_operation& o)
{
	try
	{
		database& d = db();
		_referendum = &o.referendum(d);
		auto next_vote_time = d.get_dynamic_global_properties().next_vote_time;
		FC_ASSERT(d.head_block_time() >= next_vote_time,"the referendum vote is in its packing period.");
		return void_result();
	}
	FC_CAPTURE_AND_RETHROW((o))
}
void_result referendum_update_evaluator::do_apply(const referendum_update_operation& o)
{
	try {
		database& d = db();
		d.modify(*_referendum, [&o, &d](referendum_object& p) {
			//FC_ASSERT(p.required_account_approvals(););

			auto is_miner_or_guard = [&](const address& addr)->bool {
				return p.required_account_approvals.find(addr) != p.required_account_approvals.end();
			};

			for (const auto& addr : o.key_approvals_to_add)
			{
				if (!is_miner_or_guard(addr))
					continue;
				p.approved_key_approvals.insert(addr);
				p.disapproved_key_approvals.erase(addr);
			}

			for (const auto& addr : o.key_approvals_to_remove)
			{
				if (!is_miner_or_guard(addr))
					continue;
				p.disapproved_key_approvals.insert(addr);
				p.approved_key_approvals.erase(addr);
			}

		});

		if (_referendum->review_period_time)
			return void_result();

		if (_referendum->is_authorized_to_execute(d))
		{
			// All required approvals are satisfied. Execute!
			_executed_referendum = true;
			try {
				_processed_transaction = d.push_referendum(*_referendum);
			}
			catch (fc::exception& e) {
				wlog("Proposed transaction ${id} failed to apply once approved with exception:\n----\n${reason}\n----\nWill try again when it expires.",
					("id", o.referendum)("reason", e.to_detail_string()));
				_referendum_failed = true;
			}
		}
		return void_result();
	}FC_CAPTURE_AND_RETHROW((o))
}


} } // graphene::chain
