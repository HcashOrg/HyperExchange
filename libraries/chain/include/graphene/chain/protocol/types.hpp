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
#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/optional.hpp>
#include <fc/safe.hpp>
#include <fc/container/flat.hpp>
#include <fc/string.hpp>
#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>
#include <fc/static_variant.hpp>
#include <fc/smart_ref_fwd.hpp>

#include <memory>
#include <vector>
#include <deque>
#include <cstdint>
#include <graphene/chain/protocol/address.hpp>
#include <graphene/db/object_id.hpp>
#include <graphene/chain/protocol/config.hpp>
#include <graphene/chain/forks.hpp>

namespace graphene { namespace chain {
   using namespace graphene::db;

   using                               std::map;
   using                               std::vector;
   using                               std::unordered_map;
   using                               std::string;
   using                               std::deque;
   using                               std::shared_ptr;
   using                               std::weak_ptr;
   using                               std::unique_ptr;
   using                               std::set;
   using                               std::pair;
   using                               std::enable_shared_from_this;
   using                               std::tie;
   using                               std::make_pair;

   using                               fc::smart_ref;
   using                               fc::variant_object;
   using                               fc::variant;
   using                               fc::enum_type;
   using                               fc::optional;
   using                               fc::unsigned_int;
   using                               fc::signed_int;
   using                               fc::time_point_sec;
   using                               fc::time_point;
   using                               fc::safe;
   using                               fc::flat_map;
   using                               fc::flat_set;
   using                               fc::static_variant;
   using                               fc::ecc::range_proof_type;
   using                               fc::ecc::range_proof_info;
   using                               fc::ecc::commitment_type;
   struct void_t{};

   typedef fc::ecc::private_key        private_key_type;
   typedef fc::sha256 chain_id_type;

   typedef enum senatorType
   {
	   EXTERNAL,
	   PERMANENT
   };
   enum asset_issuer_permission_flags
   {
      charge_market_fee    = 0x01, /**< an issuer-specified percentage of all market trades in this asset is paid to the issuer */
      white_list           = 0x02, /**< accounts must be whitelisted in order to hold this asset */
      override_authority   = 0x04, /**< issuer may transfer asset back to himself */
      transfer_restricted  = 0x08, /**< require the issuer to be one party to every transfer */
      disable_force_settle = 0x10, /**< disable force settling */
      global_settle        = 0x20, /**< allow the bitasset issuer to force a global settling -- this may be set in permissions, but not flags */
      disable_confidential = 0x40, /**< allow the asset to be used with confidential transactions */
      witness_fed_asset    = 0x80, /**< allow the asset to be fed by witnesses */
      committee_fed_asset  = 0x100 /**< allow the asset to be fed by the committee */
   };
   const static uint32_t ASSET_ISSUER_PERMISSION_MASK = charge_market_fee|white_list|override_authority|transfer_restricted|disable_force_settle|global_settle|disable_confidential
      |witness_fed_asset|committee_fed_asset;
   const static uint32_t UIA_ASSET_ISSUER_PERMISSION_MASK = charge_market_fee|white_list|override_authority|transfer_restricted|disable_confidential;

   enum reserved_spaces
   {
      relative_protocol_ids = 0,
      protocol_ids          = 1,
      implementation_ids    = 2
   };

   inline bool is_relative( object_id_type o ){ return o.space() == 0; }

   /**
    *  List all object types from all namespaces here so they can
    *  be easily reflected and displayed in debug output.  If a 3rd party
    *  wants to extend the core code then they will have to change the
    *  packed_object::type field from enum_type to uint16 to avoid
    *  warnings when converting packed_objects to/from json.
    */
   enum object_type
   {
	   null_object_type,
	   base_object_type,
	   account_object_type,
	   asset_object_type,
	   force_settlement_object_type,
	   guard_member_object_type,
	   miner_object_type,
	   limit_order_object_type,
	   call_order_object_type,
	   custom_object_type,
	   proposal_object_type,
	   referendum_object_type,
	   operation_history_object_type,
	   withdraw_permission_object_type,
	   vesting_balance_object_type,
	   worker_object_type,
	   balance_object_type,
	   lockbalance_object_type,
	   crosschain_trx_object_type,
	   coldhot_transfer_object_type,
	   guard_lock_balance_object_type,
	   multisig_transfer_object_type,
	   acquired_crosschain_object_type,
	   crosschain_transaction_history_count_object_type,
	   contract_storage_diff_type,
	   contract_storage_type,
	   contract_object_type,
	   contract_balance_object_type,
	   contract_storage_object_type,
	   contract_event_notify_object_type,
	   contract_invoke_result_object_type,
	   script_object_type,
	   script_binding_object_type,
	   pay_back_object_type,
	   bonus_object_type,
	   total_fees_type,
	   contract_storage_change_object_type,
	   contract_history_object_type,
	   eth_multi_account_trx_object_type,
	   vote_object_type,
	   vote_result_object_type,
      OBJECT_TYPE_COUNT ///< Sentry value which contains the number of different object types
   };

