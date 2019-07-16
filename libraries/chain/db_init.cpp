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

#include <graphene/chain/evalutor_inc.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>
#include <fc/crypto/digest.hpp>

#include <boost/algorithm/string.hpp>

namespace graphene { namespace chain {

// C++ requires that static class variables declared and initialized
// in headers must also have a definition in a single source file,
// else linker errors will occur [1].
//
// The purpose of this source file is to collect such definitions in
// a single place.
//
// [1] http://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char

const uint8_t account_object::space_id;
const uint8_t account_object::type_id;

const uint8_t asset_object::space_id;
const uint8_t asset_object::type_id;

const uint8_t block_summary_object::space_id;
const uint8_t block_summary_object::type_id;

const uint8_t call_order_object::space_id;
const uint8_t call_order_object::type_id;

const uint8_t guard_member_object::space_id;
const uint8_t guard_member_object::type_id;

const uint8_t force_settlement_object::space_id;
const uint8_t force_settlement_object::type_id;

const uint8_t global_property_object::space_id;
const uint8_t global_property_object::type_id;

const uint8_t limit_order_object::space_id;
const uint8_t limit_order_object::type_id;

const uint8_t operation_history_object::space_id;
const uint8_t operation_history_object::type_id;

const uint8_t proposal_object::space_id;
const uint8_t proposal_object::type_id;

const uint8_t transaction_object::space_id;
const uint8_t transaction_object::type_id;

const uint8_t vesting_balance_object::space_id;
const uint8_t vesting_balance_object::type_id;

const uint8_t withdraw_permission_object::space_id;
const uint8_t withdraw_permission_object::type_id;

const uint8_t miner_object::space_id;
const uint8_t miner_object::type_id;

const uint8_t worker_object::space_id;
const uint8_t worker_object::type_id;

const uint8_t lockbalance_object::space_id;
const uint8_t lockbalance_object::type_id;

const uint8_t guard_lock_balance_object::space_id;
const uint8_t guard_lock_balance_object::type_id;
const uint8_t crosschain_trx_object::space_id;
const uint8_t crosschain_trx_object::type_id;
const uint8_t multisig_asset_transfer_object::space_id;
const uint8_t multisig_asset_transfer_object::type_id;

const uint8_t acquired_crosschain_trx_object::space_id;
const uint8_t acquired_crosschain_trx_object::type_id;
const uint8_t multisig_account_pair_object::space_id;
const uint8_t multisig_account_pair_object::type_id;
const uint8_t crosschain_transaction_history_count_object::space_id;
const uint8_t crosschain_transaction_history_count_object::type_id;
const uint8_t coldhot_transfer_object::space_id;
const uint8_t coldhot_transfer_object::type_id;

const uint8_t contract_object::space_id;
const uint8_t contract_object::type_id;
const uint8_t contract_storage_object::space_id;
const uint8_t contract_storage_object::type_id;
const uint8_t transaction_contract_storage_diff_object::space_id;
const uint8_t transaction_contract_storage_diff_object::type_id;

const uint8_t contract_event_notify_object::space_id;
const uint8_t contract_event_notify_object::type_id;

const uint8_t contract_invoke_result_object::space_id;
const uint8_t contract_invoke_result_object::type_id;

const uint8_t guarantee_object::space_id;
const uint8_t guarantee_object::type_id;

const uint8_t pay_back_object::space_id;
const uint8_t pay_back_object::type_id;

const uint8_t bonus_object::space_id;
const uint8_t bonus_object::type_id;

const uint8_t eth_multi_account_trx_object::space_id;
const uint8_t eth_multi_account_trx_object::type_id;
const uint8_t whiteOperationList_object::space_id;
const uint8_t whiteOperationList_object::type_id;

void database::initialize_evaluators()
{
   _operation_evaluators.resize(255);
   register_evaluator<lockbalance_evaluator>();
   register_evaluator<foreclose_balance_evaluator>();
   register_evaluator<pay_back_evaluator>();
   register_evaluator<bonus_evaluator>();
   register_evaluator<crosschain_record_evaluate>(); 
   register_evaluator<crosschain_withdraw_evaluate>();
   register_evaluator<crosschain_withdraw_without_sign_evaluate>();
   register_evaluator<crosschain_withdraw_with_sign_evaluate>();
   register_evaluator<crosschain_withdraw_combine_sign_evaluate>();
   register_evaluator<crosschain_withdraw_result_evaluate>();
   register_evaluator<coldhot_transfer_evaluate>();
   register_evaluator<coldhot_transfer_without_sign_evaluate>();
   register_evaluator<coldhot_transfer_with_sign_evaluate>();
   register_evaluator<coldhot_transfer_combine_sign_evaluate>();
   register_evaluator<coldhot_transfer_result_evaluate>();
   register_evaluator<coldhot_cancel_transafer_transaction_evaluate>();
   register_evaluator<coldhot_cancel_uncombined_trx_evaluate>();
   register_evaluator<eths_guard_change_signer_evaluator>();
   register_evaluator<eths_guard_coldhot_change_signer_evaluator>();
   register_evaluator<eth_cancel_coldhot_fail_trx_evaluate>();
   register_evaluator<guard_lock_balance_evaluator>();
   register_evaluator<guard_foreclose_balance_evaluator>();
   register_evaluator<account_create_evaluator>();
   register_evaluator<account_update_evaluator>();
   register_evaluator<account_upgrade_evaluator>();
   register_evaluator<account_whitelist_evaluator>();
   register_evaluator<account_bind_evaluator>();
   register_evaluator<account_unbind_evaluator>();
   register_evaluator<asset_transfer_from_cold_to_hot_evaluator>();
   register_evaluator<sign_multisig_asset_evaluator>();
   register_evaluator<account_multisig_create_evaluator>();
   register_evaluator<guard_member_create_evaluator>();
   register_evaluator<guard_member_update_evaluator>();
   register_evaluator<committee_member_update_global_parameters_evaluator>();
   register_evaluator<committee_member_execute_coin_destory_operation_evaluator>();
   register_evaluator<guard_member_resign_evaluator>();
   //register_evaluator<custom_evaluator>();
   register_evaluator<asset_create_evaluator>();
   register_evaluator<asset_issue_evaluator>();
   register_evaluator<asset_reserve_evaluator>();
   register_evaluator<asset_update_evaluator>();
   register_evaluator<asset_update_bitasset_evaluator>();
   register_evaluator<asset_update_feed_producers_evaluator>();
   //register_evaluator<asset_settle_evaluator>();
   register_evaluator<asset_global_settle_evaluator>();
   register_evaluator<assert_evaluator>();
   /* register_evaluator<limit_order_create_evaluator>();
	register_evaluator<limit_order_cancel_evaluator>();
	register_evaluator<call_order_update_evaluator>();*/
   register_evaluator<transfer_evaluator>();
   //register_evaluator<override_transfer_evaluator>();
   register_evaluator<asset_fund_fee_pool_evaluator>();
   register_evaluator<asset_publish_feeds_evaluator>();
   register_evaluator<normal_asset_publish_feeds_evaluator>();
   register_evaluator<proposal_create_evaluator>();
   register_evaluator<proposal_update_evaluator>();
   register_evaluator<proposal_delete_evaluator>();
   register_evaluator<vesting_balance_create_evaluator>();
   register_evaluator<vesting_balance_withdraw_evaluator>();
   register_evaluator<miner_create_evaluator>();
   register_evaluator<witness_update_evaluator>();
   register_evaluator<withdraw_permission_create_evaluator>();
   register_evaluator<withdraw_permission_claim_evaluator>();
   register_evaluator<withdraw_permission_update_evaluator>();
   register_evaluator<withdraw_permission_delete_evaluator>();
   register_evaluator<worker_create_evaluator>();
   register_evaluator<balance_claim_evaluator>();
   register_evaluator<transfer_to_blind_evaluator>();
   register_evaluator<transfer_from_blind_evaluator>();
   register_evaluator<blind_transfer_evaluator>();
   register_evaluator<asset_claim_fees_evaluator>();
   register_evaluator<guard_refund_balance_evaluator>();
   register_evaluator<guard_refund_crosschain_trx_evaluator>();
   register_evaluator<eth_cancel_fail_crosschain_trx_evaluate>();
   register_evaluator<asset_real_create_evaluator>();
   register_evaluator<miner_generate_multi_asset_evaluator>();
   register_evaluator<guard_update_multi_account_evaluator>();

   register_evaluator<contract_register_evaluate>();
   register_evaluator<native_contract_register_evaluate>();
   register_evaluator<contract_invoke_evaluate>();
   register_evaluator<contract_upgrade_evaluate>();
   register_evaluator<contract_transfer_evaluate>();
   register_evaluator<contract_transfer_fee_evaluate>();
   register_evaluator<gurantee_create_evaluator>();
   register_evaluator<gurantee_cancel_evaluator>();
   register_evaluator<publisher_appointed_evaluator>();
   register_evaluator<publisher_canceled_evaluator>();
   register_evaluator<asset_fee_modification_evaluator>();
   register_evaluator<guard_lock_balance_evaluator>();
   register_evaluator<senator_determine_withdraw_deposit_evaluator>();
   register_evaluator<account_create_multisignature_address_evaluator>();
	register_evaluator<senator_determine_block_payment_evaluator>();
	register_evaluator<eth_series_multi_sol_create_evaluator>();
   register_evaluator<eth_series_multi_sol_guard_sign_evaluator>();
   register_evaluator<eth_multi_account_create_record_evaluator>();
   register_evaluator<eths_guard_sign_final_evaluator>();
   register_evaluator<asset_eth_create_evaluator>();
   register_evaluator<eths_coldhot_guard_sign_final_evaluator>();
   register_evaluator< referendum_create_evaluator > ();
   register_evaluator<referendum_update_evaluator>();
   register_evaluator<citizen_referendum_senator_evaluator>();
   register_evaluator<referendum_accelerate_pledge_evaluator>();
   register_evaluator<block_address_evaluator>();
   register_evaluator<cancel_address_block_evaluator>();
   register_evaluator<guard_cancel_combine_trx_evaluator>();
   register_evaluator<coldhot_cancel_combined_trx_evaluate>();
   register_evaluator<senator_pass_success_trx_evaluate>();
   register_evaluator<coldhot_pass_combine_trx_evaluate>();
   register_evaluator<senator_change_eth_gas_price_evaluator>();
   register_evaluator<eths_cancel_unsigned_transaction_evaluator>();
   register_evaluator<senator_change_acquire_trx_evaluator>();
   register_evaluator<add_whiteOperation_list_evaluator>();
   register_evaluator<cancel_whiteOperation_list_evaluator>();
   register_evaluator<set_balance_evaluator>();
   register_evaluator<correct_chain_data_evaluator>();
   register_evaluator<vote_create_evaluator>();
   register_evaluator<vote_update_evaluator>();
   register_evaluator<undertaker_evaluator>();
   register_evaluator<name_transfer_evaluator>();
}

void database::initialize_indexes()
{
   reset_indexes();
   _undo_db.set_max_size( GRAPHENE_MIN_UNDO_HISTORY );

   //Protocol object indexes
   add_index< primary_index<asset_index> >();
   add_index< primary_index<force_settlement_index> >();
   add_index<primary_index<lockbalance_index>>();
   add_index<primary_index<payback_index>>();
   add_index <primary_index<bonus_index>>();
   add_index<primary_index<guard_lock_balance_index>>();
   add_index<primary_index<crosschain_trx_index>>();
   add_index<primary_index<coldhot_transfer_index>>();
   add_index<primary_index<acquired_crosschain_index>>();
   add_index<primary_index<transaction_history_count_index>>();
   auto acnt_index = add_index< primary_index<account_index> >();
   acnt_index->add_secondary_index<account_member_index>();
   acnt_index->add_secondary_index<account_referrer_index>();

   add_index< primary_index<guard_member_index> >();
   add_index< primary_index<miner_index> >();
   add_index< primary_index<limit_order_index > >();
   add_index< primary_index<call_order_index > >();
   add_index< primary_index<crosschain_transfer_index> >();
   auto prop_index = add_index< primary_index<proposal_index > >();
   prop_index->add_secondary_index<required_approval_index>();

   add_index< primary_index<withdraw_permission_index > >();
   add_index< primary_index<vesting_balance_index> >();
   add_index< primary_index<worker_index> >();
   add_index< primary_index<balance_index> >();
   add_index< primary_index<blinded_balance_index> >();

   //Implementation object indexes
   add_index< primary_index<transaction_index                             > >();
   add_index< primary_index<account_balance_index                         > >();
   add_index< primary_index<account_binding_index                         > >();
   add_index< primary_index<multisig_account_pair_index                   > >();
   add_index< primary_index<multisig_address_index                        > >();
   add_index< primary_index<asset_bitasset_data_index                     > >();
   add_index< primary_index<simple_index<global_property_object          >> >();
   add_index< primary_index<simple_index<dynamic_global_property_object  >> >();
   add_index<primary_index<simple_index<lockbalance_record_object        >> >();
   add_index< primary_index<simple_index<total_fees_object  >> >();
   add_index< primary_index<simple_index<account_statistics_object       >> >();
   add_index< primary_index<simple_index<asset_dynamic_data_object       >> >();
   add_index< primary_index<flat_index<  block_summary_object            >> >();
   add_index< primary_index<simple_index<chain_property_object          > > >();
   add_index< primary_index<simple_index<witness_schedule_object        > > >();
   add_index< primary_index<simple_index<budget_record_object           > > >();
   add_index< primary_index< special_authority_index                      > >();
   add_index< primary_index< buyback_index                                > >();

   add_index< primary_index< simple_index< fba_accumulator_object       > > >();

   // contract
   add_index< primary_index<transaction_contract_storage_diff_index       > >();
   add_index<primary_index<contract_object_index>>();
   add_index<primary_index<contract_storage_object_index>>();
   add_index<primary_index<contract_event_notify_index>>();
   add_index<primary_index<contract_invoke_result_index>>();
   add_index<primary_index<contract_storage_change_index>>();
   add_index <primary_index<guarantee_index                               > >();
   add_index <primary_index<contract_history_object_index                               > >();
   add_index<primary_index<eth_multi_account_trx_index>>();
   add_index<primary_index<referendum_index>>();
   add_index<primary_index<blocked_index>>  ();
   add_index<primary_index<whiteOperation_index>>();
   add_index<primary_index<vote_index>>();
   add_index<primary_index<vote_result_index>>();
}

void database::init_genesis(const genesis_state_type& genesis_state)
{ try {
   FC_ASSERT( genesis_state.initial_timestamp != time_point_sec(), "Must initialize genesis timestamp." );
   FC_ASSERT( genesis_state.initial_timestamp.sec_since_epoch() % GRAPHENE_DEFAULT_BLOCK_INTERVAL == 0,
              "Genesis timestamp must be divisible by GRAPHENE_DEFAULT_BLOCK_INTERVAL." );
   FC_ASSERT(genesis_state.initial_miner_candidates.size() > 0,
             "Cannot start a chain with zero witnesses.");
   FC_ASSERT(genesis_state.initial_active_miners <= genesis_state.initial_miner_candidates.size(),
             "initial_active_witnesses is larger than the number of candidate witnesses.");

   _undo_db.disable();
   struct auth_inhibitor {
      auth_inhibitor(database& db) : db(db), old_flags(db.node_properties().skip_flags)
      { db.node_properties().skip_flags |= skip_authority_check; }
      ~auth_inhibitor()
      { db.node_properties().skip_flags = old_flags; }
   private:
      database& db;
      uint32_t old_flags;
   } inhibitor(*this);

   transaction_evaluation_state genesis_eval_state(this);

   flat_index<block_summary_object>& bsi = get_mutable_index_type< flat_index<block_summary_object> >();
   bsi.resize(0xffff+1);

   // Create more special accounts
   while( true )
   {
      uint64_t id = get_index<account_object>().get_next_id().instance();
      if( id >= genesis_state.immutable_parameters.num_special_accounts )
         break;
      const account_object& acct = create<account_object>([&](account_object& a) {
          a.name = "special-account-" + std::to_string(id);
          a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.id;}).id;
          a.owner.weight_threshold = 1;
          a.active.weight_threshold = 1;
          a.registrar = a.lifetime_referrer = a.referrer = account_id_type(id);
          a.membership_expiration_date = time_point_sec::maximum();
          a.network_fee_percentage = GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
          a.lifetime_referrer_fee_percentage = GRAPHENE_100_PERCENT - GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
      });
      FC_ASSERT( acct.get_id() == account_id_type(id) );
      remove( acct );
   }

