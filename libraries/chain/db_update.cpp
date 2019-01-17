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
#include <graphene/chain/db_with.hpp>

#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/witness_object.hpp>

#include <graphene/chain/protocol/fee_schedule.hpp>

#include <fc/uint128.hpp>

namespace graphene { namespace chain {

void database::update_global_dynamic_data( const signed_block& b )
{
   const dynamic_global_property_object& _dgp =
      dynamic_global_property_id_type(0)(*this);

   uint32_t missed_blocks = get_slot_at_time( b.timestamp );
   assert( missed_blocks != 0 );
   missed_blocks--;
   for( uint32_t i = 0; i < missed_blocks; ++i ) {
      const auto& witness_missed = get_scheduled_miner( i+1 )(*this);
      if(  witness_missed.id != b.miner ) {
         /*
         const auto& witness_account = witness_missed.miner_account(*this);
         if( (fc::time_point::now() - b.timestamp) < fc::seconds(30) )
            wlog( "Witness ${name} missed block ${n} around ${t}", ("name",witness_account.name)("n",b.block_num())("t",b.timestamp) );
            */

         modify( witness_missed, [&]( miner_object& w ) {
           w.total_missed++;
         });
      } 
   }
   
   // dynamic global properties updating
   modify( _dgp, [&]( dynamic_global_property_object& dgp ){
      if( BOOST_UNLIKELY( b.block_num() == 1 ) )
         dgp.recently_missed_count = 0;
         else if( _checkpoints.size() && _checkpoints.rbegin()->first >= b.block_num() )
         dgp.recently_missed_count = 0;
      else if( missed_blocks )
         dgp.recently_missed_count += GRAPHENE_RECENTLY_MISSED_COUNT_INCREMENT*missed_blocks;
      else if( dgp.recently_missed_count > GRAPHENE_RECENTLY_MISSED_COUNT_INCREMENT )
         dgp.recently_missed_count -= GRAPHENE_RECENTLY_MISSED_COUNT_DECREMENT;
      else if( dgp.recently_missed_count > 0 )
         dgp.recently_missed_count--;

      dgp.head_block_number = b.block_num();
      dgp.head_block_id = b.id();
      dgp.time = b.timestamp;
      dgp.current_witness = b.miner;
      dgp.recent_slots_filled = (
           (dgp.recent_slots_filled << 1)
           + 1) << missed_blocks;
      dgp.current_aslot += missed_blocks+1;
	  if (b.block_num() % GRAPHENE_PRODUCT_PER_ROUND == 0)
	  {
		  dgp.round_produced_miners.clear();
	  }
	  dgp.round_produced_miners.insert(b.miner);
   });

   if( !(get_node_properties().skip_flags & skip_undo_history_check) )
   {
      GRAPHENE_ASSERT( _dgp.head_block_number - _dgp.last_irreversible_block_num  < GRAPHENE_MAX_UNDO_HISTORY, undo_database_exception,
                 "The database does not have enough undo history to support a blockchain with so many missed blocks. "
                 "Please add a checkpoint if you would like to continue applying blocks beyond this point.",
                 ("last_irreversible_block_num",_dgp.last_irreversible_block_num)("head", _dgp.head_block_number)
                 ("recently_missed",_dgp.recently_missed_count)("max_undo",GRAPHENE_MAX_UNDO_HISTORY) );
   }

  // _undo_db.set_max_size( _dgp.head_block_number - _dgp.last_irreversible_block_num + 1 );
  // _fork_db.set_max_size( _dgp.head_block_number - _dgp.last_irreversible_block_num + 1 );
}

void database::update_signing_miner(const miner_object& signing_witness, const signed_block& new_block)
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t new_block_aslot = dpo.current_aslot + get_slot_at_time( new_block.timestamp );

   share_type miner_pay = std::min( gpo.parameters.miner_pay_per_block, dpo.miner_budget );

   /*  modify( dpo, [&]( dynamic_global_property_object& _dpo )
	 {
		_dpo.miner_budget -= miner_pay;
	 } );*/