   enum impl_object_type
   {
      impl_global_property_object_type,
      impl_dynamic_global_property_object_type,
      impl_reserved0_object_type,      // formerly index_meta_object_type, TODO: delete me
      impl_asset_dynamic_data_type,
      impl_asset_bitasset_data_type,
      impl_account_balance_object_type,
	  impl_account_binding_object_type,
	  impl_multisig_account_binding_object_type,
	  impl_multisig_address_object_type,
	  impl_account_statistics_object_type,
      impl_transaction_object_type,
	  impl_history_transaction_object_type,
	  impl_trx_object_type,
      impl_block_summary_object_type,
      impl_account_transaction_history_object_type,
      impl_blinded_balance_object_type,
      impl_chain_property_object_type,
      impl_witness_schedule_object_type,
      impl_budget_record_object_type,
      impl_special_authority_object_type,
      impl_buyback_object_type,
      impl_fba_accumulator_object_type,
	  impl_guarantee_obj_type,
	  impl_address_transaction_history_object_type,
	  impl_blocked_address_obj_type,
	  impl_lockbalance_record_object_type,
	  impl_white_operationlist_object_type
   };

   //typedef fc::unsigned_int            object_id_type;
   //typedef uint64_t                    object_id_type;
   class account_object;
   class guard_member_object;
   class miner_object;
   class asset_object;
   class force_settlement_object;
   class limit_order_object;
   class call_order_object;
   class custom_object;
   class proposal_object;
   class referendum_object;
   class operation_history_object;
   class withdraw_permission_object;
   class vesting_balance_object;
   class worker_object;
   class balance_object;
   class blinded_balance_object;
   class lockbalance_object;
   class guard_lock_balance_object;
   class multisig_asset_transfer_object;
   class crosschain_trx_object;
   class coldhot_transfer_object;
   class acquired_crosschain_trx_object;
   class crosschain_transaction_history_count_object;
   class contract_object;
   class contract_storage_object;
   class transaction_contract_storage_diff_object;
   class contract_event_notify_object;
   class contract_invoke_result_object;
   class pay_back_object;
   class bonus_object;
   class contract_history_object;
   class eth_multi_account_trx_object;
   class total_fees_object;
   class vote_object;
   class vote_result_object;
   typedef object_id< protocol_ids, account_object_type,            account_object>               account_id_type;
   typedef object_id< protocol_ids, asset_object_type,              asset_object>                 asset_id_type;
   typedef object_id< protocol_ids, force_settlement_object_type,   force_settlement_object>      force_settlement_id_type;
   typedef object_id< protocol_ids, guard_member_object_type,           guard_member_object>              guard_member_id_type;
   typedef object_id< protocol_ids, miner_object_type,            miner_object>               miner_id_type;
   typedef object_id< protocol_ids, limit_order_object_type,        limit_order_object>           limit_order_id_type;
   typedef object_id< protocol_ids, call_order_object_type,         call_order_object>            call_order_id_type;
   typedef object_id< protocol_ids, custom_object_type,             custom_object>                custom_id_type;
   typedef object_id< protocol_ids, proposal_object_type,           proposal_object>              proposal_id_type;
   typedef object_id<protocol_ids,  referendum_object_type,         referendum_object>            referendum_id_type;
   typedef object_id< protocol_ids, operation_history_object_type,  operation_history_object>     operation_history_id_type;
   typedef object_id< protocol_ids, withdraw_permission_object_type,withdraw_permission_object>   withdraw_permission_id_type;
   typedef object_id< protocol_ids, vesting_balance_object_type,    vesting_balance_object>       vesting_balance_id_type;
   typedef object_id< protocol_ids, worker_object_type,             worker_object>                worker_id_type;
   typedef object_id< protocol_ids, balance_object_type,            balance_object>               balance_id_type;
   typedef object_id<protocol_ids, lockbalance_object_type, lockbalance_object>			  lockbalance_id_type;
   typedef object_id<protocol_ids,crosschain_trx_object_type,crosschain_trx_object> crosschain_trx_id_type;
   typedef object_id< protocol_ids, guard_lock_balance_object_type, guard_lock_balance_object>	  guard_lock_balance_id_type;
   typedef object_id< protocol_ids, multisig_transfer_object_type, multisig_asset_transfer_object >          multisig_asset_transfer_id_type;
   typedef object_id<protocol_ids, acquired_crosschain_object_type, acquired_crosschain_trx_object> acquired_crosschain_id_type;
   typedef object_id<protocol_ids, crosschain_transaction_history_count_object_type, crosschain_transaction_history_count_object> transaction_history_count_id_type;
   typedef object_id<protocol_ids, coldhot_transfer_object_type, coldhot_transfer_object> coldhot_transfer_id_type;
   typedef object_id<protocol_ids, contract_object_type, contract_object> contract_id_type;
   typedef object_id<protocol_ids, contract_storage_object_type, contract_storage_object> contract_storage_id_type;
   typedef object_id<protocol_ids, contract_storage_diff_type, transaction_contract_storage_diff_object> transaction_contract_storage_diff_object_id_type;
   typedef object_id<protocol_ids, contract_event_notify_object_type, contract_event_notify_object> contract_event_notify_object_id_type;
   typedef object_id<protocol_ids, contract_invoke_result_object_type, contract_invoke_result_object> contract_invoke_result_object_id_type;
   typedef object_id<protocol_ids, pay_back_object_type, pay_back_object> pay_back_object_id_type;
   typedef object_id<protocol_ids, bonus_object_type, bonus_object> bonus_object_id_type;
   typedef object_id<protocol_ids, total_fees_type, total_fees_object> total_fees_object_id_type;
   typedef object_id<protocol_ids, contract_history_object_type, contract_history_object> contract_history_object_id_type;
   typedef object_id<protocol_ids, eth_multi_account_trx_object_type, eth_multi_account_trx_object> eth_multi_account_id_type;
   typedef object_id<protocol_ids, vote_object_type, vote_object> vote_object_id_type;
   typedef object_id<protocol_ids, vote_result_object_type, vote_result_object> vote_result_object_id_type;
   // implementation types
   class global_property_object;
   class dynamic_global_property_object;
   class asset_dynamic_data_object;
   class asset_bitasset_data_object;
   class account_balance_object;
   class account_statistics_object;
   class transaction_object;
   class block_summary_object;
   class account_transaction_history_object;
   class chain_property_object;
   class witness_schedule_object;
   class budget_record_object;
   class special_authority_object;
   class buyback_object;
   class fba_accumulator_object;
   class multisig_account_pair_object;
   class guarantee_object;
   class history_transaction_object;
   class blocked_address_object;
   class trx_object;
   class lockbalance_record_object;
   class whiteOperationList_object;
   typedef object_id< implementation_ids, impl_global_property_object_type,  global_property_object>                    global_property_id_type;
   typedef object_id< implementation_ids, impl_dynamic_global_property_object_type,  dynamic_global_property_object>    dynamic_global_property_id_type;
   typedef object_id< implementation_ids, impl_asset_dynamic_data_type,      asset_dynamic_data_object>                 asset_dynamic_data_id_type;
   typedef object_id< implementation_ids, impl_asset_bitasset_data_type,     asset_bitasset_data_object>                asset_bitasset_data_id_type;
   typedef object_id< implementation_ids, impl_account_balance_object_type,  account_balance_object>                    account_balance_id_type;
   typedef object_id< implementation_ids, impl_account_statistics_object_type,account_statistics_object>                account_statistics_id_type;
   typedef object_id< implementation_ids, impl_transaction_object_type,      transaction_object>                        transaction_obj_id_type;
   typedef object_id<implementation_ids, impl_trx_object_type,               trx_object>                                trx_obj_id_type;
   typedef object_id<implementation_ids, impl_history_transaction_object_type, history_transaction_object>              history_transaction_obj_id_type;
   typedef object_id< implementation_ids, impl_block_summary_object_type,    block_summary_object>                      block_summary_id_type;

