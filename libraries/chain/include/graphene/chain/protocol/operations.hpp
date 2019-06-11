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
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/lockbalance.hpp>
#include <graphene/chain/crosschain_record.hpp>
#include <graphene/chain/coldhot_withdraw.hpp>
#include <graphene/chain/protocol/account.hpp>
#include <graphene/chain/protocol/assert.hpp>
#include <graphene/chain/protocol/asset_ops.hpp>
#include <graphene/chain/protocol/balance.hpp>
#include <graphene/chain/protocol/custom.hpp>
#include <graphene/chain/protocol/committee_member.hpp>
#include <graphene/chain/protocol/confidential.hpp>
#include <graphene/chain/protocol/fba.hpp>
#include <graphene/chain/protocol/market.hpp>
#include <graphene/chain/protocol/proposal.hpp>
#include <graphene/chain/protocol/referendum.hpp>
#include <graphene/chain/protocol/transfer.hpp>
#include <graphene/chain/protocol/vesting.hpp>
#include <graphene/chain/protocol/withdraw_permission.hpp>
#include <graphene/chain/protocol/witness.hpp>
#include <graphene/chain/protocol/worker.hpp>
#include <graphene/chain/protocol/guard_lock_balance.hpp>
#include <graphene/chain/protocol/guard_refund_balance.hpp>
#include <graphene/chain/pay_back.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/native_contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/eth_seri_record.hpp>

namespace graphene { namespace chain {

   /**
    * @ingroup operations
    *
    * Defines the set of valid operations as a discriminated union type.
    */
   typedef fc::static_variant<
            transfer_operation,
            limit_order_create_operation,
            limit_order_cancel_operation,
            call_order_update_operation,
            fill_order_operation,           // VIRTUAL
            account_create_operation,
            account_update_operation,
            account_whitelist_operation,
            account_upgrade_operation,
            account_transfer_operation,
			account_bind_operation,
			account_unbind_operation,
			account_multisig_create_operation,
			asset_create_operation,
            asset_update_operation,
            asset_update_bitasset_operation,
            asset_update_feed_producers_operation,
            asset_issue_operation,
            asset_reserve_operation,
            asset_fund_fee_pool_operation,
            asset_settle_operation,
            asset_global_settle_operation,
            asset_publish_feed_operation,
			normal_asset_publish_feed_operation,
            miner_create_operation,
            witness_update_operation,
	        miner_generate_multi_asset_operation,
	        miner_merge_signatures_operation,
            proposal_create_operation,
            proposal_update_operation,
            proposal_delete_operation,
            withdraw_permission_create_operation,
            withdraw_permission_update_operation,
            withdraw_permission_claim_operation,
            withdraw_permission_delete_operation,
            guard_member_create_operation,
            guard_member_update_operation,
            committee_member_update_global_parameters_operation,
            guard_member_resign_operation,
            vesting_balance_create_operation,
            vesting_balance_withdraw_operation,
            worker_create_operation,
            custom_operation,
            assert_operation,
            balance_claim_operation,
            override_transfer_operation,
	        asset_transfer_from_cold_to_hot_operation,
	        sign_multisig_asset_operation,
            transfer_to_blind_operation,
            blind_transfer_operation,
            transfer_from_blind_operation,
            asset_settle_cancel_operation,  // VIRTUAL
            asset_claim_fees_operation,
            fba_distribute_operation,        // VIRTUAL
			committee_member_execute_coin_destory_operation,
		    lockbalance_operation,
		    foreclose_balance_operation,
			guard_lock_balance_operation,
			guard_foreclose_balance_operation,
	        guard_refund_balance_operation,
			crosschain_record_operation,
	        crosschain_withdraw_operation,
	        crosschain_withdraw_without_sign_operation,
	        crosschain_withdraw_with_sign_operation,
	        crosschain_withdraw_combine_sign_operation,
	        crosschain_withdraw_result_operation,
	        coldhot_transfer_operation,
	        coldhot_transfer_without_sign_operation,
	        coldhot_transfer_with_sign_operation,
	        coldhot_transfer_combine_sign_operation,
	        coldhot_transfer_result_operation,
	        coldhot_cancel_transafer_transaction_operation,
	        coldhot_cancel_uncombined_trx_operaion,
	        pay_back_operation,
	        guard_update_multi_account_operation,
	        asset_real_create_operation,
	        contract_register_operation,
	        contract_upgrade_operation,
	        native_contract_register_operation,
	        contract_invoke_operation,
	        storage_operation,
	        transfer_contract_operation,
	        contract_transfer_fee_proposal_operation,
	        gurantee_create_operation,
	        gurantee_cancel_operation,
	        guard_refund_crosschain_trx_operation,
	        publisher_appointed_operation,
	        asset_fee_modification_operation,
	        bonus_operation,
	        set_guard_lockbalance_operation,
	        publisher_canceled_operation,
	        senator_determine_withdraw_deposit_operation,
	        account_create_multisignature_address_operation,
			senator_determine_block_payment_operation,
			graphene::chain::eth_seri_guard_sign_operation,
	        eth_series_multi_sol_create_operation,
	        eth_multi_account_create_record_operation,
	        eths_multi_sol_guard_sign_operation,
	        eths_guard_sign_final_operation,
	        asset_eth_create_operation,
	        eths_coldhot_guard_sign_final_operation,
			referendum_create_operation,
		    citizen_referendum_senator_operation,
			referendum_update_operation,
			referendum_accelerate_pledge_operation,
			block_address_operation,
		    cancel_address_block_operation,
			eth_cancel_fail_crosschain_trx_operation,
			eth_cancel_coldhot_fail_trx_operaion,
			eths_guard_change_signer_operation,
			eths_guard_coldhot_change_signer_operation,
			coldhot_cancel_combined_trx_operaion,
			guard_cancel_combine_trx_operation,
			senator_pass_success_trx_operation,
			coldhot_pass_combine_trx_operation,
			senator_change_eth_gas_price_operation,
			eths_cancel_unsigned_transaction_operation,
			senator_change_acquire_trx_operation,
			add_whiteOperation_list_operation,
			cancel_whiteOperation_list_operation,
			set_balance_operation,
		    correct_chain_data_operation,
			vote_create_operation,
			vote_update_operation,
			undertaker_operation,
			name_transfer_operation
         > operation;

   /// @} // operations group

   /**
    *  Appends required authorites to the result vector.  The authorities appended are not the
    *  same as those returned by get_required_auth 
    *
    *  @return a set of required authorities for @ref op
    */
   void operation_get_required_authorities( const operation& op, 
                                            flat_set<account_id_type>& active,
                                            flat_set<account_id_type>& owner,
                                            vector<authority>&  other );

   void operation_validate( const operation& op );
   fc::variant operation_fee_payer(const operation& op);
   bool is_contract_operation(const operation& op);
   optional<guarantee_object_id_type> operation_gurantee_id(const operation& op);
  
   /**
    *  @brief necessary to support nested operations inside the proposal_create_operation
    */
   struct op_wrapper
   {
      public:
         op_wrapper(const operation& op = operation()):op(op){}
         operation op;
   };

} } // graphene::chain

FC_REFLECT_TYPENAME( graphene::chain::operation )
FC_REFLECT( graphene::chain::op_wrapper, (op) )