   deposit_miner_pay( signing_witness, miner_pay );

   modify( signing_witness, [&]( miner_object& _wit )
   {
      _wit.last_aslot = new_block_aslot;
      _wit.last_confirmed_block_num = new_block.block_num();
	  _wit.total_produced++;
	  _wit.next_secret_hash = new_block.next_secret_hash;
   } );
}

void database::update_last_irreversible_block()
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   vector< const miner_object* > wit_objs;
   wit_objs.reserve( gpo.active_witnesses.size() );
   for( const miner_id_type& wid : gpo.active_witnesses )
      wit_objs.push_back( &(wid(*this)) );

   static_assert( GRAPHENE_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero" );

   // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
   // 1 1 1 1 1 1 1 2 2 2 -> 1
   // 3 3 3 3 3 3 3 3 3 3 -> 3

   size_t offset = ((GRAPHENE_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / GRAPHENE_100_PERCENT);
   std::sort( wit_objs.begin(), wit_objs.end(),
      []( const miner_object* a, const miner_object* b )
      {
         return a->last_confirmed_block_num < b->last_confirmed_block_num;
      } );
   //uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num ;
   int64_t new_last_irreversible_block_num = (int64_t)head_block_num() - _undo_db.size();
   if( new_last_irreversible_block_num > dpo.last_irreversible_block_num )
   {
      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
         _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
      } );
   }
}

vector<multisig_address_object> database::get_multi_account_senator(const string & multi_address, const string& symbol) const
{
	vector<multisig_address_object> result;
	auto& hot_db = get_index_type<multisig_account_pair_index>().indices().get<by_bindhot_chain_type>();
	auto& cold_db = get_index_type<multisig_account_pair_index>().indices().get<by_bindcold_chain_type>();
	auto hot_iterange = hot_db.equal_range(boost::make_tuple(multi_address, symbol));
	auto cold_iterange = cold_db.equal_range(boost::make_tuple(multi_address, symbol));
	multisig_account_pair_id_type multi_id;
	vector<multisig_account_pair_object> multisig_objs;
	if (hot_iterange.first != hot_iterange.second) {
		multisig_objs.assign(hot_iterange.first, hot_iterange.second);
		while (multisig_objs.size())
		{
			if (multisig_objs.back().effective_block_num)
			{
				multi_id = multisig_objs.rbegin()->id;
				break;
			}
				
			else
				multisig_objs.pop_back();
		}
	}
	if (cold_iterange.first != cold_iterange.second) {
		multisig_objs.assign(cold_iterange.first, cold_iterange.second);
		while (multisig_objs.size())
		{
			if (multisig_objs.back().effective_block_num)
			{
				multi_id = multisig_objs.rbegin()->id;
				break;
			}
			else
				multisig_objs.pop_back();
		}
	}
	FC_ASSERT(multi_id != multisig_account_pair_id_type(), "Can`t find coldhot multiaddress");
	auto guard_range = get_index_type<multisig_address_index>().indices().get<by_multisig_account_pair_id>().equal_range(multi_id);
	for (auto guard : boost::make_iterator_range(guard_range.first, guard_range.second))
	{
		result.push_back(guard);
	}
	return result;
}

