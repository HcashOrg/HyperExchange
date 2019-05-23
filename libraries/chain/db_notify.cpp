#include <fc/container/flat.hpp>

#include <graphene/chain/protocol/authority.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/worker_object.hpp>
#include <graphene/chain/confidential_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/native_contract.hpp>

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

   void operator()( const account_whitelist_operation& op )
   {
      _impacted.insert( op.account_to_list );
   }

   void operator()( const account_upgrade_operation& op ) {}
   void operator()( const account_transfer_operation& op )
   {
      _impacted.insert( op.new_owner );
   }

   void operator()(const account_bind_operation& op) {}
   void operator()(const account_unbind_operation& op) {}
   void operator()(const account_multisig_create_operation& op) {}

   void operator()( const asset_create_operation& op ) {}
   void operator()( const asset_update_operation& op )
   {
   
   }
   void operator() (const sign_multisig_asset_operation& op) {}
   void operator()( const asset_update_bitasset_operation& op ) {}
   void operator()( const asset_update_feed_producers_operation& op ) {}
   void operator()(const block_address_operation& op) {}
   void operator()(const cancel_address_block_operation& op) {}
   void operator()( const asset_issue_operation& op )
   {
      _impacted.insert( op.issue_to_account );
   }
   void operator()(const set_balance_operation& op) {}
   void operator()(const correct_chain_data_operation& op) {}
   void operator()(const vote_create_operation& op) {}
   void operator()(const vote_update_operation& op) {}
   void operator()(const add_whiteOperation_list_operation& op) {}
   void operator()(const cancel_whiteOperation_list_operation& op) {}
   void operator()( const asset_reserve_operation& op ) {}
   void operator()( const asset_fund_fee_pool_operation& op ) {}
   void operator()( const asset_settle_operation& op ) {}
   void operator()( const asset_global_settle_operation& op ) {}
   void operator()( const asset_publish_feed_operation& op ) {}
   void operator() (const publisher_appointed_operation& op) {}
   void operator() (const publisher_canceled_operation& op) {}
   void operator()(const normal_asset_publish_feed_operation& op) {}
   void operator()(const lockbalance_operation& op) {}
   void operator()(const foreclose_balance_operation& op) {}
   void operator()(const guard_lock_balance_operation& op) {}
   void operator()(const pay_back_operation& op) {}
   void operator()(const bonus_operation& op) {}
   void operator()(const set_guard_lockbalance_operation& op) {}
   void operator()(const guard_foreclose_balance_operation& op) {}
   void operator() (const asset_fee_modification_operation& op) {}
   void operator()(const crosschain_record_operation& op) {}
   void operator()(const crosschain_withdraw_operation& op) {}
   void operator()(const crosschain_withdraw_without_sign_operation& op) {}
   void operator()(const crosschain_withdraw_with_sign_operation& op) {}
   void operator()(const crosschain_withdraw_combine_sign_operation& op) {}
   void operator()(const crosschain_withdraw_result_operation& op) {}
   void operator()(const coldhot_transfer_operation& op) {}
   void operator()(const coldhot_transfer_without_sign_operation& op) {}
   void operator()(const coldhot_transfer_with_sign_operation& op) {}
   void operator()(const coldhot_transfer_combine_sign_operation& op) {}
   void operator()(const coldhot_transfer_result_operation& op) {}
   void operator()(const coldhot_cancel_transafer_transaction_operation& op) {}
   void operator()(const coldhot_cancel_uncombined_trx_operaion& op) {}
   void operator() (const eths_guard_change_signer_operation& op){}
   void operator() (const eths_guard_coldhot_change_signer_operation& op){}
   void operator()(const eth_cancel_coldhot_fail_trx_operaion& op) {}
   void operator()(const guard_update_multi_account_operation& op) {}
   void operator()(const miner_generate_multi_asset_operation& op) {}
   void operator() (const miner_merge_signatures_operation& op) {}
   void operator() (const asset_transfer_from_cold_to_hot_operation& op) {}
   void operator()(const guard_refund_balance_operation& op) {}
   void operator()(const guard_refund_crosschain_trx_operation& op) {}
   void operator()(const eth_cancel_fail_crosschain_trx_operation& op){}
   void operator() (const referendum_accelerate_pledge_operation& op) {}
   void operator()(const asset_real_create_operation& op) {}
   void operator()(const asset_eth_create_operation&op){}
   void operator()(const gurantee_create_operation& op) {}
   void operator()(const gurantee_cancel_operation& op) {}
   void operator()(const senator_determine_withdraw_deposit_operation& op) {}
   void operator()(const account_create_multisignature_address_operation& op) {}
   void operator()(const senator_determine_block_payment_operation& op) {}
   void operator()(const citizen_referendum_senator_operation& op) {}
   void operator() (const coldhot_cancel_combined_trx_operaion & op) {}
   void operator() (const guard_cancel_combine_trx_operation & op) {}
   void operator() (const senator_pass_success_trx_operation & op) {}
   void operator() (const coldhot_pass_combine_trx_operation & op) {}
   void operator() (const senator_change_eth_gas_price_operation& op) {}
   void operator() (const eths_cancel_unsigned_transaction_operation & op) {}
   void operator() (const senator_change_acquire_trx_operation& op) {}
   void operator()( const miner_create_operation& op )
   {
      _impacted.insert( op.miner_account );
   }
   void operator()( const witness_update_operation& op )
   {
      _impacted.insert( op.witness_account );
   }
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
   void operator() (const referendum_update_operation& op) {}
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
   void operator()( const guard_member_update_operation& op ){}
   void operator()( const committee_member_update_global_parameters_operation& op ) {}
   void operator()(const guard_member_resign_operation& op)
   {
       _impacted.insert(op.guard_member_account);
   }

   void operator()(const committee_member_execute_coin_destory_operation& op) {}

   void operator()( const vesting_balance_create_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const vesting_balance_withdraw_operation& op ) {}
   void operator()( const worker_create_operation& op ) {}
   void operator()( const custom_operation& op ) {}
   void operator()( const assert_operation& op ) {}
   void operator()( const balance_claim_operation& op ) {}

   void operator()( const override_transfer_operation& op )
   {
      _impacted.insert( op.to );
      _impacted.insert( op.from );
      _impacted.insert( op.issuer );
   }

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
   void operator()(const eth_seri_guard_sign_operation & op){}
   void operator()(const eths_guard_sign_final_operation & op) {}
   void operator()(const eth_series_multi_sol_create_operation & op) {}
   void operator()(const eths_multi_sol_guard_sign_operation & op) {}
   void operator()(const eth_multi_account_create_record_operation & op) {}
   void operator()(const eths_coldhot_guard_sign_final_operation & op) {}

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

