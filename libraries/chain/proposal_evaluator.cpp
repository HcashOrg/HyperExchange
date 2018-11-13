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
#include <graphene/chain/proposal_evaluator.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/referendum_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <fc/smart_ref_impl.hpp>


namespace graphene { namespace chain {

void_result proposal_create_evaluator::do_evaluate(const proposal_create_operation& o)
{ try {
   const database& d = db();
   const auto& global_parameters = d.get_global_properties().parameters;

   FC_ASSERT( o.expiration_time > d.head_block_time(), "Proposal has already expired on creation." );
   FC_ASSERT(o.expiration_time <= d.head_block_time() + global_parameters.maximum_proposal_lifetime,
	   "Proposal expiration time is too far in the future.");
   FC_ASSERT(!o.review_period_seconds || fc::seconds(*o.review_period_seconds) < (o.expiration_time - d.head_block_time()),
	   "Proposal review period must be less than its overall lifetime.");
   // If we're dealing with the committee authority, make sure this transaction has a sufficient review period.
   const auto& proposal_idx = db().get_index_type<proposal_index>().indices().get<by_id>();
   flat_set<account_id_type> auths;
   vector<authority> other;
   bool senator_type_validate = false;
   for (auto& op : o.proposed_ops)
   {
	   operation_get_required_authorities(op.op, auths, auths, other);
	   
	   if (op.op.which() == operation::tag<guard_refund_crosschain_trx_operation>::value) {
		   FC_ASSERT(o.type == vote_id_type::cancel_commit, "vote Type error");
	   }
	   if (op.op.which() == operation::tag<eth_cancel_coldhot_fail_trx_operaion>::value) {
		   FC_ASSERT(o.type == vote_id_type::cancel_commit, "vote Type error");
	   }
	   if (op.op.which() == operation::tag<eth_cancel_fail_crosschain_trx_operation>::value) {
		   FC_ASSERT(o.type == vote_id_type::cancel_commit, "vote Type error");
	   }
	   else if (op.op.which() == operation::tag<guard_member_resign_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error.");
	   }
	   else if (op.op.which() == operation::tag<transfer_contract_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<guard_member_update_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
		   senator_type_validate = true;
	   }
	   else if (op.op.which() == operation::tag<publisher_appointed_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if(op.op.which() == operation::tag<publisher_canceled_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<asset_fee_modification_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<set_guard_lockbalance_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<coldhot_cancel_transafer_transaction_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<guard_refund_crosschain_trx_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<coldhot_cancel_uncombined_trx_operaion>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<coldhot_transfer_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<guard_update_multi_account_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<committee_member_update_global_parameters_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<contract_transfer_fee_proposal_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error.");
	   }
	   else if (op.op.which() == operation::tag<senator_determine_withdraw_deposit_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<senator_determine_block_payment_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::committee, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<block_address_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::cancel_commit, "Vote Type is error");
	   }
	   else if (op.op.which() == operation::tag<cancel_address_block_operation>::value)
	   {
		   FC_ASSERT(o.type == vote_id_type::cancel_commit, "Vote Type is error");
	   }
	   else
	   {
		   FC_CAPTURE_AND_THROW(proposal_create_invalid_proposals, (""));

	   }
      //FC_ASSERT( other.size() == 0 ); // TODO: what about other??? 

      if( auths.find(GRAPHENE_GUARD_ACCOUNT) != auths.end() )
      {
         GRAPHENE_ASSERT(
            o.review_period_seconds.valid(),
            proposal_create_review_period_required,
            "Review period not given, but at least ${min} required",
            ("min", global_parameters.committee_proposal_review_period)
         );
         GRAPHENE_ASSERT(
            *o.review_period_seconds >= global_parameters.committee_proposal_review_period,
            proposal_create_review_period_insufficient,
            "Review period of ${t} specified, but at least ${min} required",
            ("t", *o.review_period_seconds)
            ("min", global_parameters.committee_proposal_review_period)
         );
      }
   }
   //proposer has to be a formal guard
   auto  proposer = o.proposer;
   auto& guard_index = d.get_index_type<guard_member_index>().indices().get<by_account>();
   auto iter = guard_index.find(proposer);
   FC_ASSERT(iter != guard_index.end() && iter->formal == true, "propser has to be a formal guard.");
   if (senator_type_validate)
   {
	   FC_ASSERT(iter->senator_type == PERMANENT ,"only permanent senator can propose.");
	   for (const auto& proposal : proposal_idx)
	   {
		   for (const auto& op_p : proposal.proposed_transaction.operations)
		   {

			   FC_ASSERT(op_p.which() != operation::tag<guard_member_update_operation>::value, "there is other same operation for senator election.");
		   }
	   }
   }
   
   for (const op_wrapper& op : o.proposed_ops)
   {
	   _proposed_trx.operations.push_back(op.op);
	  
   }
   _proposed_trx.validate();
   transaction_evaluation_state eval_state(&db());
   //eval_state.operation_results.reserve(_proposed_trx.operations.size());
   //eval_state._trx = &processed_transaction(_proposed_trx);

   for (const auto& op : _proposed_trx.operations)
   {
	   unique_ptr<op_evaluator>& eval = db().get_evaluator(op);
	   eval->evaluate(eval_state, op, false);
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

object_id_type proposal_create_evaluator::do_apply(const proposal_create_operation& o)
{ try {
   database& d = db();

   const proposal_object& proposal = d.create<proposal_object>([&](proposal_object& proposal) {
      _proposed_trx.expiration = o.expiration_time;
      proposal.proposed_transaction = _proposed_trx;
      proposal.expiration_time = o.expiration_time;
      if( o.review_period_seconds.valid() )
         proposal.review_period_time = o.expiration_time - *o.review_period_seconds;
	  proposal.type = static_cast<vote_id_type::vote_type>(o.type);
      //Populate the required approval sets
      flat_set<account_id_type> required_active;
      vector<authority> other;
      
      // TODO: consider caching values from evaluate?
      for( auto& op : _proposed_trx.operations )
         operation_get_required_authorities(op, required_active, proposal.required_owner_approvals, other);
	  proposal.proposer = o.proposer;
      //proposal should only be approved by guard or miners
	  const auto& acc = d.get_index_type<account_index>().indices().get<by_id>();
	  if (o.type == vote_id_type::committee)
	  {
		  const auto& iter = d.get_index_type<guard_member_index>().indices().get<by_account>();
		  std::for_each(iter.begin(), iter.end(), [&](const guard_member_object& a)

		  {
			  if (a.formal)  //only formal guard can vote
				  proposal.required_account_approvals.insert(acc.find(a.guard_member_account)->addr);
		  });
	  }
	  else if (o.type == vote_id_type::cancel_commit)
	  {
		  const auto& iter = d.get_index_type<guard_member_index>().indices().get<by_account>();
		  std::for_each(iter.begin(), iter.end(), [&](const guard_member_object& a)

		  {
			  if (a.formal)  //only formal guard can vote
				  proposal.required_account_approvals.insert(acc.find(a.guard_member_account)->addr);
		  });
	  }
	  else
	  {
		  const auto& iter = d.get_index_type<miner_index>().indices().get<by_account>();
		  std::for_each(iter.begin(), iter.end(), [&](const miner_object& a)

		  {
			  proposal.required_account_approvals.insert(acc.find(a.miner_account)->addr);
		  });
	  }
   });

   return proposal.id;
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result proposal_update_evaluator::do_evaluate(const proposal_update_operation& o)
{ try {
   database& d = db();

   _proposal = &o.proposal(d);
   FC_ASSERT(_proposal->finished == false,"the proposal has not finished.");
   if( _proposal->review_period_time && d.head_block_time() >= *_proposal->review_period_time )
      FC_ASSERT( o.active_approvals_to_add.empty() && o.owner_approvals_to_add.empty(),
                 "This proposal is in its review period. No new approvals may be added." );

   for( account_id_type id : o.active_approvals_to_remove )
   {
      FC_ASSERT( _proposal->available_active_approvals.find(id) != _proposal->available_active_approvals.end(),
                 "", ("id", id)("available", _proposal->available_active_approvals) );
   }
   for( account_id_type id : o.owner_approvals_to_remove )
   {
      FC_ASSERT( _proposal->available_owner_approvals.find(id) != _proposal->available_owner_approvals.end(),
                 "", ("id", id)("available", _proposal->available_owner_approvals) );
   }
   /*  All authority checks happen outside of evaluators
   if( (d.get_node_properties().skip_flags & database::skip_authority_check) == 0 )
   {
      for( const auto& id : o.key_approvals_to_add )
      {
         FC_ASSERT( trx_state->signed_by(id) );
      }
      for( const auto& id : o.key_approvals_to_remove )
      {
         FC_ASSERT( trx_state->signed_by(id) );
      }
   }
   */

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result proposal_update_evaluator::do_apply(const proposal_update_operation& o)
{ try {
   database& d = db();

   // Potential optimization: if _executed_proposal is true, we can skip the modify step and make push_proposal skip
   // signature checks. This isn't done now because I just wrote all the proposals code, and I'm not yet 100% sure the
   // required approvals are sufficient to authorize the transaction.
  
   d.modify(*_proposal, [&o, &d](proposal_object& p) {
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
	   
	   for( const auto& addr : o.key_approvals_to_remove )
	   {
		   if (!is_miner_or_guard(addr))
			   continue;
		   p.disapproved_key_approvals.insert(addr);
		   p.approved_key_approvals.erase(addr);
	   }
        
   });

   // If the proposal has a review period, don't bother attempting to authorize/execute it.
   // Proposals with a review period may never be executed except at their expiration.
   if( _proposal->review_period_time )
      return void_result();

   if( _proposal->is_authorized_to_execute(d) )
   {
      // All required approvals are satisfied. Execute!
      _executed_proposal = true;
      try {
         _processed_transaction = d.push_proposal(*_proposal);
      } catch(fc::exception& e) {
         wlog("Proposed transaction ${id} failed to apply once approved with exception:\n----\n${reason}\n----\nWill try again when it expires.",
              ("id", o.proposal)("reason", e.to_detail_string()));
		 db().remove(*_proposal);
         _proposal_failed = true;
      }
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result proposal_delete_evaluator::do_evaluate(const proposal_delete_operation& o)
{ try {
   database& d = db();

   _proposal = &o.proposal(d);
   auto proposer_addr = d.get(_proposal->proposer).addr;
   // if no one has voted, i think we can remove this proposal
   // and definately only proposer can remove this proposal
   FC_ASSERT(proposer_addr == o.fee_paying_account,"not created by this account");
   FC_ASSERT(_proposal->approved_key_approvals.size()==0 && _proposal->disapproved_key_approvals.size()==0 , "as there are already votes, so it cannot be removed any more.");
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result proposal_delete_evaluator::do_apply(const proposal_delete_operation& o)
{ try {
   db().remove(*_proposal);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain
