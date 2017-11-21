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
#include <graphene/witness/witness.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/crosschain/crosschain_interface_emu.hpp>
#include <graphene/crosschain/crosschain.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <iostream>

using namespace graphene::miner_plugin;
using std::string;
using std::vector;

namespace bpo = boost::program_options;

void new_chain_banner( const graphene::chain::database& db )
{
   std::cerr << "\n"
      "********************************\n"
      "*                              *\n"
      "*   ------- NEW CHAIN ------   *\n"
      "*   - Welcome to Graphene! -   *\n"
      "*   ------------------------   *\n"
      "*                              *\n"
      "********************************\n"
      "\n";
   if( db.get_slot_at_time( fc::time_point::now() ) > 200 )
   {
      std::cerr << "Your genesis seems to have an old timestamp\n"
         "Please consider using the --genesis-timestamp option to give your genesis a recent timestamp\n"
         "\n"
         ;
   }
   return;
}

void set_operation_fees(signed_transaction& tx, const fee_schedule& s)
{
	for (auto& op : tx.operations)
		s.set_fee(op);
}

void miner_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   auto default_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("nathan")));

   vector<std::pair<chain::public_key_type, string>> vec;
   vec.push_back
    (std::make_pair(chain::public_key_type(default_priv_key.get_public_key()), graphene::utilities::key_to_wif(default_priv_key)));
   for (uint64_t i = 0; i < GRAPHENE_DEFAULT_MIN_MINER_COUNT; i++)
   {
	   auto name = "miner" + fc::to_string(i);
	   auto name_key = fc::ecc::private_key::regenerate(fc::sha256::hash(name));
	   vec.push_back
	   (std::make_pair(chain::public_key_type(name_key.get_public_key()), graphene::utilities::key_to_wif(name_key)));
   }

   for (uint64_t i = 0; i < GRAPHENE_DEFAULT_MIN_GUARD_COUNT; i++)
   {
	   auto name = "guard" + fc::to_string(i);
	   auto name_key = fc::ecc::private_key::regenerate(fc::sha256::hash(name));
	   vec.push_back
	   (std::make_pair(chain::public_key_type(name_key.get_public_key()), graphene::utilities::key_to_wif(name_key)));
   }

   string miner_id_example = fc::json::to_string(chain::miner_id_type(5));
   command_line_options.add_options()
         ("enable-stale-production", bpo::bool_switch()->notifier([this](bool e){_production_enabled = e;}), "Enable block production, even if the chain is stale.")
         ("required-participation", bpo::bool_switch()->notifier([this](int e){_required_miner_participation = uint32_t(e*GRAPHENE_1_PERCENT);}), "Percent of miners (0-99) that must be participating in order to produce blocks")
         ("miner-id,w", bpo::value<vector<string>>()->composing()->multitoken(),
          ("ID of miner controlled by this node (e.g. " + miner_id_example + ", quotes are required, may specify multiple times)").c_str())
         ("private-key", bpo::value<string>()->composing()->multitoken()->
          DEFAULT_VALUE_VECTOR(vec),
          "Tuple of [PublicKey, WIF private key] (just append)")
         ;
   config_file_options.add(command_line_options);
}

std::string miner_plugin::plugin_name()const
{
   return "miner";
}

void miner_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   ilog("miner plugin:  plugin_initialize() begin");
   _options = &options;
   LOAD_VALUE_SET(options, "miner-id", _miners, chain::miner_id_type)

   if( options.count("private-key") )
   {
      const std::string key_id_to_wif_pair_strings = options["private-key"].as<std::string>();
	  auto key_id_to_wif_pairs = graphene::app::dejsonify<vector<std::pair<chain::public_key_type, std::string>> >(key_id_to_wif_pair_strings);
      for (auto& key_id_to_wif_pair : key_id_to_wif_pairs)
      {
         //idump((key_id_to_wif_pair));
         ilog("Public Key: ${public}", ("public", key_id_to_wif_pair.first));
         fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(key_id_to_wif_pair.second);
         if (!private_key)
         {
            // the key isn't in WIF format; see if they are still passing the old native private key format.  This is
            // just here to ease the transition, can be removed soon
            try
            {
               private_key = fc::variant(key_id_to_wif_pair.second).as<fc::ecc::private_key>();
            }
            catch (const fc::exception&)
            {
               FC_THROW("Invalid WIF-format private key ${key_string}", ("key_string", key_id_to_wif_pair.second));
            }
         }
         _private_keys[key_id_to_wif_pair.first] = *private_key;
      }
   }
   ilog("miner plugin:  plugin_initialize() end");
} FC_LOG_AND_RETHROW() }

void miner_plugin::plugin_startup()
{ try {
   ilog("miner plugin:  plugin_startup() begin");
   chain::database& d = database();

   if( !_miners.empty() )
   {
      ilog("Launching block production for ${n} mining.", ("n", _miners.size()));
      app().set_block_production(true);
      if( _production_enabled )
      {
         if( d.head_block_num() == 0 )
            new_chain_banner(d);
         _production_skip_flags |= graphene::chain::database::skip_undo_history_check;
      }
      schedule_production_loop();
   } else
      elog("No miners configured! Please add miner IDs and private keys to configuration.");
   ilog("miner plugin:  plugin_startup() end");
} FC_CAPTURE_AND_RETHROW() }