   typedef object_id< implementation_ids,
                      impl_account_transaction_history_object_type,
                      account_transaction_history_object>       account_transaction_history_id_type;
   typedef object_id< implementation_ids, impl_chain_property_object_type,   chain_property_object>                     chain_property_id_type;
   typedef object_id< implementation_ids, impl_witness_schedule_object_type, witness_schedule_object>                   witness_schedule_id_type;
   typedef object_id< implementation_ids, impl_budget_record_object_type, budget_record_object >                        budget_record_id_type;
   typedef object_id< implementation_ids, impl_blinded_balance_object_type, blinded_balance_object >                    blinded_balance_id_type;
   typedef object_id< implementation_ids, impl_special_authority_object_type, special_authority_object >                special_authority_id_type;
   typedef object_id< implementation_ids, impl_buyback_object_type, buyback_object >                                    buyback_id_type;
   typedef object_id< implementation_ids, impl_fba_accumulator_object_type, fba_accumulator_object >                    fba_accumulator_id_type;
   typedef object_id< implementation_ids, impl_multisig_account_binding_object_type, multisig_account_pair_object >     multisig_account_pair_id_type;
   typedef object_id< implementation_ids, impl_guarantee_obj_type, guarantee_object >     guarantee_object_id_type;
   typedef object_id< implementation_ids, impl_blocked_address_obj_type, blocked_address_object >     blocked_address_id_type;
   typedef object_id< implementation_ids, impl_lockbalance_record_object_type, lockbalance_record_object >     lockbalance_record_id_type;
   typedef object_id<implementation_ids, impl_white_operationlist_object_type, whiteOperationList_object > whiteOperationList_id_type;
   typedef fc::array<char, GRAPHENE_MAX_ASSET_SYMBOL_LENGTH>    symbol_type;
   typedef fc::ripemd160                                        block_id_type;
   typedef fc::ripemd160                                        checksum_type;
   typedef fc::ripemd160                                        transaction_id_type;
   typedef fc::ripemd160										SecretHashType;
   typedef fc::sha256                                           digest_type;
   typedef fc::ecc::compact_signature                           signature_type;
   typedef safe<int64_t>                                        share_type;
   typedef uint16_t                                             weight_type;

