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

#include <graphene/chain/protocol/authority.hpp>
#include <graphene/app/impacted.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/native_contract.hpp>

namespace graphene { namespace app {

using namespace fc;
using namespace graphene::chain;

// TODO:  Review all of these, especially no-ops
struct get_impacted_account_visitor
{
   flat_set<account_id_type>& _impacted;
   get_impacted_account_visitor( flat_set<account_id_type>& impact ):_impacted(impact) {}
   typedef void result_type;

   void operator()( const transfer_operation& op )
   {
      _impacted.insert( op.to );
   }

   void operator()( const asset_claim_fees_operation& op ){}
   void operator()( const limit_order_create_operation& op ) {}
   void operator()( const limit_order_cancel_operation& op )
   {
      _impacted.insert( op.fee_paying_account );
   }
   void operator()( const call_order_update_operation& op ) {}
   void operator()( const fill_order_operation& op )
   {
      _impacted.insert( op.account_id );
   }

   void operator()( const account_create_operation& op )
   {
      _impacted.insert( op.registrar );
      _impacted.insert( op.referrer );
      add_authority_accounts( _impacted, op.owner );
      add_authority_accounts( _impacted, op.active );
   }

   void operator()( const account_update_operation& op )
   {
      _impacted.insert( op.account );
      if( op.owner )
         add_authority_accounts( _impacted, *(op.owner) );
      if( op.active )
         add_authority_accounts( _impacted, *(op.active) );
   }
   void operator() (const correct_chain_data_operation& op) {};
   void operator() (const vote_create_operation& op) {};
   void operator() (const vote_update_operation& op) {};
   void operator()( const account_whitelist_operation& op )
   {
      _impacted.insert( op.account_to_list );
   }

   void operator()( const account_upgrade_operation& op ) {}
   void operator()( const account_transfer_operation& op )
   {
      _impacted.insert( op.new_owner );
   }
   void operator() (const senator_determine_block_payment_operation& op) {}
   void operator()(const account_bind_operation& op) {}
   void operator()(const account_unbind_operation& op) {}
   void operator()(const account_multisig_create_operation& op) {}
   void operator()(const block_address_operation& op) {}
   void operator() (const cancel_address_block_operation& op) {}
   void operator()( const asset_create_operation& op ) {}
   void operator()( const asset_update_operation& op )
   {
   }
   void operator()(const add_whiteOperation_list_operation& op) {}
   void operator()(const cancel_whiteOperation_list_operation& op) {}
   void operator()(const set_balance_operation& op) {}

   void operator()( const asset_update_bitasset_operation& op ) {}
   void operator()( const asset_update_feed_producers_operation& op ) {}

   void operator()( const asset_issue_operation& op )
   {
      _impacted.insert( op.issue_to_account );
   }

   void operator()( const asset_reserve_operation& op ) {}
   void operator()(const asset_real_create_operation& op) {}
   void operator()(const asset_eth_create_operation& op) {}
   void operator()( const asset_fund_fee_pool_operation& op ) {}
   void operator()( const asset_settle_operation& op ) {}
   void operator()( const asset_global_settle_operation& op ) {}
   void operator()( const asset_publish_feed_operation& op ) {}
   void operator()(const normal_asset_publish_feed_operation& op) {}
   void operator()( const miner_create_operation& op )
   {
      _impacted.insert( op.miner_account );
   }
   void operator() (const miner_generate_multi_asset_operation& op) {}
   void operator() (const miner_merge_signatures_operation& op) {}
   void operator()( const witness_update_operation& op )
   {
      _impacted.insert( op.witness_account );
   }
   void operator() (const referendum_accelerate_pledge_operation& op) {}
   void operator() (const undertaker_operation& op) {
	   vector<authority> other;
	   for (const auto& mop : op.maker_op)
		   operation_get_required_authorities(mop.op, _impacted, _impacted, other);
	   for (const auto& top : op.taker_op)
		   operation_get_required_authorities(top.op, _impacted, _impacted, other);
	   for (auto& o : other)
		   add_authority_accounts(_impacted, o);
   }
   void operator()(const name_transfer_operation& op) {}
   void operator()( const proposal_create_operation& op )
   {
      vector<authority> other;
      for( const auto& proposed_op : op.proposed_ops )
         operation_get_required_authorities( proposed_op.op, _impacted, _impacted, other );
      for( auto& o : other )
         add_authority_accounts( _impacted, o );
   }

   void operator()(const referendum_create_operation& op)
   {
	   vector<authority> other;
	   for (const auto& proposed_op : op.proposed_ops)
		   operation_get_required_authorities(proposed_op.op, _impacted, _impacted, other);
	   for (auto& o : other)
		   add_authority_accounts(_impacted, o);
   }
   void operator()(const referendum_update_operation& op) {}
   void operator()( const proposal_update_operation& op ) {}
   void operator()( const proposal_delete_operation& op ) {}

   void operator()( const withdraw_permission_create_operation& op )
   {
      _impacted.insert( op.authorized_account );
   }

   void operator()( const withdraw_permission_update_operation& op )
   {
      _impacted.insert( op.authorized_account );
   }

   void operator()( const withdraw_permission_claim_operation& op )
   {
      _impacted.insert( op.withdraw_from_account );
   }

   void operator()( const withdraw_permission_delete_operation& op )
   {
      _impacted.insert( op.authorized_account );
   }

   void operator()( const guard_member_create_operation& op )
   {
      _impacted.insert( op.guard_member_account );
   }
   void operator()( const guard_member_update_operation& op ){ }
   void operator()( const committee_member_update_global_parameters_operation& op ) {}
   void operator()(const committee_member_execute_coin_destory_operation& op) {}
   void operator()(const guard_member_resign_operation& op)
   {
       _impacted.insert(op.guard_member_account);
   }