void miner_plugin::plugin_shutdown()
{
   return;
}

void miner_plugin::schedule_production_loop()
{
   //Schedule for the next second's tick regardless of chain state
   // If we would wait less than 50ms, wait for the whole second.
   fc::time_point now = fc::time_point::now();
   int64_t time_to_next_second = 1000000 - (now.time_since_epoch().count() % 1000000);
   if( time_to_next_second < 50000 )      // we must sleep for at least 50ms
       time_to_next_second += 1000000;

   fc::time_point next_wakeup( now + fc::microseconds( time_to_next_second ) );

   //wdump( (now.time_since_epoch().count())(next_wakeup.time_since_epoch().count()) );
   _block_production_task = fc::schedule([this]{block_production_loop();},
                                         next_wakeup, "Miner Block Production");
}

block_production_condition::block_production_condition_enum miner_plugin::block_production_loop()
{
   block_production_condition::block_production_condition_enum result;
   fc::mutable_variant_object capture;
   try
   {
      result = maybe_produce_block(capture);
   }
   catch( const fc::canceled_exception& )
   {
      //We're trying to exit. Go ahead and let this one out.
      throw;
   }
   catch( const fc::exception& e )
   {
      elog("Got exception while generating block:\n${e}", ("e", e.to_detail_string()));
      result = block_production_condition::exception_producing_block;
   }

   switch( result )
   {
      case block_production_condition::produced:
         ilog("Generated block #${n} with timestamp ${t} at time ${c}", (capture));
         break;
      case block_production_condition::not_synced:
         ilog("Not producing block because production is disabled until we receive a recent block (see: --enable-stale-production)");
         break;
      case block_production_condition::not_my_turn:
         //ilog("Not producing block because it isn't my turn");
         break;
      case block_production_condition::not_time_yet:
         //ilog("Not producing block because slot has not yet arrived");
         break;
      case block_production_condition::no_private_key:
         ilog("Not producing block because I don't have the private key for ${scheduled_key}", (capture) );
         break;
      case block_production_condition::low_participation:
         elog("Not producing block because node appears to be on a minority fork with only ${pct}% miner participation", (capture) );
         break;
      case block_production_condition::lag:
         elog("Not producing block because node didn't wake up within 500ms of the slot time.");
         break;
      case block_production_condition::consecutive:
         elog("Not producing block because the last block was generated by the same miner.\nThis node is probably disconnected from the network so block production has been disabled.\nDisable this check with --allow-consecutive option.");
         break;
      case block_production_condition::exception_producing_block:
         elog( "exception prodcing block" );
         break;
   }

   schedule_production_loop();
   return result;
}

fc::variant miner_plugin::check_generate_multi_addr(miner_id_type miner,fc::ecc::private_key pk)
{
	chain::database& db = database();
	try {
		const auto& addr = db.get_index_type<multisig_address_index>().indices().get<by_account_chain_type>();
		const auto& guard_ids = db.get_global_properties().active_committee_members;
		const auto& symbols = db.get_index_type<asset_index>().indices().get<by_symbol>();
		auto& instance = graphene::crosschain::crosschain_manager::get_instance();
		

		const auto& miners = db.get_index_type<miner_index>().indices().get<by_id>();
		auto iter = miners.find(miner);
		auto accid = iter->miner_account;
		const auto& accounts = db.get_index_type<account_index>().indices().get<by_id>();
		auto miner_addr = accounts.find(accid)->addr;

		//get cold address
		for (auto iter : symbols)
		{
			vector<string> symbol_addrs_cold;
			vector<string> symbol_addrs_hot;
			auto crosschain_interface = instance.get_crosschain_handle(iter.symbol);
			auto addr_range=addr.equal_range(boost::make_tuple(iter.symbol));
			std::for_each(
				addr_range.first, addr_range.second, [&symbol_addrs_cold,&symbol_addrs_hot](const multisig_address_object& obj) {
				symbol_addrs_cold.push_back(obj.new_address_cold);
				symbol_addrs_hot.push_back(obj.new_address_hot);
			}
			);
			if (symbol_addrs_cold.size() == guard_ids.size() && symbol_addrs_hot.size() == guard_ids.size())
			{
				
				auto multi_addr_cold = crosschain_interface->create_multi_sig_account(iter.symbol+"_cold", symbol_addrs_cold, std::ceill(symbol_addrs_cold.size()*2/3));
				auto multi_addr_hot = crosschain_interface->create_multi_sig_account(iter.symbol + "_hot", symbol_addrs_hot, std::ceill(symbol_addrs_hot.size() * 2 / 3));
				signed_transaction trx;
				miner_generate_multi_asset_operation op;
				uint32_t expiration_time_offset = 0;
				auto dyn_props = db.get_dynamic_global_properties();
				op.miner = miner;
				op.miner_address = miner_addr;
				op.multi_address_cold = multi_addr_cold;
				op.multi_address_hot = multi_addr_hot;
				op.chain_type = iter.symbol;
				trx.operations.emplace_back(op);
				set_operation_fees(trx,db.get_global_properties().parameters.current_fees);
				trx.set_reference_block(dyn_props.head_block_id);
				trx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
				trx.sign(pk, db.get_chain_id());
				db.push_transaction(trx);
			}
		}
	}FC_CAPTURE_AND_LOG((0))
	return fc::variant();
}