void database::determine_referendum_detailes()
{
	try {
		/* when there are first referendum come up
		*  the referendum packing period will begin
		*  during referendum packing period, citizen can create new referendum
		*  but they cannot vote
		*  in other words,during vote period, a new referendum cannot be created
		*/
		const auto& dynamic_obj = get_dynamic_global_properties();
		if (dynamic_obj.referendum_flag)
		{
			auto& referendum_pleges = get_index_type<referendum_index>().indices().get<by_pledge>();
			if (referendum_pleges.size() > 0)
			{
				if (head_block_time() >= dynamic_obj.next_vote_time)
				{
					while (!(referendum_pleges.size() == 1))
					{
						auto& referendum = *referendum_pleges.begin();
						remove(referendum);
					}
				}
				if (referendum_pleges.rbegin()->finished == false)
					return;
			}
			//just like to modify the referendum flag to false
			// need to do some validations , for now only one need to be confirmed
			// have created new multisignatures
			auto senator_multisigs = [this] (const string symbol,account_id_type senator){
				vector<multisig_address_object> result;
				const auto& multisig_addr_by_guard = get_index_type<multisig_address_index>().indices().get<by_account_chain_type>();
				const auto iter_range = multisig_addr_by_guard.equal_range(boost::make_tuple(symbol, senator));
				std::for_each(iter_range.first, iter_range.second, [&](multisig_address_object obj) {
					result.emplace_back(obj);
				});
				return result;
			};
			auto all_senators_in = [this](const vector<multisig_address_object>& senator_multisignatures, vector<guard_member_object> senators) {
				if (senator_multisignatures.size() < senators.size())
					return false;
				while (senators.size() != 0)
				{
					auto senator = senators.back();
					senators.pop_back();
					bool found = false;
					for (auto multisig_address_obj : senator_multisignatures)
					{
						if (multisig_address_obj.guard_account == senator.guard_member_account)
						{
							found = true;
							break;
						}
					}
					if (!found)
						return false;
				}
				return true;
			};
			auto senators = get_guard_members();
			const auto& assets_by_symbol = get_index_type<asset_index>().indices().get<by_symbol>();
			for (const auto& itr : assets_by_symbol)
			{
				if (itr.get_id() == asset_id_type())
					continue;
				if (itr.symbol == "ETH" || itr.symbol.find("ERC") != string::npos)
					continue;
				auto multisig_account_pair = get_current_multisig_account(itr.symbol);
				if (!multisig_account_pair.valid())
				{
					continue;
				}
				auto ret = all_senators_in(get_multi_account_senator(multisig_account_pair->bind_account_hot,itr.symbol),senators);
				if (!ret)
				{
					return;
				}
			}
			modify(get(dynamic_global_property_id_type()), [&](dynamic_global_property_object& obj) {
				obj.referendum_flag = false;
			});
			while (!(referendum_pleges.size() == 0))
			{
				auto& referendum = *referendum_pleges.begin();
				remove(referendum);
			}
			return;
		}
		const auto& referendum_pleges = get_index_type<referendum_index>().indices().get<by_pledge>();
		if (referendum_pleges.size() > 0)
		{
			modify(get(dynamic_global_property_id_type()), [&]( dynamic_global_property_object& obj) {
				obj.referendum_flag = true;
				obj.next_vote_time = head_block_time() + fc::seconds(HX_REFERENDUM_PACKING_PERIOD);
			});
		}
	}FC_CAPTURE_AND_RETHROW()
}
void database::clear_expired_transactions()
{ try {
   //Look for expired transactions in the deduplication list, and remove them.
   //Transactions must have expired by at least two forking windows in order to be removed.
   auto& transaction_idx = static_cast<transaction_index&>(get_mutable_index(implementation_ids, impl_transaction_object_type));
   const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
   while( (!dedupe_index.empty()) && (head_block_time() > dedupe_index.begin()->trx.expiration) )
      transaction_idx.remove(*dedupe_index.begin());
} FC_CAPTURE_AND_RETHROW() }

bool database::_need_rollback(const proposal_object& proposal)
{
	try {
		FC_ASSERT(proposal.finished == true);
		auto senator_multisigs = [this](const string symbol, account_id_type senator) {
			vector<multisig_address_object> result;
			const auto& multisig_addr_by_guard = get_index_type<multisig_address_index>().indices().get<by_account_chain_type>();
			const auto iter_range = multisig_addr_by_guard.equal_range(boost::make_tuple(symbol, senator));
			std::for_each(iter_range.first, iter_range.second, [&](multisig_address_object obj) {
				result.emplace_back(obj);
			});
			return result;
		};
		auto all_senators_in = [this](const vector<multisig_address_object>& senator_multisignatures, vector<guard_member_object> senators) {
			if (senator_multisignatures.size() < senators.size())
				return false;
			while (senators.size() != 0)
			{
				auto senator = senators.back();
				senators.pop_back();
				bool found = false;
				for (auto multisig_address_obj : senator_multisignatures)
				{
					if (multisig_address_obj.guard_account == senator.guard_member_account)
					{
						found = true;
						break;
					}
				}
				if (!found)
					return false;
			}
			return true;
		};
		auto senators = get_guard_members();
		const auto& assets_by_symbol = get_index_type<asset_index>().indices().get<by_symbol>();
		for (const auto& itr : assets_by_symbol)
		{
			if (itr.get_id() == asset_id_type())
				continue;
			auto multisig_account_pair = get_current_multisig_account(itr.symbol);
			if (!multisig_account_pair.valid())
			{
				continue;
			}
			auto ret = all_senators_in(get_multi_account_senator(multisig_account_pair->bind_account_hot, itr.symbol), senators);
			if (!ret)
			{
				return true;
			}
		}
		return false;

	} FC_CAPTURE_AND_RETHROW()
}