   // Create core asset
   const asset_dynamic_data_object& dyn_asset =
      create<asset_dynamic_data_object>([&](asset_dynamic_data_object& a) {
         //a.current_supply = GRAPHENE_MAX_SHARE_SUPPLY;
      });
   const asset_object& core_asset =
     create<asset_object>( [&]( asset_object& a ) {
         a.symbol = GRAPHENE_SYMBOL;
         a.options.max_supply = genesis_state.max_core_supply;
         a.precision = GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS;
         a.options.flags = 0;
         a.options.issuer_permissions = 0;
         a.issuer = GRAPHENE_NULL_ACCOUNT;
         a.options.core_exchange_rate.base.amount = 1;
         a.options.core_exchange_rate.base.asset_id = asset_id_type(0);
         a.options.core_exchange_rate.quote.amount = 1;
         a.options.core_exchange_rate.quote.asset_id = asset_id_type(0);
         a.dynamic_asset_data_id = dyn_asset.id;
      });
   assert( asset_id_type(core_asset.id) == asset().asset_id );
   assert( get_balance(address(), asset_id_type()) == asset(dyn_asset.current_supply) );
   // Create more special assets
   while( true )
   {
      uint64_t id = get_index<asset_object>().get_next_id().instance();
      if( id >= genesis_state.immutable_parameters.num_special_assets )
         break;
      const asset_dynamic_data_object& dyn_asset =
         create<asset_dynamic_data_object>([&](asset_dynamic_data_object& a) {
            a.current_supply = 0;
         });
      const asset_object& asset_obj = create<asset_object>( [&]( asset_object& a ) {
         a.symbol = "SPECIAL" + std::to_string( id );
         a.options.max_supply = 0;
         a.precision = GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS;
         a.options.flags = 0;
         a.options.issuer_permissions = 0;
         a.issuer = GRAPHENE_NULL_ACCOUNT;
         a.options.core_exchange_rate.base.amount = 1;
         a.options.core_exchange_rate.base.asset_id = asset_id_type(0);
         a.options.core_exchange_rate.quote.amount = 1;
         a.options.core_exchange_rate.quote.asset_id = asset_id_type(0);
         a.dynamic_asset_data_id = dyn_asset.id;
      });
      FC_ASSERT( asset_obj.get_id() == asset_id_type(id) );
      remove( asset_obj );
   }