   template<class T>
   optional<T> maybe_id(const string& name_or_id)
   {
       if (std::isdigit(name_or_id.front()))
       {
           try
           {
               return fc::variant(name_or_id).as<T>();
           }
           catch (const fc::exception&)
           {
           }
       }
       return optional<T>();
   }
   struct public_key_type
   {
       struct binary_key
       {
          binary_key() {}
          uint32_t                 check = 0;
          fc::ecc::public_key_data data;
       };
       fc::ecc::public_key_data key_data;
       public_key_type();
       public_key_type( const fc::ecc::public_key_data& data );
       public_key_type( const fc::ecc::public_key& pubkey );
       explicit public_key_type( const std::string& base58str );
       operator fc::ecc::public_key_data() const;
       operator fc::ecc::public_key() const;
       explicit operator std::string() const;
       friend bool operator == ( const public_key_type& p1, const fc::ecc::public_key& p2);
       friend bool operator == ( const public_key_type& p1, const public_key_type& p2);
       friend bool operator != ( const public_key_type& p1, const public_key_type& p2);
       // TODO: This is temporary for testing
       bool is_valid_v1( const std::string& base58str );
   };

   struct extended_public_key_type
   {
      struct binary_key
      {
         binary_key() {}
         uint32_t                   check = 0;
         fc::ecc::extended_key_data data;
      };
      
      fc::ecc::extended_key_data key_data;
       
      extended_public_key_type();
      extended_public_key_type( const fc::ecc::extended_key_data& data );
      extended_public_key_type( const fc::ecc::extended_public_key& extpubkey );
      explicit extended_public_key_type( const std::string& base58str );
      operator fc::ecc::extended_public_key() const;
      explicit operator std::string() const;
      friend bool operator == ( const extended_public_key_type& p1, const fc::ecc::extended_public_key& p2);
      friend bool operator == ( const extended_public_key_type& p1, const extended_public_key_type& p2);
      friend bool operator != ( const extended_public_key_type& p1, const extended_public_key_type& p2);
   };
   
   struct extended_private_key_type
   {
      struct binary_key
      {
         binary_key() {}
         uint32_t                   check = 0;
         fc::ecc::extended_key_data data;
      };
      
      fc::ecc::extended_key_data key_data;
       