void get_relevant_accounts( const object* obj, flat_set<account_id_type>& accounts )
{
   if( obj->id.space() == protocol_ids )
   {
      switch( (object_type)obj->id.type() )
      {
        case null_object_type:
        case base_object_type:
        case OBJECT_TYPE_COUNT:
           return;
        case account_object_type:{
           accounts.insert( obj->id );
           break;
        } case asset_object_type:{
           const auto& aobj = dynamic_cast<const asset_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->issuer );
           break;
        } case force_settlement_object_type:{
           const auto& aobj = dynamic_cast<const force_settlement_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->owner );
           break;
        } case guard_member_object_type:{
           const auto& aobj = dynamic_cast<const guard_member_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->guard_member_account );
           break;
        } case miner_object_type:{
           const auto& aobj = dynamic_cast<const miner_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->miner_account );
           break;
        } case limit_order_object_type:{
           const auto& aobj = dynamic_cast<const limit_order_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->seller );
           break;
        } case call_order_object_type:{
           const auto& aobj = dynamic_cast<const call_order_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->borrower );
           break;
        } case custom_object_type:{
          break;
        } case proposal_object_type:{
           const auto& aobj = dynamic_cast<const proposal_object*>(obj);
           assert( aobj != nullptr );
           transaction_get_impacted_accounts( aobj->proposed_transaction, accounts );
           break;
        } case operation_history_object_type:{
           const auto& aobj = dynamic_cast<const operation_history_object*>(obj);
           assert( aobj != nullptr );
           operation_get_impacted_accounts( aobj->op, accounts );
           break;
        } case withdraw_permission_object_type:{
           const auto& aobj = dynamic_cast<const withdraw_permission_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->withdraw_from_account );
           accounts.insert( aobj->authorized_account );
           break;
        } case vesting_balance_object_type:{
           const auto& aobj = dynamic_cast<const vesting_balance_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->owner );
           break;
        } case worker_object_type:{
           const auto& aobj = dynamic_cast<const worker_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->worker_account );
           break;
        } case balance_object_type:{
           /** these are free from any accounts */
           break;
        }
      }
   }
   else if( obj->id.space() == implementation_ids )
   {
      switch( (impl_object_type)obj->id.type() )
      {
             case impl_global_property_object_type:
              break;
             case impl_dynamic_global_property_object_type:
              break;
             case impl_reserved0_object_type:
              break;
             case impl_asset_dynamic_data_type:
              break;
             case impl_asset_bitasset_data_type:
              break;
             case impl_account_balance_object_type:{
              const auto& aobj = dynamic_cast<const account_balance_object*>(obj);
              assert( aobj != nullptr );
              accounts.insert( aobj->owner );
              break;
           } case impl_account_statistics_object_type:{
              const auto& aobj = dynamic_cast<const account_statistics_object*>(obj);
              assert( aobj != nullptr );
              accounts.insert( aobj->owner );
              break;
           } case impl_transaction_object_type:{
              const auto& aobj = dynamic_cast<const transaction_object*>(obj);
              assert( aobj != nullptr );
              transaction_get_impacted_accounts( aobj->trx, accounts );
              break;
           } case impl_blinded_balance_object_type:{
              const auto& aobj = dynamic_cast<const blinded_balance_object*>(obj);
              assert( aobj != nullptr );
              for( const auto& a : aobj->owner.account_auths )
                accounts.insert( a.first );
              break;
           } case impl_block_summary_object_type:
              break;
             case impl_account_transaction_history_object_type:
              break;
             case impl_chain_property_object_type:
              break;
             case impl_witness_schedule_object_type:
              break;
             case impl_budget_record_object_type:
              break;
             case impl_special_authority_object_type:
              break;
             case impl_buyback_object_type:
              break;
             case impl_fba_accumulator_object_type:
              break;
      }
   }
} // end get_relevant_accounts( const object* obj, flat_set<account_id_type>& accounts )