   chain_id_type chain_id = genesis_state.compute_chain_id();

   // Create global properties
   create<global_property_object>([&](global_property_object& p) {
       p.parameters = genesis_state.initial_parameters;
       // Set fees to zero initially, so that genesis initialization needs not pay them
       // We'll fix it at the end of the function
       p.parameters.current_fees->zero_all_fees();
	   p.unorder_blocks_match[6307200] = 27 * GRAPHENE_HXCHAIN_PRECISION;
	   p.unorder_blocks_match[12614400] = 25 * GRAPHENE_HXCHAIN_PRECISION;
	   p.unorder_blocks_match[18921600] = 24 * GRAPHENE_HXCHAIN_PRECISION;
	   p.unorder_blocks_match[25228800] = 22 * GRAPHENE_HXCHAIN_PRECISION;
	   p.unorder_blocks_match[31536000] = 21 * GRAPHENE_HXCHAIN_PRECISION;
	   p.unorder_blocks_match[37843200] = 19 * GRAPHENE_HXCHAIN_PRECISION;
	   p.unorder_blocks_match[44150400] = 17* GRAPHENE_HXCHAIN_PRECISION;
	   p.unorder_blocks_match[-1] = 2 * GRAPHENE_HXCHAIN_PRECISION;
   });
   create<dynamic_global_property_object>([&](dynamic_global_property_object& p) {
      p.time = genesis_state.initial_timestamp;
      p.dynamic_flags = 0;
      p.miner_budget = 0;
      p.recent_slots_filled = fc::uint128::max_value();
   });
   create<lockbalance_record_object>([&](lockbalance_record_object& p) {});
   create<total_fees_object>([&](total_fees_object& obj) {});
   create<chain_property_object>([&](chain_property_object& p)
   {
      p.chain_id = chain_id;
      p.immutable_parameters = genesis_state.immutable_parameters;
   } );
   create<block_summary_object>([&](block_summary_object&) {});

