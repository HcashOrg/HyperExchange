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
#include <graphene/chain/asset_evaluator.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/balance_object.hpp>
#include <functional>

namespace graphene { namespace chain {

void_result asset_create_evaluator::do_evaluate( const asset_create_operation& op )
{ try {

   database& d = db();

   const auto& chain_parameters = d.get_global_properties().parameters;
   FC_ASSERT( op.common_options.whitelist_authorities.size() <= chain_parameters.maximum_asset_whitelist_authorities );
   FC_ASSERT( op.common_options.blacklist_authorities.size() <= chain_parameters.maximum_asset_whitelist_authorities );

   // Check that all authorities do exist
   for( auto id : op.common_options.whitelist_authorities )
      d.get_object(id);
   for( auto id : op.common_options.blacklist_authorities )
      d.get_object(id);

   auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
   auto asset_symbol_itr = asset_indx.find( op.symbol );
   FC_ASSERT( asset_symbol_itr == asset_indx.end() );

   auto dotpos = op.symbol.find( '.' );
   if( dotpos != std::string::npos )
	   wlog( "Asset ${s} has a name which requires hardfork 385", ("s",op.symbol) );

   core_fee_paid -= core_fee_paid.value/2;

   if( op.bitasset_opts )
   {
      const asset_object& backing = op.bitasset_opts->short_backing_asset(d);
      if( backing.is_market_issued() )
      {
         const asset_bitasset_data_object& backing_bitasset_data = backing.bitasset_data(d);
         const asset_object& backing_backing = backing_bitasset_data.options.short_backing_asset(d);
         FC_ASSERT( !backing_backing.is_market_issued(),
                    "May not create a bitasset backed by a bitasset backed by a bitasset." );
         FC_ASSERT( op.issuer != GRAPHENE_GUARD_ACCOUNT || backing_backing.get_id() == asset_id_type(),
                    "May not create a blockchain-controlled market asset which is not backed by CORE.");
      } else
         FC_ASSERT( op.issuer != GRAPHENE_GUARD_ACCOUNT || backing.get_id() == asset_id_type(),
                    "May not create a blockchain-controlled market asset which is not backed by CORE.");
      FC_ASSERT( op.bitasset_opts->feed_lifetime_sec > chain_parameters.block_interval &&
                 op.bitasset_opts->force_settlement_delay_sec > chain_parameters.block_interval );
   }
   if( op.is_prediction_market )
   {
      FC_ASSERT( op.bitasset_opts );
      FC_ASSERT( op.precision == op.bitasset_opts->short_backing_asset(d).precision );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type asset_create_evaluator::do_apply( const asset_create_operation& op )
{ try {
   const asset_dynamic_data_object& dyn_asset =
      db().create<asset_dynamic_data_object>( [&]( asset_dynamic_data_object& a ) {
         a.current_supply = 0;
         a.fee_pool = core_fee_paid; //op.calculate_fee(db().current_fee_schedule()).value / 2;
      });

   asset_bitasset_data_id_type bit_asset_id;
   if( op.bitasset_opts.valid() )
      bit_asset_id = db().create<asset_bitasset_data_object>( [&]( asset_bitasset_data_object& a ) {
            a.options = *op.bitasset_opts;
            a.is_prediction_market = op.is_prediction_market;
         }).id;

   auto next_asset_id = db().get_index_type<asset_index>().get_next_id();

   const asset_object& new_asset =
     db().create<asset_object>( [&]( asset_object& a ) {
         a.issuer = op.issuer;
         a.symbol = op.symbol;
         a.precision = op.precision;
         a.options = op.common_options;
         if( a.options.core_exchange_rate.base.asset_id.instance.value == 0 )
            a.options.core_exchange_rate.quote.asset_id = next_asset_id;
         else
            a.options.core_exchange_rate.base.asset_id = next_asset_id;
         a.dynamic_asset_data_id = dyn_asset.id;
         if( op.bitasset_opts.valid() )
            a.bitasset_data_id = bit_asset_id;
      });
   assert( new_asset.id == next_asset_id );

   return new_asset.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result asset_issue_evaluator::do_evaluate( const asset_issue_operation& o )
{ try {
   const database& d = db();

   const asset_object& a = o.asset_to_issue.asset_id(d);
   FC_ASSERT( o.issuer == a.issuer );
   FC_ASSERT( !a.is_market_issued(), "Cannot manually issue a market-issued asset." );

   to_account = &o.issue_to_account(d);
   FC_ASSERT( is_authorized_asset( d, *to_account, a ) );

   asset_dyn_data = &a.dynamic_asset_data_id(d);
   FC_ASSERT( (asset_dyn_data->current_supply + o.asset_to_issue.amount) <= a.options.max_supply );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_issue_evaluator::do_apply( const asset_issue_operation& o )
{ try {
   //use this issue to generate new asset in balanceOject
   auto& id_idx = db().get_index_type<account_index>().indices().get<by_id>();
   auto acc = *id_idx.find(o.issue_to_account);

   db().adjust_balance( acc.addr, o.asset_to_issue );
   db().modify( *asset_dyn_data, [&]( asset_dynamic_data_object& data ){
        data.current_supply += o.asset_to_issue.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_reserve_evaluator::do_evaluate( const asset_reserve_operation& o )
{ try {
   const database& d = db();

   const asset_object& a = o.amount_to_reserve.asset_id(d);
   GRAPHENE_ASSERT(
      !a.is_market_issued(),
      asset_reserve_invalid_on_mia,
      "Cannot reserve ${sym} because it is a market-issued asset",
      ("sym", a.symbol)
   );

   from_account = &o.payer(d);
   FC_ASSERT( is_authorized_asset( d, *from_account, a ) );

   asset_dyn_data = &a.dynamic_asset_data_id(d);
   FC_ASSERT( (asset_dyn_data->current_supply - o.amount_to_reserve.amount) >= 0 );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_reserve_evaluator::do_apply( const asset_reserve_operation& o )
{ try {
   db().adjust_balance( o.payer, -o.amount_to_reserve );

   db().modify( *asset_dyn_data, [&]( asset_dynamic_data_object& data ){
        data.current_supply -= o.amount_to_reserve.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_fund_fee_pool_evaluator::do_evaluate(const asset_fund_fee_pool_operation& o)
{ try {
   database& d = db();

   const asset_object& a = o.asset_id(d);

   asset_dyn_data = &a.dynamic_asset_data_id(d);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_fund_fee_pool_evaluator::do_apply(const asset_fund_fee_pool_operation& o)
{ try {
   db().adjust_balance(o.from_account, -o.amount);

   db().modify( *asset_dyn_data, [&]( asset_dynamic_data_object& data ) {
      data.fee_pool += o.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_evaluator::do_evaluate(const asset_update_operation& o)
{ try {
   database& d = db();
   const asset_object& a = o.asset_to_update(d);
   FC_ASSERT(d.get(a.issuer).addr == o.issuer ,"must be same issuer.");
   return void_result();
} FC_CAPTURE_AND_RETHROW((o)) }

void_result asset_update_evaluator::do_apply(const asset_update_operation& o)
{ try {
   database& d = db();
   const asset_object& a = o.asset_to_update(d);
   d.modify(a, [&](asset_object& obj) {
	   obj.options.description = o.description;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_bitasset_evaluator::do_evaluate(const asset_update_bitasset_operation& o)
{ try {
   database& d = db();

   const asset_object& a = o.asset_to_update(d);

   FC_ASSERT(a.is_market_issued(), "Cannot update BitAsset-specific settings on a non-BitAsset.");

   const asset_bitasset_data_object& b = a.bitasset_data(d);
   FC_ASSERT( !b.has_settlement(), "Cannot update a bitasset after a settlement has executed" );
   if( o.new_options.short_backing_asset != b.options.short_backing_asset )
   {
      FC_ASSERT(a.dynamic_asset_data_id(d).current_supply == 0);
      FC_ASSERT(d.find_object(o.new_options.short_backing_asset));

      if( a.issuer == GRAPHENE_GUARD_ACCOUNT )
      {
         const asset_object& backing = a.bitasset_data(d).options.short_backing_asset(d);
         if( backing.is_market_issued() )
         {
            const asset_object& backing_backing = backing.bitasset_data(d).options.short_backing_asset(d);
            FC_ASSERT( backing_backing.get_id() == asset_id_type(),
                       "May not create a blockchain-controlled market asset which is not backed by CORE.");
         } else
            FC_ASSERT( backing.get_id() == asset_id_type(),
                       "May not create a blockchain-controlled market asset which is not backed by CORE.");
      }
   }

   bitasset_to_update = &b;
   FC_ASSERT( o.issuer == a.issuer, "", ("o.issuer", o.issuer)("a.issuer", a.issuer) );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_bitasset_evaluator::do_apply(const asset_update_bitasset_operation& o)
{ try {
   bool should_update_feeds = false;
   // If the minimum number of feeds to calculate a median has changed, we need to recalculate the median
   if( o.new_options.minimum_feeds != bitasset_to_update->options.minimum_feeds )
      should_update_feeds = true;

   db().modify(*bitasset_to_update, [&](asset_bitasset_data_object& b) {
      b.options = o.new_options;

      if( should_update_feeds )
         b.update_median_feeds(db().head_block_time());
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_feed_producers_evaluator::do_evaluate(const asset_update_feed_producers_evaluator::operation_type& o)
{ try {
   database& d = db();

   FC_ASSERT( o.new_feed_producers.size() <= d.get_global_properties().parameters.maximum_asset_feed_publishers );
   for( auto id : o.new_feed_producers )
      d.get_object(id);

   const asset_object& a = o.asset_to_update(d);

   FC_ASSERT(a.is_market_issued(), "Cannot update feed producers on a non-BitAsset.");
   FC_ASSERT(!(a.options.flags & committee_fed_asset), "Cannot set feed producers on a committee-fed asset.");
   FC_ASSERT(!(a.options.flags & witness_fed_asset), "Cannot set feed producers on a witness-fed asset.");

   const asset_bitasset_data_object& b = a.bitasset_data(d);
   bitasset_to_update = &b;
   FC_ASSERT( a.issuer == o.issuer );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_feed_producers_evaluator::do_apply(const asset_update_feed_producers_evaluator::operation_type& o)
{ try {
   db().modify(*bitasset_to_update, [&](asset_bitasset_data_object& a) {
      //This is tricky because I have a set of publishers coming in, but a map of publisher to feed is stored.
      //I need to update the map such that the keys match the new publishers, but not munge the old price feeds from
      //publishers who are being kept.
      //First, remove any old publishers who are no longer publishers
      for( auto itr = a.feeds.begin(); itr != a.feeds.end(); )
      {
         if( !o.new_feed_producers.count(itr->first) )
            itr = a.feeds.erase(itr);
         else
            ++itr;
      }
      //Now, add any new publishers
      for( auto itr = o.new_feed_producers.begin(); itr != o.new_feed_producers.end(); ++itr )
         if( !a.feeds.count(*itr) )
            a.feeds[*itr];
      a.update_median_feeds(db().head_block_time());
   });
   db().check_call_orders( o.asset_to_update(db()) );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_global_settle_evaluator::do_evaluate(const asset_global_settle_evaluator::operation_type& op)
{ try {
   const database& d = db();
   asset_to_settle = &op.asset_to_settle(d);
   FC_ASSERT(asset_to_settle->is_market_issued());
   FC_ASSERT(asset_to_settle->can_global_settle());
   FC_ASSERT(asset_to_settle->issuer == op.issuer );
   FC_ASSERT(asset_to_settle->dynamic_data(d).current_supply > 0);
   const auto& idx = d.get_index_type<call_order_index>().indices().get<by_collateral>();
   assert( !idx.empty() );
   auto itr = idx.lower_bound(boost::make_tuple(price::min(asset_to_settle->bitasset_data(d).options.short_backing_asset,
                                                           op.asset_to_settle)));
   assert( itr != idx.end() && itr->debt_type() == op.asset_to_settle );
   const call_order_object& least_collateralized_short = *itr;
   FC_ASSERT(least_collateralized_short.get_debt() * op.settle_price <= least_collateralized_short.get_collateral(),
             "Cannot force settle at supplied price: least collateralized short lacks sufficient collateral to settle.");

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result asset_global_settle_evaluator::do_apply(const asset_global_settle_evaluator::operation_type& op)
{ try {
   database& d = db();
   d.globally_settle_asset( op.asset_to_settle(db()), op.settle_price );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result asset_settle_evaluator::do_evaluate(const asset_settle_evaluator::operation_type& op)
{ try {
   const database& d = db();
   asset_to_settle = &op.amount.asset_id(d);
   FC_ASSERT(asset_to_settle->is_market_issued());
   const auto& bitasset = asset_to_settle->bitasset_data(d);
   FC_ASSERT(asset_to_settle->can_force_settle() || bitasset.has_settlement() );
   if( bitasset.is_prediction_market )
      FC_ASSERT( bitasset.has_settlement(), "global settlement must occur before force settling a prediction market"  );
   else if( bitasset.current_feed.settlement_price.is_null() )
      FC_THROW_EXCEPTION(insufficient_feeds, "Cannot force settle with no price feed.");
   FC_ASSERT(d.get_balance(d.get(op.account), *asset_to_settle) >= op.amount);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result asset_settle_evaluator::do_apply(const asset_settle_evaluator::operation_type& op)
{ try {
   database& d = db();
   d.adjust_balance(op.account, -op.amount);

   const auto& bitasset = asset_to_settle->bitasset_data(d);
   if( bitasset.has_settlement() )
   {
      auto settled_amount = op.amount * bitasset.settlement_price;
      FC_ASSERT( settled_amount.amount <= bitasset.settlement_fund );

      d.modify( bitasset, [&]( asset_bitasset_data_object& obj ){
                obj.settlement_fund -= settled_amount.amount;
                });

      d.adjust_balance(op.account, settled_amount);

      const auto& mia_dyn = asset_to_settle->dynamic_asset_data_id(d);

      d.modify( mia_dyn, [&]( asset_dynamic_data_object& obj ){
                obj.current_supply -= op.amount.amount;
                });

      return settled_amount;
   }
   else
   {
      return d.create<force_settlement_object>([&](force_settlement_object& s) {
         s.owner = op.account;
         s.balance = op.amount;
         s.settlement_date = d.head_block_time() + asset_to_settle->bitasset_data(d).options.force_settlement_delay_sec;
      }).id;
   }
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result asset_publish_feeds_evaluator::do_evaluate(const asset_publish_feed_operation& o)
{ try {
   database& d = db();

   const asset_object& base = o.asset_id(d);
   //Verify that this feed is for a market-issued asset and that asset is backed by the base
   auto publisher = d.get(o.publisher);
   FC_ASSERT(publisher.addr != address());
   FC_ASSERT(base.publishers.count(publisher.addr) >=0);
   return void_result();
} FC_CAPTURE_AND_RETHROW((o)) }

void_result asset_publish_feeds_evaluator::do_apply(const asset_publish_feed_operation& o)
{ try {

   database& d = db();

   const asset_object& base = o.asset_id(d);
   const asset_bitasset_data_object& bad = base.bitasset_data(d);

   auto old_feed =  bad.current_feed;
   // Store medians for this asset
   d.modify(bad , [&o,&d](asset_bitasset_data_object& a) {
      a.feeds[o.publisher] = make_pair(d.head_block_time(), o.feed);
      a.update_median_feeds(d.head_block_time());
   });

   if( !(old_feed == bad.current_feed) )
      db().check_call_orders(base);

   return void_result();
} FC_CAPTURE_AND_RETHROW((o)) }



void_result normal_asset_publish_feeds_evaluator::do_evaluate(const normal_asset_publish_feed_operation& o)
{
	try {
		database& d = db();
		
		const asset_object& base = o.asset_id(d);
		//Verify that this feed is for a market-issued asset and that asset is backed by the base

		FC_ASSERT(o.feed.settlement_price.quote.asset_id == asset_id_type(0));

		if (!o.feed.core_exchange_rate.is_null())
		{
			FC_ASSERT(o.feed.core_exchange_rate.quote.asset_id == asset_id_type());
		}
// 		if (!o.feed.core_exchange_rate.is_null())
// 		{
// 			FC_ASSERT(o.feed.core_exchange_rate.quote.asset_id == asset_id_type());
// 		}
		FC_ASSERT(base.publishers.count(o.publisher_addr)>0);
		return void_result();
	} FC_CAPTURE_AND_RETHROW((o))
}

void_result normal_asset_publish_feeds_evaluator::do_apply(const normal_asset_publish_feed_operation& o)
{
	try {

		database& d = db();

		const asset_object& base = o.asset_id(d);

		auto old_feed = base.current_feed;
		// Store medians for this asset
		d.modify(base, [&o, &d](asset_object& a) {
			a.feeds[o.publisher_addr] = make_pair(d.head_block_time(), o.feed);
			a.update_median_feeds(d.head_block_time());
		});

		return void_result();
	} FC_CAPTURE_AND_RETHROW((o))
}

bool normal_asset_publish_feeds_evaluator::if_evluate()
{
	return true;
}

void_result asset_claim_fees_evaluator::do_evaluate( const asset_claim_fees_operation& o )
{ try {
   FC_ASSERT( o.amount_to_claim.asset_id(db()).issuer == o.issuer, "Asset fees may only be claimed by the issuer" );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }


void_result asset_claim_fees_evaluator::do_apply( const asset_claim_fees_operation& o )
{ try {
   database& d = db();

   const asset_object& a = o.amount_to_claim.asset_id(d);
   const asset_dynamic_data_object& addo = a.dynamic_asset_data_id(d);
   FC_ASSERT( o.amount_to_claim.amount <= addo.accumulated_fees, "Attempt to claim more fees than have accumulated", ("addo",addo) );

   d.modify( addo, [&]( asset_dynamic_data_object& _addo  ) {
     _addo.accumulated_fees -= o.amount_to_claim.amount;
   });

   d.adjust_balance( o.issuer, o.amount_to_claim );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }


void_result asset_real_create_evaluator::do_evaluate(const asset_real_create_operation& o)
{
	try {
		const database& d = db();
		auto id = o.issuer;
		const auto& guards = d.get_index_type<guard_member_index>().indices().get<by_account>();
		FC_ASSERT(guards.find(o.issuer) != guards.end());
		FC_ASSERT(guards.find(o.issuer)->formal == true); // only formal guard can create asset.
		FC_ASSERT(o.max_supply <= GRAPHENE_MAX_SHARE_SUPPLY);
		const auto& accounts = d.get_index_type<account_index>().indices().get<by_id>();
		FC_ASSERT(accounts.find(o.issuer)->addr == o.issuer_addr);
		auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		auto asset_symbol_itr = asset_indx.find(o.symbol);
		FC_ASSERT(asset_symbol_itr == asset_indx.end());

	}FC_CAPTURE_AND_RETHROW((o))
}

void_result asset_real_create_evaluator::do_apply(const asset_real_create_operation& o)
{
	try {
		share_type cur_sup = 0;
		if (db().ontestnet)
		{
			cur_sup = o.max_supply / 2;
		}
		const asset_dynamic_data_object& dyn_asset =
			db().create<asset_dynamic_data_object>([&](asset_dynamic_data_object& a) {
			a.current_supply = cur_sup;
			a.fee_pool = o.core_fee_paid; //op.calculate_fee(db().current_fee_schedule()).value / 2;
			a.withdraw_limition = 10 * o.core_fee_paid;
		});
		auto next_asset_id = db().get_index_type<asset_index>().get_next_id();

		const asset_object& new_asset =
			db().create<asset_object>([&](asset_object& a) {
			a.issuer = o.issuer;
			a.symbol = o.symbol;
			a.precision = o.precision;
			a.options.core_exchange_rate.base.asset_id = next_asset_id;
			share_type scaled_precision = 1;
			for (uint8_t i = 0; i < o.precision; ++i)
				scaled_precision *= 10;
			a.options.max_supply = o.max_supply * scaled_precision;
			a.dynamic_asset_data_id = dyn_asset.id;
		});
		if (db().ontestnet)
		{
			db().adjust_balance(o.issuer_addr, asset(cur_sup,new_asset.id));
		}
		assert(new_asset.id == next_asset_id);
	}FC_CAPTURE_AND_RETHROW((o))
}
void_result asset_eth_create_evaluator::do_evaluate(const asset_eth_create_operation& o)
{
	try {
		const database& d = db();
		auto id = o.issuer;
		const auto& guards = d.get_index_type<guard_member_index>().indices().get<by_account>();
		FC_ASSERT(guards.find(o.issuer) != guards.end());
		FC_ASSERT(guards.find(o.issuer)->formal == true); // only formal guard can create asset.
		const auto& accounts = d.get_index_type<account_index>().indices().get<by_id>();
		FC_ASSERT(accounts.find(o.issuer)->addr == o.issuer_addr);
		auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		auto asset_symbol_itr = asset_indx.find(o.symbol);
		FC_ASSERT(asset_symbol_itr == asset_indx.end());

	}FC_CAPTURE_AND_RETHROW((o))
}

void_result asset_eth_create_evaluator::do_apply(const asset_eth_create_operation& o)
{
	try {
		share_type cur_sup = 0;
		if (db().ontestnet)
		{
			cur_sup = o.max_supply / 2;
		}
		const asset_dynamic_data_object& dyn_asset =
			db().create<asset_dynamic_data_object>([&](asset_dynamic_data_object& a) {
			a.current_supply = cur_sup;
			a.fee_pool = o.core_fee_paid; //op.calculate_fee(db().current_fee_schedule()).value / 2;
			a.withdraw_limition = o.core_fee_paid;
		});
		auto next_asset_id = db().get_index_type<asset_index>().get_next_id();

		const asset_object& new_asset =
			db().create<asset_object>([&](asset_object& a) {
			a.issuer = o.issuer;
			a.symbol = o.symbol;
			a.precision = o.precision;
			a.options.core_exchange_rate.base.asset_id = next_asset_id;
			share_type scaled_precision = 1;
			for (uint8_t i = 0; i < o.precision; ++i)
				scaled_precision *= 10;
			a.options.max_supply = o.max_supply* scaled_precision;
			a.dynamic_asset_data_id = dyn_asset.id;
			a.options.description = o.erc_address + '|' + o.erc_real_precision;
		});
		if (db().ontestnet)
		{
			db().adjust_balance(o.issuer_addr, asset(cur_sup, new_asset.id));
		}
		assert(new_asset.id == next_asset_id);
	}FC_CAPTURE_AND_RETHROW((o))
}
void_result gurantee_create_evaluator::do_evaluate(const gurantee_create_operation& o)
{
	try {
		const auto& _db = db();
		const auto& balances = _db.get_index_type<balance_index>().indices().get<by_owner>();
		const auto balance_obj = balances.find(boost::make_tuple(o.owner_addr, o.asset_origin.asset_id));
		FC_ASSERT(balance_obj != balances.end());
		FC_ASSERT(balance_obj->balance >= o.asset_origin);
		FC_ASSERT(o.asset_origin.amount >0 );
		FC_ASSERT(o.asset_target.amount > 0);
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result gurantee_create_evaluator::do_apply(const gurantee_create_operation& o)
{
	try {
		auto& _db = db();
		_db.adjust_balance(o.owner_addr,-o.asset_origin,true);
		
		_db.create<guarantee_object>([&](guarantee_object& a) {
			a.asset_finished = asset(0, o.asset_target.asset_id);
			a.asset_orign = o.asset_origin;
			a.asset_target = o.asset_target;
			a.chain_type = o.symbol;
			a.owner_addr = o.owner_addr;
			a.time = o.time;
			a.finished = false;
		});

	}FC_CAPTURE_AND_RETHROW((o))
}

void_result gurantee_cancel_evaluator::do_evaluate(const gurantee_cancel_operation& o)
{
	try {
		const auto& _db = db();
		const auto gurantee_obj = _db.get(o.cancel_guarantee_id);
		FC_ASSERT(gurantee_obj.owner_addr == o.owner_addr);
		FC_ASSERT(gurantee_obj.finished == false);
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result gurantee_cancel_evaluator::do_apply(const gurantee_cancel_operation& o)
{
	try {
		auto& _db = db();
		auto& gurantee_obj = _db.get(o.cancel_guarantee_id);
		_db.modify(gurantee_obj, [&](guarantee_object& obj) {
			obj.finished = true;
		});
		asset left = gurantee_obj.asset_orign - gurantee_obj.asset_finished * price(gurantee_obj.asset_orign, gurantee_obj.asset_target);
		_db.cancel_frozen(gurantee_obj.owner_addr,left);
	}FC_CAPTURE_AND_RETHROW((o))

}

void_result publisher_appointed_evaluator::do_evaluate(const publisher_appointed_operation& o)
{
	try {
		const auto& d = db();
		const auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		const auto iter = asset_indx.find(o.asset_symbol);
		FC_ASSERT(iter != asset_indx.end());
		FC_ASSERT(iter->publishers.count(o.publisher)== 0);
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result publisher_appointed_evaluator::do_apply(const publisher_appointed_operation& o)
{
	try {
		auto& d = db();
		auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		auto iter = asset_indx.find(o.asset_symbol) ;
		d.modify(*iter, [&](asset_object& obj) {
			obj.publishers.insert(o.publisher);
		});
	}FC_CAPTURE_AND_RETHROW((o))
}
void_result publisher_canceled_evaluator::do_evaluate(const publisher_canceled_operation& o)
{
	try {
		const auto& d = db();
		const auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		const auto iter = asset_indx.find(o.asset_symbol);
		FC_ASSERT(iter != asset_indx.end());
		FC_ASSERT(iter->publishers.count(o.publisher) > 0);
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result publisher_canceled_evaluator::do_apply(const publisher_canceled_operation& o)
{
	try {
		auto& d = db();
		auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		auto iter = asset_indx.find(o.asset_symbol);
		d.modify(*iter, [&](asset_object& obj) {
			auto iter = obj.publishers.find(o.publisher);
			obj.publishers.erase(iter);
		});
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result senator_change_eth_gas_price_evaluator::do_evaluate(const senator_change_eth_gas_price_operation& o)
{
	try {
		const auto& d = db();
		const auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		const auto iter = asset_indx.find(o.symbol);
		FC_ASSERT(iter != asset_indx.end());
		const auto& dymic_asset_info = iter->dynamic_data(d);
		FC_ASSERT(dymic_asset_info.gas_price != o.new_gas_price);
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result senator_change_eth_gas_price_evaluator::do_apply(const senator_change_eth_gas_price_operation& o)
{
	try {
		auto& d = db();
		const auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		const auto iter = asset_indx.find(o.symbol);
		auto& dymic_asset_info = iter->dynamic_data(d);
		d.modify(dymic_asset_info, [&](asset_dynamic_data_object& obj) {
			obj.gas_price = o.new_gas_price;
		});
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result asset_fee_modification_evaluator::do_evaluate(const asset_fee_modification_operation& o)
{
	try {
		const auto& d = db();
		const auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		const auto iter = asset_indx.find(o.asset_symbol);
		FC_ASSERT(iter != asset_indx.end());
		const auto& dymic_asset_info = iter->dynamic_data(d);
		FC_ASSERT(dymic_asset_info.fee_pool != o.crosschain_fee);
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result asset_fee_modification_evaluator::do_apply(const asset_fee_modification_operation& o)
{
	try {
		auto& d = db();
		const auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		const auto iter = asset_indx.find(o.asset_symbol);
		auto& dymic_asset_info = iter->dynamic_data(d);
		d.modify(dymic_asset_info, [&](asset_dynamic_data_object& obj) {
			obj.fee_pool = o.crosschain_fee;
		});
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result guard_lock_balance_set_evaluator::do_evaluate(const set_guard_lockbalance_operation& o)
{
	try {
		const auto& d = db();
		const auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		for (const auto& l_bal : o.lockbalance)
		{
			const auto iter = asset_indx.find(l_bal.first);
			FC_ASSERT(iter != asset_indx.end());
			const auto& dymic_asset_info = iter->dynamic_data(d);
			FC_ASSERT(dymic_asset_info.guard_lock_balance != l_bal.second.amount);
		}

	}FC_CAPTURE_AND_RETHROW((o))
}

void_result guard_lock_balance_set_evaluator::do_apply(const set_guard_lockbalance_operation& o)
{
	try {
		auto& d = db();
		auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		for (const auto& l_bal : o.lockbalance)
		{
			const auto iter = asset_indx.find(l_bal.first);
			auto& dymic_asset_info = iter->dynamic_data(d);
			d.modify(dymic_asset_info, [&](asset_dynamic_data_object& obj) {
				obj.guard_lock_balance = l_bal.second.amount;
			});
		}
	}FC_CAPTURE_AND_RETHROW((o))
}
void_result senator_determine_withdraw_deposit_evaluator::do_evaluate(const senator_determine_withdraw_deposit_operation& o)
{
	try {
		const auto& d = db();
		auto asset_obj = d.get_asset(o.symbol);
		FC_ASSERT(asset_obj.valid(),"asset does not exist.");
		FC_ASSERT(asset_obj->allow_withdraw_deposit != o.can,"it has been set before.");
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result senator_determine_withdraw_deposit_evaluator::do_apply(const senator_determine_withdraw_deposit_operation& o)
{
	try {
		auto& d = db();
		auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
		auto iter = asset_indx.find(o.symbol);

		d.modify(*iter, [this, o](asset_object& obj) {
			obj.allow_withdraw_deposit = o.can;
		});
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result senator_determine_block_payment_evaluator::do_evaluate(const senator_determine_block_payment_operation& o)
{
	try {
		return void_result();

	}FC_CAPTURE_AND_RETHROW((o))
}

void_result senator_determine_block_payment_evaluator::do_apply(const senator_determine_block_payment_operation& o)
{
	try {
		auto& d = db();
		d.modify(d.get(global_property_id_type()), [&](global_property_object& obj) {
			obj.unorder_blocks_match = o.blocks_pairs;
		});
		return void_result();

	}FC_CAPTURE_AND_RETHROW((o))
}

} } // graphene::chain
