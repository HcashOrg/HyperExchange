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
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/witness_object.hpp>
namespace graphene { namespace chain {

bool proposal_object::is_authorized_to_execute(database& db) const
{
   transaction_evaluation_state dry_run_eval(&db);

   try {
	   if (type == vote_id_type::committee)
		   return approved_key_approvals.size() >= size_t(std::ceil(double(required_account_approvals.size())*2/3));
	   auto& miner_idx = db.get_index_type<miner_index>().indices().get<by_account>();
	   auto& account_idx = db.get_index_type<account_index>().indices().get<by_address>();
	   uint64_t total_weights = 0;
	   uint64_t approved_key_weights = 0;
	   for (auto acc : required_account_approvals)
	   {
		   auto iter = miner_idx.find(account_idx.find(acc)->get_id());
		   total_weights += iter->pledge_weight.value;
	   }
	   
	   for (auto acc : approved_key_approvals)
	   {
		   auto iter = miner_idx.find(account_idx.find(acc)->get_id());
		   approved_key_weights += iter->pledge_weight.value;
	   }

	   return approved_key_weights >= uint64_t(std::ceil(double(total_weights) * 2 / 3));
   } 
   catch ( const fc::exception& e )
   {
      //idump((available_active_approvals));
      //wlog((e.to_detail_string()));
      return false;
   }
   return true;
}


void required_approval_index::object_inserted( const object& obj )
{
    assert( dynamic_cast<const proposal_object*>(&obj) );
    const proposal_object& p = static_cast<const proposal_object&>(obj);

    for( const auto& a : p.required_active_approvals )
       _account_to_proposals[a].insert( p.id );
    for( const auto& a : p.required_owner_approvals )
       _account_to_proposals[a].insert( p.id );
    for( const auto& a : p.available_active_approvals )
       _account_to_proposals[a].insert( p.id );
    for( const auto& a : p.available_owner_approvals )
       _account_to_proposals[a].insert( p.id );
}

void required_approval_index::remove( account_id_type a, proposal_id_type p )
{
    auto itr = _account_to_proposals.find(a);
    if( itr != _account_to_proposals.end() )
    {
        itr->second.erase( p );
        if( itr->second.empty() )
            _account_to_proposals.erase( itr->first );
    }
}

void required_approval_index::object_removed( const object& obj )
{
    assert( dynamic_cast<const proposal_object*>(&obj) );
    const proposal_object& p = static_cast<const proposal_object&>(obj);

    for( const auto& a : p.required_active_approvals )
       remove( a, p.id );
    for( const auto& a : p.required_owner_approvals )
       remove( a, p.id );
    for( const auto& a : p.available_active_approvals )
       remove( a, p.id );
    for( const auto& a : p.available_owner_approvals )
       remove( a, p.id );
}

} } // graphene::chain