      extended_private_key_type();
      extended_private_key_type( const fc::ecc::extended_key_data& data );
      extended_private_key_type( const fc::ecc::extended_private_key& extprivkey );
      explicit extended_private_key_type( const std::string& base58str );
      operator fc::ecc::extended_private_key() const;
      explicit operator std::string() const;
      friend bool operator == ( const extended_private_key_type& p1, const fc::ecc::extended_private_key& p2);
      friend bool operator == ( const extended_private_key_type& p1, const extended_private_key_type& p2);
      friend bool operator != ( const extended_private_key_type& p1, const extended_private_key_type& p2);
   };
} }  // graphene::chain

namespace fc
{
    void to_variant( const graphene::chain::public_key_type& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  graphene::chain::public_key_type& vo );
    void to_variant( const graphene::chain::extended_public_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, graphene::chain::extended_public_key_type& vo );
    void to_variant( const graphene::chain::extended_private_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, graphene::chain::extended_private_key_type& vo );
}

FC_REFLECT( graphene::chain::public_key_type, (key_data) )
FC_REFLECT( graphene::chain::public_key_type::binary_key, (data)(check) )
FC_REFLECT( graphene::chain::extended_public_key_type, (key_data) )
FC_REFLECT( graphene::chain::extended_public_key_type::binary_key, (check)(data) )
FC_REFLECT( graphene::chain::extended_private_key_type, (key_data) )
FC_REFLECT( graphene::chain::extended_private_key_type::binary_key, (check)(data) )

FC_REFLECT_ENUM( graphene::chain::object_type,
                 (null_object_type)
                 (base_object_type)
                 (account_object_type)
                 (force_settlement_object_type)
                 (asset_object_type)
                 (guard_member_object_type)
                 (miner_object_type)
                 (limit_order_object_type)
                 (call_order_object_type)
                 (custom_object_type)
                 (proposal_object_type)
	             (referendum_object_type)
                 (operation_history_object_type)
                 (withdraw_permission_object_type)
                 (vesting_balance_object_type)
                 (worker_object_type)
                 (balance_object_type)
				 (crosschain_trx_object_type)
				 (lockbalance_object_type)
				 (guard_lock_balance_object_type)
	             (multisig_transfer_object_type)
                 (acquired_crosschain_object_type)
				 (crosschain_transaction_history_count_object_type)
				 (contract_object_type)
				 (contract_storage_object_type)
				 (contract_storage_diff_type)
				 (contract_storage_type)
				 (contract_balance_object_type)
				 (contract_event_notify_object_type)
				 (contract_invoke_result_object_type)
				 (coldhot_transfer_object_type)
				 (script_object_type)
				 (script_binding_object_type)
				 (pay_back_object_type)
	             (bonus_object_type)
	             (total_fees_type)
				 (contract_storage_change_object_type)
				 (contract_history_object_type)
				 (eth_multi_account_trx_object_type)
	             (vote_object_type)
                 (OBJECT_TYPE_COUNT)
               )
FC_REFLECT_ENUM( graphene::chain::impl_object_type,
                 (impl_global_property_object_type)
                 (impl_dynamic_global_property_object_type)
                 (impl_reserved0_object_type)
                 (impl_asset_dynamic_data_type)
                 (impl_asset_bitasset_data_type)
                 (impl_account_balance_object_type)
                 (impl_account_statistics_object_type)
	             (impl_trx_object_type)
                 (impl_transaction_object_type)
	             (impl_history_transaction_object_type)
                 (impl_block_summary_object_type)
                 (impl_account_transaction_history_object_type)
                 (impl_blinded_balance_object_type)
                 (impl_chain_property_object_type)
                 (impl_witness_schedule_object_type)
                 (impl_budget_record_object_type)
                 (impl_special_authority_object_type)
                 (impl_buyback_object_type)
                 (impl_fba_accumulator_object_type)
	             (impl_multisig_account_binding_object_type)
	             (impl_blocked_address_obj_type)
	             (impl_lockbalance_record_object_type)
	             (impl_guarantee_obj_type)
	            (impl_white_operationlist_object_type)
               )

FC_REFLECT_TYPENAME( graphene::chain::share_type )