void database::_rollback_votes(const proposal_object& proposal)
{
	try {
		if (!proposal.finished)
			return;
		bool need_return = true;
		guard_member_update_operation upop;
		for (auto op : proposal.proposed_transaction.operations)
		{
			if (op.which() == operation::tag<guard_member_update_operation>().value)
			{
				upop = op.get<guard_member_update_operation>();
				need_return = false;
				break;
			}
				
		}
		if (need_return)
			return;
		auto ret = _need_rollback(proposal);
		if (ret)
		{
			auto& senator_idx = get_index_type<guard_member_index>().indices().get<by_account>();
			for (auto itr : upop.replace_queue)
			{
				modify(*senator_idx.find(itr.first), [](guard_member_object& obj) {
					obj.formal = false;
				});
				modify(*senator_idx.find(itr.second), [](guard_member_object& obj) {
					obj.formal = true;
				});
			}
		}
	} FC_CAPTURE_AND_RETHROW()
}

void database::clear_expired_proposals()
{
   const auto& proposal_expiration_index = get_index_type<proposal_index>().indices().get<by_expiration>();
   while( !proposal_expiration_index.empty() && proposal_expiration_index.begin()->expiration_time <= head_block_time() )
   {
      const proposal_object& proposal = *proposal_expiration_index.begin();
      processed_transaction result;
      try {
         if( proposal.is_authorized_to_execute(*this) )
         {
            result = push_proposal(proposal);
            //TODO: Do something with result so plugins can process it.
            continue;
         }
      } catch( const fc::exception& e ) {
         elog("Failed to apply proposed transaction on its expiration. Deleting it.\n${proposal}\n${error}",
              ("proposal", proposal)("error", e.to_detail_string()));
      }
	  if (proposal.finished == true)
		  _rollback_votes(proposal);
      remove(proposal);
   }
   const auto& proposal_finished_index = get_index_type<proposal_index>().indices().get<by_finished>();
   auto range_finish = proposal_finished_index.equal_range(true);
   bool remove_finished_proposal = false;
   for (const auto& itr : boost::make_iterator_range(range_finish.first, range_finish.second))
   {
	   if (itr.finished == false)
		   continue;
	   for (auto op : itr.proposed_transaction.operations)
	   {
		   if (op.which() == operation::tag<guard_member_update_operation>().value)
		   {
			   if (!_need_rollback(itr))
			   {
				   remove_finished_proposal = true;
				   break;
			   }
				   
		   }
	   }
	   if (remove_finished_proposal)
	   {
		   remove(itr);
		   break;
	   }
   }
   const auto& referedum_expiration_index = get_index_type<referendum_index>().indices().get<by_expiration>();
   while (!referedum_expiration_index.empty() && referedum_expiration_index.begin()->expiration_time <= head_block_time())
   {
	   const referendum_object& referedum = *referedum_expiration_index.begin();
	   processed_transaction result;
	   try {
		   if (referedum.is_authorized_to_execute(*this))
		   {
			   result = push_referendum(referedum);
			   continue;
		   }
		   else
		   {
			   if (referedum.finished == true)
			   {
				   for (const auto & op : referedum.proposed_transaction.operations)
				   {
					   if (op.which() == operation::tag<citizen_referendum_senator_operation>::value)
					   {
						   auto senator_op = op.get<citizen_referendum_senator_operation>();
						   auto& senator_idx = get_index_type<guard_member_index>().indices().get<by_account>();
						   for (auto itr : senator_op.replace_queue)
						   {
							   modify(*senator_idx.find(itr.first), [](guard_member_object& obj) {
								   obj.formal = false;
							   });
							   modify(*senator_idx.find(itr.second), [](guard_member_object& obj) {
								   obj.formal = true;
							   });
						   }
					   }
				   }
			   }
		   }
	   }
	   catch (const fc::exception& e) {
		   elog("Failed to apply proposed transaction on its expiration. Deleting it.\n${proposal}\n${error}",
			   ("referedum", referedum)("error", e.to_detail_string()));
	   }
	   remove(referedum);
   }

}

