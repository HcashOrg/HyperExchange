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

   for (const op_wrapper& op : o.proposed_ops)
   {
	   _proposed_trx.operations.push_back(op.op);
	   evaluate(op.op);
   }
   _proposed_trx.validate();
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

object_id_type referendum_create_evaluator::do_apply(const referendum_create_operation& o)
{ try {
   database& d = db();
   const auto& ref_obj= d.create<referendum_object>([&](referendum_object& referendum) {
	   //_proposed_trx.expiration = o.expiration_time;
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
   });

   return ref_obj.id;
} FC_CAPTURE_AND_RETHROW( (o) ) }


} } // graphene::chain