   void operator()( const vesting_balance_create_operation& op )
   {
      _impacted.insert( op.owner );
   }
   void operator()(const guard_refund_balance_operation& op) {}
   void operator()(const guard_refund_crosschain_trx_operation& op) {}
   void operator()(const eth_cancel_fail_crosschain_trx_operation& op){}
   void operator()( const vesting_balance_withdraw_operation& op ) {}
   void operator()( const worker_create_operation& op ) {}
   void operator()( const custom_operation& op ) {}
   void operator()( const assert_operation& op ) {}
   void operator()(const lockbalance_operation& op) {}
   void operator()(const foreclose_balance_operation& op) {}
   void operator()(const guard_lock_balance_operation& op) {}
   void operator()(const guard_foreclose_balance_operation& op) {}
   void operator()(const crosschain_record_operation& op) {}
   void operator()(const crosschain_withdraw_operation& op) {}
   void operator()(const pay_back_operation& op) {}
   void operator()(const bonus_operation& op) {}
   void operator()(const set_guard_lockbalance_operation& op) {}
   void operator()(const crosschain_withdraw_without_sign_operation& op) {}
   void operator()(const crosschain_withdraw_with_sign_operation& op) {}
   void operator()(const crosschain_withdraw_combine_sign_operation& op) {}
   void operator()(const crosschain_withdraw_result_operation& op) {}
   void operator()(const guard_update_multi_account_operation& op) {}
   void operator()(const coldhot_transfer_operation& op) {}
   void operator()(const coldhot_transfer_without_sign_operation& op) {}
   void operator()(const coldhot_transfer_with_sign_operation& op) {}
   void operator()(const coldhot_transfer_combine_sign_operation& op) {}
   void operator()(const coldhot_transfer_result_operation& op) {}
   void operator()(const coldhot_cancel_transafer_transaction_operation& op){}
   void operator()(const coldhot_cancel_uncombined_trx_operaion& op) {}
   void operator()(const eth_cancel_coldhot_fail_trx_operaion& op) {}
   void operator()( const balance_claim_operation& op ) {}
   void operator() (const gurantee_create_operation& op) {}
   void operator() (const publisher_appointed_operation& op) {}
   void operator() (const publisher_canceled_operation& op) {}
   void operator() (const gurantee_cancel_operation& op) {}
   void operator() (const asset_fee_modification_operation& op) {}
   void operator() (const senator_determine_withdraw_deposit_operation& op) {}
   void operator() (const account_create_multisignature_address_operation& op) {}
   void operator() (const eths_guard_change_signer_operation& op){}
   void operator() (const eths_guard_coldhot_change_signer_operation & op){}
   void operator() (const coldhot_cancel_combined_trx_operaion & op) {}
   void operator() (const guard_cancel_combine_trx_operation & op) {}
   void operator() (const senator_pass_success_trx_operation & op) {}
   void operator() (const coldhot_pass_combine_trx_operation & op) {}
   void operator() (const senator_change_eth_gas_price_operation& op) {}
   void operator() (const eths_cancel_unsigned_transaction_operation & op) {}
   void operator() (const senator_change_acquire_trx_operation& op) {}
   void operator()( const override_transfer_operation& op )
   {
      _impacted.insert( op.to );
      _impacted.insert( op.from );
      _impacted.insert( op.issuer );
   }

   void operator() (const asset_transfer_from_cold_to_hot_operation& op) {}
   void operator() (const sign_multisig_asset_operation& op) {}

   void operator()( const transfer_to_blind_operation& op )
   {
      _impacted.insert( op.from );
      for( const auto& out : op.outputs )
         add_authority_accounts( _impacted, out.owner );
   }

   void operator()( const blind_transfer_operation& op )
   {
      for( const auto& in : op.inputs )
         add_authority_accounts( _impacted, in.owner );
      for( const auto& out : op.outputs )
         add_authority_accounts( _impacted, out.owner );
   }

   void operator()( const transfer_from_blind_operation& op )
   {
      _impacted.insert( op.to );
      for( const auto& in : op.inputs )
         add_authority_accounts( _impacted, in.owner );
   }

   void operator()( const asset_settle_cancel_operation& op )
   {
      _impacted.insert( op.account );
   }

   void operator()( const fba_distribute_operation& op )
   {
      _impacted.insert( op.account_id );
   }

   void operator()(const contract_register_operation& op) {}
   void operator()(const contract_upgrade_operation& op) {}
   void operator()(const native_contract_register_operation& op) {}
   void operator()(const contract_invoke_operation& op) {}
   void operator()(const storage_operation& op) {}
   void operator()(const transfer_contract_operation& op) {}
   void operator()(const contract_transfer_fee_proposal_operation& op) {}
   void operator()(const citizen_referendum_senator_operation& op) {}

   void operator()(const eth_seri_guard_sign_operation & op){}
   void operator()(const eths_guard_sign_final_operation & op) {}
   void operator()(const eths_coldhot_guard_sign_final_operation & op) {}
   void operator()(const eth_series_multi_sol_create_operation & op) {}
   void operator()(const eths_multi_sol_guard_sign_operation & op) {}
   void operator()(const eth_multi_account_create_record_operation & op) {}
};

void operation_get_impacted_accounts( const operation& op, flat_set<account_id_type>& result )
{
   get_impacted_account_visitor vtor = get_impacted_account_visitor( result );
   op.visit( vtor );
}

void transaction_get_impacted_accounts( const transaction& tx, flat_set<account_id_type>& result )
{
   for( const auto& op : tx.operations )
      operation_get_impacted_accounts( op, result );
}

} }