/**
 *  let HB = the highest bid for the collateral  (aka who will pay the most DEBT for the least collateral)
 *  let SP = current median feed's Settlement Price 
 *  let LC = the least collateralized call order's swan price (debt/collateral)
 *
 *  If there is no valid price feed or no bids then there is no black swan.
 *
 *  A black swan occurs if MAX(HB,SP) <= LC
 */
bool database::check_for_blackswan( const asset_object& mia, bool enable_black_swan )
{
    if( !mia.is_market_issued() ) return false;

    const asset_bitasset_data_object& bitasset = mia.bitasset_data(*this);
    if( bitasset.has_settlement() ) return true; // already force settled
    auto settle_price = bitasset.current_feed.settlement_price;
    if( settle_price.is_null() ) return false; // no feed

    const call_order_index& call_index = get_index_type<call_order_index>();
    const auto& call_price_index = call_index.indices().get<by_price>();

    const limit_order_index& limit_index = get_index_type<limit_order_index>();
    const auto& limit_price_index = limit_index.indices().get<by_price>();

    // looking for limit orders selling the most USD for the least CORE
    auto highest_possible_bid = price::max( mia.id, bitasset.options.short_backing_asset );
    // stop when limit orders are selling too little USD for too much CORE
    auto lowest_possible_bid  = price::min( mia.id, bitasset.options.short_backing_asset );

    assert( highest_possible_bid.base.asset_id == lowest_possible_bid.base.asset_id );
    // NOTE limit_price_index is sorted from greatest to least
    auto limit_itr = limit_price_index.lower_bound( highest_possible_bid );
    auto limit_end = limit_price_index.upper_bound( lowest_possible_bid );

    auto call_min = price::min( bitasset.options.short_backing_asset, mia.id );
    auto call_max = price::max( bitasset.options.short_backing_asset, mia.id );
    auto call_itr = call_price_index.lower_bound( call_min );
    auto call_end = call_price_index.upper_bound( call_max );

    if( call_itr == call_end ) return false;  // no call orders

    price highest = settle_price;
    if( limit_itr != limit_end ) {
       assert( settle_price.base.asset_id == limit_itr->sell_price.base.asset_id );
       highest = std::max( limit_itr->sell_price, settle_price );
    }

    auto least_collateral = call_itr->collateralization();
    if( ~least_collateral >= highest  ) 
    {
       elog( "Black Swan detected: \n"
             "   Least collateralized call: ${lc}  ${~lc}\n"
           //  "   Highest Bid:               ${hb}  ${~hb}\n"
             "   Settle Price:              ${sp}  ${~sp}\n"
             "   Max:                       ${h}   ${~h}\n",
            ("lc",least_collateral.to_real())("~lc",(~least_collateral).to_real())
          //  ("hb",limit_itr->sell_price.to_real())("~hb",(~limit_itr->sell_price).to_real())
            ("sp",settle_price.to_real())("~sp",(~settle_price).to_real())
            ("h",highest.to_real())("~h",(~highest).to_real()) );
       FC_ASSERT( enable_black_swan, "Black swan was detected during a margin update which is not allowed to trigger a blackswan" );
       globally_settle_asset(mia, ~least_collateral );
       return true;
    } 
    return false;
}