   // Create initial accounts
   for( const auto& account : genesis_state.initial_accounts )
   {
      account_create_operation cop;
      cop.name = account.name;
      cop.registrar = GRAPHENE_TEMP_ACCOUNT;
      cop.owner = authority(1, account.owner_key, 1);
      if( account.active_key == public_key_type() )
      {
         cop.active = cop.owner;
         cop.options.memo_key = account.owner_key;
		 cop.payer = address(account.owner_key);
      }
      else
      {
         cop.active = authority(1, account.active_key, 1);
         cop.options.memo_key = account.active_key;
		 cop.payer = address(account.active_key);
      }
      account_id_type account_id(apply_operation(genesis_eval_state, cop).get<object_id_type>());

      if( account.is_lifetime_member )
      {
          account_upgrade_operation op;
          op.account_to_upgrade = account_id;
          op.upgrade_to_lifetime_member = true;
          apply_operation(genesis_eval_state, op);
      }
   }

   // Helper function to get account ID by name
   const auto& accounts_by_name = get_index_type<account_index>().indices().get<by_name>();
   auto get_account_id = [&accounts_by_name](const string& name) {
      auto itr = accounts_by_name.find(name);
      FC_ASSERT(itr != accounts_by_name.end(),
                "Unable to find account '${acct}'. Did you forget to add a record for it to initial_accounts?",
                ("acct", name));
      return itr->get_id();
   };
   auto get_account_address = [&accounts_by_name](const string& name) {
	   auto itr = accounts_by_name.find(name);
	   FC_ASSERT(itr != accounts_by_name.end(),
		   "Unable to find account '${acct}'. Did you forget to add a record for it to initial_accounts?",
		   ("acct", name));
	   return itr->addr;
   };
   const auto& senator_by_accs = get_index_type<guard_member_index>().indices().get<by_account>();
   auto get_senator = [&senator_by_accs,&get_account_id](const string& name) {
	   auto acc_id = get_account_id(name);
	   auto itr = senator_by_accs.find(acc_id);
	   FC_ASSERT(itr != senator_by_accs.end(),
		   "Unable to find senator '${acct}'. Did you forget to add a record for it to initial_accounts?",
		   ("acct", name));
	   return itr;
   };