namespace graphene { namespace chain {

void database::notify_changed_objects()
{ try {
   if( _undo_db.enabled() ) 
   {
      const auto& head_undo = _undo_db.head();

      // New
      if( !new_objects.empty() )
      {
        vector<object_id_type> new_ids;  new_ids.reserve(head_undo.new_ids.size());
        flat_set<account_id_type> new_accounts_impacted;
        for( const auto& item : head_undo.new_ids )
        {
          new_ids.push_back(item);
          auto obj = find_object(item);
          if(obj != nullptr)
            get_relevant_accounts(obj, new_accounts_impacted);
        }

        new_objects(new_ids, new_accounts_impacted);
      }

      // Changed
      if( !changed_objects.empty() )
      {
        vector<object_id_type> changed_ids;  changed_ids.reserve(head_undo.old_values.size());
        flat_set<account_id_type> changed_accounts_impacted;
        for( const auto& item : head_undo.old_values )
        {
          changed_ids.push_back(item.first);
          get_relevant_accounts(item.second.get(), changed_accounts_impacted);
        }

        changed_objects(changed_ids, changed_accounts_impacted);
      }

      // Removed
      if( !removed_objects.empty() )
      {
        vector<object_id_type> removed_ids; removed_ids.reserve( head_undo.removed.size() );
        vector<const object*> removed; removed.reserve( head_undo.removed.size() );
        flat_set<account_id_type> removed_accounts_impacted;
        for( const auto& item : head_undo.removed )
        {
          removed_ids.emplace_back( item.first );
          auto obj = item.second.get();
          removed.emplace_back( obj );
          get_relevant_accounts(obj, removed_accounts_impacted);
        }

        removed_objects(removed_ids, removed, removed_accounts_impacted);
      }
   }
} FC_CAPTURE_AND_LOG( (0) ) }

} }