void database::clear_expired_orders()
{ try {
   detail::with_skip_flags( *this,
      get_node_properties().skip_flags | skip_authority_check, [&](){
         transaction_evaluation_state cancel_context(this);

         //Cancel expired limit orders
         auto& limit_index = get_index_type<limit_order_index>().indices().get<by_expiration>();
         while( !limit_index.empty() && limit_index.begin()->expiration <= head_block_time() )
         {
            limit_order_cancel_operation canceler;
            const limit_order_object& order = *limit_index.begin();
            canceler.fee_paying_account = order.seller;
            canceler.order = order.id;
            canceler.fee = current_fee_schedule().calculate_fee( canceler );
            if( canceler.fee.amount > order.deferred_fee )
            {
               // Cap auto-cancel fees at deferred_fee; see #549
               wlog( "At block ${b}, fee for clearing expired order ${oid} was capped at deferred_fee ${fee}", ("b", head_block_num())("oid", order.id)("fee", order.deferred_fee) );
               canceler.fee = asset( order.deferred_fee, asset_id_type() );
            }
            // we know the fee for this op is set correctly since it is set by the chain.
            // this allows us to avoid a hung chain:
            // - if #549 case above triggers
            // - if the fee is incorrect, which may happen due to #435 (although since cancel is a fixed-fee op, it shouldn't)
            cancel_context.skip_fee_schedule_check = true;
            apply_operation(cancel_context, canceler);
         }
     });

   //Process expired force settlement orders
   auto& settlement_index = get_index_type<force_settlement_index>().indices().get<by_expiration>();
   if( !settlement_index.empty() )
   {
      asset_id_type current_asset = settlement_index.begin()->settlement_asset_id();
      asset max_settlement_volume;
      bool extra_dump = false;

      auto next_asset = [&current_asset, &settlement_index, &extra_dump] {
         auto bound = settlement_index.upper_bound(current_asset);
         if( bound == settlement_index.end() )
         {
            if( extra_dump )
            {
               ilog( "next_asset() returning false" );
            }
            return false;
         }
         if( extra_dump )
         {
            ilog( "next_asset returning true, bound is ${b}", ("b", *bound) );
         }
         current_asset = bound->settlement_asset_id();
         return true;
      };

      uint32_t count = 0;

      // At each iteration, we either consume the current order and remove it, or we move to the next asset
      for( auto itr = settlement_index.lower_bound(current_asset);
           itr != settlement_index.end();
           itr = settlement_index.lower_bound(current_asset) )
      {
         ++count;
         const force_settlement_object& order = *itr;
         auto order_id = order.id;
         current_asset = order.settlement_asset_id();
         const asset_object& mia_object = get(current_asset);
         const asset_bitasset_data_object& mia = mia_object.bitasset_data(*this);

         extra_dump = ((count >= 1000) && (count <= 1020));

         if( extra_dump )
         {
            wlog( "clear_expired_orders() dumping extra data for iteration ${c}", ("c", count) );
            ilog( "head_block_num is ${hb} current_asset is ${a}", ("hb", head_block_num())("a", current_asset) );
         }

         if( mia.has_settlement() )
         {
            ilog( "Canceling a force settlement because of black swan" );
            cancel_order( order );
            continue;
         }

         // Has this order not reached its settlement date?
         if( order.settlement_date > head_block_time() )
         {
            if( next_asset() )
            {
               if( extra_dump )
               {
                  ilog( "next_asset() returned true when order.settlement_date > head_block_time()" );
               }
               continue;
            }
            break;
         }
         // Can we still settle in this asset?
         if( mia.current_feed.settlement_price.is_null() )
         {
            ilog("Canceling a force settlement in ${asset} because settlement price is null",
                 ("asset", mia_object.symbol));
            cancel_order(order);
            continue;
         }
         if( max_settlement_volume.asset_id != current_asset )
            max_settlement_volume = mia_object.amount(mia.max_force_settlement_volume(mia_object.dynamic_data(*this).current_supply));
         if( mia.force_settled_volume >= max_settlement_volume.amount )
         {
            /*
            ilog("Skipping force settlement in ${asset}; settled ${settled_volume} / ${max_volume}",
                 ("asset", mia_object.symbol)("settlement_price_null",mia.current_feed.settlement_price.is_null())
                 ("settled_volume", mia.force_settled_volume)("max_volume", max_settlement_volume));
                 */
            if( next_asset() )
            {
               if( extra_dump )
               {
                  ilog( "next_asset() returned true when mia.force_settled_volume >= max_settlement_volume.amount" );
               }
               continue;
            }
            break;
         }

         auto& pays = order.balance;
         auto receives = (order.balance * mia.current_feed.settlement_price);
         receives.amount = (fc::uint128_t(receives.amount.value) *
                            (GRAPHENE_100_PERCENT - mia.options.force_settlement_offset_percent) / GRAPHENE_100_PERCENT).to_uint64();
         assert(receives <= order.balance * mia.current_feed.settlement_price);

         price settlement_price = pays / receives;

         auto& call_index = get_index_type<call_order_index>().indices().get<by_collateral>();
         asset settled = mia_object.amount(mia.force_settled_volume);
         // Match against the least collateralized short until the settlement is finished or we reach max settlements
         while( settled < max_settlement_volume && find_object(order_id) )
         {
            auto itr = call_index.lower_bound(boost::make_tuple(price::min(mia_object.bitasset_data(*this).options.short_backing_asset,
                                                                           mia_object.get_id())));
            // There should always be a call order, since asset exists!
            assert(itr != call_index.end() && itr->debt_type() == mia_object.get_id());
            asset max_settlement = max_settlement_volume - settled;

            if( order.balance.amount == 0 )
            {
               wlog( "0 settlement detected" );
               cancel_order( order );
               break;
            }
            try {
               settled += match(*itr, order, settlement_price, max_settlement);
            } 
            catch ( const black_swan_exception& e ) { 
               wlog( "black swan detected: ${e}", ("e", e.to_detail_string() ) );
               cancel_order( order );
               break;
            }
         }
         if( mia.force_settled_volume != settled.amount )
         {
            modify(mia, [settled](asset_bitasset_data_object& b) {
               b.force_settled_volume = settled.amount;
            });
         }
      }
   }
} FC_CAPTURE_AND_RETHROW() }