FC_REFLECT_TYPENAME( graphene::chain::account_id_type )
FC_REFLECT_TYPENAME(graphene::chain::crosschain_trx_id_type)
FC_REFLECT_TYPENAME( graphene::chain::asset_id_type )
FC_REFLECT_TYPENAME( graphene::chain::force_settlement_id_type )
FC_REFLECT_TYPENAME( graphene::chain::guard_member_id_type )
FC_REFLECT_TYPENAME( graphene::chain::miner_id_type )
FC_REFLECT_TYPENAME( graphene::chain::limit_order_id_type )
FC_REFLECT_TYPENAME( graphene::chain::call_order_id_type )
FC_REFLECT_TYPENAME( graphene::chain::custom_id_type )
FC_REFLECT_TYPENAME( graphene::chain::proposal_id_type )
FC_REFLECT_TYPENAME( graphene::chain::operation_history_id_type )
FC_REFLECT_TYPENAME( graphene::chain::withdraw_permission_id_type )
FC_REFLECT_TYPENAME( graphene::chain::vesting_balance_id_type )
FC_REFLECT_TYPENAME( graphene::chain::worker_id_type )
FC_REFLECT_TYPENAME( graphene::chain::balance_id_type )
FC_REFLECT_TYPENAME( graphene::chain::global_property_id_type )
FC_REFLECT_TYPENAME( graphene::chain::dynamic_global_property_id_type )
FC_REFLECT_TYPENAME( graphene::chain::asset_dynamic_data_id_type )
FC_REFLECT_TYPENAME( graphene::chain::asset_bitasset_data_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_balance_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_statistics_id_type )
FC_REFLECT_TYPENAME( graphene::chain::transaction_obj_id_type )
FC_REFLECT_TYPENAME(graphene::chain::history_transaction_obj_id_type)
FC_REFLECT_TYPENAME(graphene::chain::trx_obj_id_type)
FC_REFLECT_TYPENAME( graphene::chain::block_summary_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_transaction_history_id_type )
FC_REFLECT_TYPENAME( graphene::chain::budget_record_id_type )
FC_REFLECT_TYPENAME( graphene::chain::special_authority_id_type )
FC_REFLECT_TYPENAME( graphene::chain::buyback_id_type )
FC_REFLECT_TYPENAME(graphene::chain::lockbalance_id_type)
FC_REFLECT_TYPENAME(graphene::chain::guard_lock_balance_id_type)
FC_REFLECT_TYPENAME( graphene::chain::fba_accumulator_id_type )
FC_REFLECT_TYPENAME(graphene::chain::multisig_asset_transfer_id_type)
FC_REFLECT_TYPENAME(graphene::chain::acquired_crosschain_id_type)
FC_REFLECT_TYPENAME(graphene::chain::coldhot_transfer_id_type)
FC_REFLECT_TYPENAME(graphene::chain::multisig_account_pair_id_type)
FC_REFLECT_TYPENAME(graphene::chain::transaction_history_count_id_type)
FC_REFLECT_TYPENAME(graphene::chain::contract_id_type)
FC_REFLECT_TYPENAME(graphene::chain::contract_storage_id_type)
FC_REFLECT_TYPENAME(graphene::chain::transaction_contract_storage_diff_object_id_type)
FC_REFLECT_TYPENAME(graphene::chain::contract_event_notify_object_id_type)
FC_REFLECT_TYPENAME(graphene::chain::contract_history_object_id_type)
FC_REFLECT_TYPENAME(graphene::chain::eth_multi_account_id_type)
FC_REFLECT_TYPENAME(graphene::chain::blocked_address_id_type)
FC_REFLECT_TYPENAME(graphene::chain::lockbalance_record_id_type)
FC_REFLECT_TYPENAME(graphene::chain::guarantee_object_id_type)
FC_REFLECT_TYPENAME(graphene::chain::bonus_object_id_type)
FC_REFLECT_TYPENAME(graphene::chain::total_fees_object_id_type)
FC_REFLECT_TYPENAME(graphene::chain::whiteOperationList_id_type)
FC_REFLECT_TYPENAME(graphene::chain::vote_object_id_type)
FC_REFLECT( graphene::chain::void_t, )

FC_REFLECT_ENUM( graphene::chain::asset_issuer_permission_flags,
   (charge_market_fee)
   (white_list)
   (transfer_restricted)
   (override_authority)
   (disable_force_settle)
   (global_settle)
   (disable_confidential)
   (witness_fed_asset)
   (committee_fed_asset)
   )
FC_REFLECT_ENUM(graphene::chain::senatorType, (EXTERNAL)(PERMANENT))