void miner_plugin::check_multi_transfer(miner_id_type miner, fc::ecc::private_key pk)
{
	chain::database& db = database();
	try {
		const auto& miners = db.get_index_type<miner_index>().indices().get<by_id>();
		auto iter = miners.find(miner);
		auto accid = iter->miner_account;
		const auto& accounts = db.get_index_type<account_index>().indices().get<by_id>();
		auto miner_addr = accounts.find(accid)->addr;
		const auto& guard_ids = db.get_global_properties().active_committee_members;
		const auto& transfers = db.get_index_type<crosschain_transfer_index>().indices().get<by_status>();
		uint32_t expiration_time_offset = 0;
		auto dyn_props = db.get_dynamic_global_properties();
		
		for (auto transfer : transfers)
		{
			if (transfer.signatures.size() >= std::ceill(guard_ids.size() * 2 / 3))
			{
				miner_merge_signatures_operation op;
				op.miner = miner;
				op.miner_address = miner_addr;
				op.chain_type = transfer.chain_type;
				op.id = transfer.id;
				signed_transaction trx;
				trx.operations.emplace_back(op);
				set_operation_fees(trx, db.get_global_properties().parameters.current_fees);
				trx.set_reference_block(dyn_props.head_block_id);
				trx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
				trx.sign(pk, db.get_chain_id());
				db.push_transaction(trx);
			}
			
		}
	}FC_CAPTURE_AND_LOG((0))
}


block_production_condition::block_production_condition_enum miner_plugin::maybe_produce_block( fc::mutable_variant_object& capture )
{
   chain::database& db = database();
   fc::time_point now_fine = fc::time_point::now();
   fc::time_point_sec now = now_fine + fc::microseconds( 500000 );

   // If the next block production opportunity is in the present or future, we're synced.
   if( !_production_enabled )
   {
      if( db.get_slot_time(1) >= now )
         _production_enabled = true;
      else
         return block_production_condition::not_synced;
   }

   // is anyone scheduled to produce now or one second in the future?
   uint32_t slot = db.get_slot_at_time( now );
   if( slot == 0 )
   {
      capture("next_time", db.get_slot_time(1));
      return block_production_condition::not_time_yet;
   }

   //
   // this assert should not fail, because now <= db.head_block_time()
   // should have resulted in slot == 0.
   //
   // if this assert triggers, there is a serious bug in get_slot_at_time()
   // which would result in allowing a later block to have a timestamp
   // less than or equal to the previous block
   //
   assert( now > db.head_block_time() );

   graphene::chain::miner_id_type scheduled_miner = db.get_scheduled_miner( slot );
   // we must control the miner scheduled to produce the next block.
   if( _miners.find( scheduled_miner ) == _miners.end() )
   {
      capture("scheduled_miner", scheduled_miner);
      return block_production_condition::not_my_turn;
   }

   fc::time_point_sec scheduled_time = db.get_slot_time( slot );
   graphene::chain::public_key_type scheduled_key = scheduled_miner( db ).signing_key;
   auto private_key_itr = _private_keys.find( scheduled_key );

   if( private_key_itr == _private_keys.end() )
   {
      capture("scheduled_key", scheduled_key);
      return block_production_condition::no_private_key;
   }

   uint32_t prate = db.miner_participation_rate();
   if( prate < _required_miner_participation )
   {
      capture("pct", uint32_t(100*uint64_t(prate) / GRAPHENE_1_PERCENT));
      return block_production_condition::low_participation;
   }

   if( llabs((scheduled_time - now).count()) > fc::milliseconds( 500 ).count() )
   {
      capture("scheduled_time", scheduled_time)("now", now);
      return block_production_condition::lag;
   }
   //through this to generate new multi-addr
   auto varient_obj = check_generate_multi_addr(scheduled_miner, private_key_itr->second);
   db.create_result_transaction(scheduled_miner, private_key_itr->second);
   db.combine_sign_transaction(scheduled_miner, private_key_itr->second);
   check_multi_transfer(scheduled_miner, private_key_itr->second);
   //generate blocks
   auto block = db.generate_block(
      scheduled_time,
	   scheduled_miner,
      private_key_itr->second,
      _production_skip_flags
      );
   capture("n", block.block_num())("t", block.timestamp)("c", now);
   fc::async( [this,block](){ p2p_node().broadcast(net::block_message(block)); } );

   return block_production_condition::produced;
}