   // Helper function to get asset ID by symbol
   const auto& assets_by_symbol = get_index_type<asset_index>().indices().get<by_symbol>();
   const auto get_asset_id = [&assets_by_symbol](const string& symbol) {
      auto itr = assets_by_symbol.find(symbol);

      // TODO: This is temporary for handling LNK snapshot
      if( symbol == "HX" )
          itr = assets_by_symbol.find(GRAPHENE_SYMBOL);

      FC_ASSERT(itr != assets_by_symbol.end(),
                "Unable to find asset '${sym}'. Did you forget to add a record for it to initial_assets?",
                ("sym", symbol));
      return itr->get_id();
   };

   map<asset_id_type, share_type> total_supplies;
   map<asset_id_type, share_type> total_delnk;

   // Create initial assets
   for( const genesis_state_type::initial_asset_type& asset : genesis_state.initial_assets )
   {
      asset_id_type new_asset_id = get_index_type<asset_index>().get_next_id();
      total_supplies[ new_asset_id ] = 0;

      asset_dynamic_data_id_type dynamic_data_id;
      optional<asset_bitasset_data_id_type> bitasset_data_id;
      if( asset.is_bitasset )
      {
         int collateral_holder_number = 0;
         total_delnk[ new_asset_id ] = 0;
         for( const auto& collateral_rec : asset.collateral_records )
         {
            account_create_operation cop;
            cop.name = asset.symbol + "-collateral-holder-" + std::to_string(collateral_holder_number);
            boost::algorithm::to_lower(cop.name);
            cop.registrar = GRAPHENE_TEMP_ACCOUNT;
            cop.owner = authority(1, collateral_rec.owner, 1);
            cop.active = cop.owner;
            account_id_type owner_account_id = apply_operation(genesis_eval_state, cop).get<object_id_type>();

            modify( owner_account_id(*this).statistics(*this), [&]( account_statistics_object& o ) {
                    o.total_core_in_orders = collateral_rec.collateral;
                    });

            create<call_order_object>([&](call_order_object& c) {
               c.borrower = owner_account_id;
               c.collateral = collateral_rec.collateral;
               c.debt = collateral_rec.debt;
               c.call_price = price::call_price(chain::asset(c.debt, new_asset_id),
                                                chain::asset(c.collateral, core_asset.id),
                                                GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
            });

            total_supplies[ asset_id_type(0) ] += collateral_rec.collateral;
            total_delnk[ new_asset_id ] += collateral_rec.debt;
            ++collateral_holder_number;
         }

         bitasset_data_id = create<asset_bitasset_data_object>([&](asset_bitasset_data_object& b) {
            b.options.short_backing_asset = core_asset.id;
            b.options.minimum_feeds = GRAPHENE_DEFAULT_MINIMUM_FEEDS;
         }).id;
      }

      dynamic_data_id = create<asset_dynamic_data_object>([&](asset_dynamic_data_object& d) {
         d.accumulated_fees = asset.accumulated_fees;
      }).id;

      total_supplies[ new_asset_id ] += asset.accumulated_fees;

      create<asset_object>([&](asset_object& a) {
         a.symbol = asset.symbol;
         a.options.description = asset.description;
         a.precision = asset.precision;
         string issuer_name = asset.issuer_name;
         a.issuer = get_account_id(issuer_name);
         a.options.max_supply = asset.max_supply;
         a.options.flags = witness_fed_asset;
         a.options.issuer_permissions = charge_market_fee | override_authority | white_list | transfer_restricted | disable_confidential |
                                       ( asset.is_bitasset ? disable_force_settle | global_settle | witness_fed_asset | committee_fed_asset : 0 );
         a.dynamic_asset_data_id = dynamic_data_id;
         a.bitasset_data_id = bitasset_data_id;
      });
   }

   // Create initial balances
   share_type total_allocation;
   for( const auto& handout : genesis_state.initial_balances )
   {
      const auto asset_id = get_asset_id(handout.asset_symbol);
      create<balance_object>([&handout,&get_asset_id,total_allocation,asset_id](balance_object& b) {
         b.balance = asset(handout.amount, asset_id);
         b.owner = handout.owner;
      });

      total_supplies[ asset_id ] += handout.amount;
   }
   /*
   //create vote
   create<proposal_object>([&](proposal_object& obj) {
	   obj.proposed_transaction = transaction();
	   obj.expiration_time = head_block_time();
   });
   */
   // Create initial vesting balances
   for( const genesis_state_type::initial_vesting_balance_type& vest : genesis_state.initial_vesting_balances )
   {
      const auto asset_id = get_asset_id(vest.asset_symbol);
      create<balance_object>([&](balance_object& b) {
         b.owner = vest.owner;
         b.balance = asset(vest.amount, asset_id);

         linear_vesting_policy policy;
         policy.begin_timestamp = vest.begin_timestamp;
         policy.vesting_cliff_seconds = 0;
         policy.vesting_duration_seconds = vest.vesting_duration_seconds;
         policy.begin_balance = vest.begin_balance;

         b.vesting_policy = std::move(policy);
      });

      total_supplies[ asset_id ] += vest.amount;
   }

   /*if( total_supplies[ asset_id_type(0) ] > 0 )
   {
       adjust_balance(GRAPHENE_GUARD_ACCOUNT, -get_balance(GRAPHENE_GUARD_ACCOUNT,{}));
   }
   else
   {
       total_supplies[ asset_id_type(0) ] = GRAPHENE_MAX_SHARE_SUPPLY;
   }
*/
   const auto& idx = get_index_type<asset_index>().indices().get<by_symbol>();
   auto it = idx.begin();
   bool has_imbalanced_assets = false;

   while( it != idx.end() )
   {
      if( it->bitasset_data_id.valid() )
      {
         auto supply_itr = total_supplies.find( it->id );
         auto debt_itr = total_delnk.find( it->id );
         FC_ASSERT( supply_itr != total_supplies.end() );
         FC_ASSERT( debt_itr != total_delnk.end() );
         if( supply_itr->second != debt_itr->second )
         {
            has_imbalanced_assets = true;
            elog( "Genesis for asset ${aname} is not balanced\n"
                  "   Debt is ${debt}\n"
                  "   Supply is ${supply}\n",
                  ("debt", debt_itr->second)
                  ("supply", supply_itr->second)
                );
         }
      }
      ++it;
   }
   FC_ASSERT( !has_imbalanced_assets );

   // Save tallied supplies
   for( const auto& item : total_supplies )
   {
       const auto asset_id = item.first;
       const auto total_supply = item.second;

       modify( get( asset_id ), [ & ]( asset_object& asset ) {
           modify( get( asset.dynamic_asset_data_id ), [ & ]( asset_dynamic_data_object& asset_data ) {
               asset_data.current_supply = total_supply;
           } );
       } );
   }

   // Create special witness account
   const miner_object& wit = create<miner_object>([&](miner_object& w) {});
   FC_ASSERT( wit.id == GRAPHENE_NULL_WITNESS );
   remove(wit);

   // Create initial miners
   std::for_each(genesis_state.initial_miner_candidates.begin(), genesis_state.initial_miner_candidates.end(),
                 [&](const genesis_state_type::initial_miner_type& witness) {
      miner_create_operation op;
      op.miner_account = get_account_id(witness.owner_name);
	  op.miner_address = get_account_address(witness.owner_name);
      op.block_signing_key = witness.block_signing_key;
      apply_operation(genesis_eval_state, op);
   });

   // Create initial guard members
   std::for_each(genesis_state.initial_guard_candidates.begin(), genesis_state.initial_guard_candidates.end(),
                 [&](const genesis_state_type::initial_committee_member_type& member) {
	   guard_member_create_operation op;
	   op.guard_member_account = get_account_id(member.owner_name);
	   op.fee_pay_address = get_account_address(member.owner_name);
	   apply_operation(genesis_eval_state, op);
	   modify(*get_senator(member.owner_name), [&](guard_member_object& obj) {
		   obj.formal = true;
		   obj.senator_type = member.type;
	   });


	  modify(get(GRAPHENE_GUARD_ACCOUNT), [&](account_object& n) {
		  n.active.account_auths[get_account_id(member.owner_name)] = 100;
	  
	  });
   });

   // Create initial workers
   std::for_each(genesis_state.initial_worker_candidates.begin(), genesis_state.initial_worker_candidates.end(),
                  [&](const genesis_state_type::initial_worker_type& worker)
   {
       worker_create_operation op;
       op.owner = get_account_id(worker.owner_name);
       op.work_begin_date = genesis_state.initial_timestamp;
       op.work_end_date = time_point_sec::maximum();
       op.daily_pay = worker.daily_pay;
       op.name = "Genesis-Worker-" + worker.owner_name;
       op.initializer = vesting_balance_worker_initializer{uint16_t(0)};

       apply_operation(genesis_eval_state, std::move(op));
   });

   // Set active witnesses
   modify(get_global_properties(), [&](global_property_object& p) {
      for( uint32_t i = 1; i <= genesis_state.initial_active_miners; ++i )
      {
         p.active_witnesses.insert(miner_id_type(i));
      }
   });

   // Enable fees
   modify(get_global_properties(), [&genesis_state](global_property_object& p) {
      p.parameters.current_fees = genesis_state.initial_parameters.current_fees;
   });

   // Create witness scheduler
   create<witness_schedule_object>([&]( witness_schedule_object& wso )
   {
      for( const miner_id_type& wid : get_global_properties().active_witnesses )
         wso.current_shuffled_miners.push_back( wid );
   });

   // Create FBA counters
   create<fba_accumulator_object>([&]( fba_accumulator_object& acc )
   {
      FC_ASSERT( acc.id == fba_accumulator_id_type( fba_accumulator_id_transfer_to_blind ) );
      acc.accumulated_fba_fees = 0;
#ifdef GRAPHENE_FBA_STEALTH_DESIGNATED_ASSET
      acc.designated_asset = GRAPHENE_FBA_STEALTH_DESIGNATED_ASSET;
#endif
   });

   create<fba_accumulator_object>([&]( fba_accumulator_object& acc )
   {
      FC_ASSERT( acc.id == fba_accumulator_id_type( fba_accumulator_id_blind_transfer ) );
      acc.accumulated_fba_fees = 0;
#ifdef GRAPHENE_FBA_STEALTH_DESIGNATED_ASSET
      acc.designated_asset = GRAPHENE_FBA_STEALTH_DESIGNATED_ASSET;
#endif
   });

   create<fba_accumulator_object>([&]( fba_accumulator_object& acc )
   {
      FC_ASSERT( acc.id == fba_accumulator_id_type( fba_accumulator_id_transfer_from_blind ) );
      acc.accumulated_fba_fees = 0;
#ifdef GRAPHENE_FBA_STEALTH_DESIGNATED_ASSET
      acc.designated_asset = GRAPHENE_FBA_STEALTH_DESIGNATED_ASSET;
#endif
   });

   FC_ASSERT( get_index<fba_accumulator_object>().get_next_id() == fba_accumulator_id_type( fba_accumulator_id_count ) );

   debug_dump();

   _undo_db.enable();
} FC_CAPTURE_AND_RETHROW() }

} }