void database::update_expired_feeds()
{
   auto& asset_idx = get_index_type<asset_index>().indices().get<by_type>();
   auto itr = asset_idx.lower_bound( true /** market issued */ );
   while( itr != asset_idx.end() )
   {
      const asset_object& a = *itr;
      ++itr;
      assert( a.is_market_issued() );

      const asset_bitasset_data_object& b = a.bitasset_data(*this);
      bool feed_is_expired;
      feed_is_expired = b.feed_is_expired( head_block_time() );
      if( feed_is_expired )
      {
         modify(b, [this](asset_bitasset_data_object& a) {
            a.update_median_feeds(head_block_time());
         });
         check_call_orders(b.current_feed.settlement_price.base.asset_id(*this));
      }
      if( !b.current_feed.core_exchange_rate.is_null() &&
          a.options.core_exchange_rate != b.current_feed.core_exchange_rate )
         modify(a, [&b](asset_object& a) {
            a.options.core_exchange_rate = b.current_feed.core_exchange_rate;
         });
   }
}

void database::update_maintenance_flag( bool new_maintenance_flag )
{
   modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& dpo )
   {
      auto maintenance_flag = dynamic_global_property_object::maintenance_flag;
      dpo.dynamic_flags =
           (dpo.dynamic_flags & ~maintenance_flag)
         | (new_maintenance_flag ? maintenance_flag : 0);
   } );
   return;
}

void database::update_withdraw_permissions()
{
   auto& permit_index = get_index_type<withdraw_permission_index>().indices().get<by_expiration>();
   while( !permit_index.empty() && permit_index.begin()->expiration <= head_block_time() )
      remove(*permit_index.begin());
}

} }
