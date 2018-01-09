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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS ORminer_create_operation
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain { 

  /**
    * @brief Create a miner object, as a bid to hold a miner position on the network.
    * @ingroup operations
    *
    * Accounts which wish to become miner may use this operation to create a miner object which stakeholders may
    * vote on to approve its position as a miner.
    */
   struct miner_create_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 5000 * GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset             fee;
      /// The account which owns the miner. This account pays the fee for this operation.
      account_id_type   miner_account;
      string            url;
      public_key_type   block_signing_key;

      account_id_type fee_payer()const { return miner_account; }
      void            validate()const;
   };

  /**
    * @brief Update a witness object's URL and block signing key.
    * @ingroup operations
    */
   struct witness_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         share_type fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
      };

      asset             fee;
      /// The witness object to update.
      miner_id_type   witness;
      /// The account which owns the witness. This account pays the fee for this operation.
      account_id_type   witness_account;
      /// The new URL.
      optional< string > new_url;
      /// The new block signing key.
      optional< public_key_type > new_signing_key;

      account_id_type fee_payer()const { return witness_account; }
      void            validate()const;
   };

   //miner to generate new multi-address for specific asset
   struct miner_generate_multi_asset_operation :public base_operation
   {
	   struct fee_parameters_type
	   {
		   share_type fee = 0 * GRAPHENE_BLOCKCHAIN_PRECISION;
	   };
	   asset fee;
	   miner_id_type miner;
	   string chain_type;
	   address miner_address;
	   string multi_address_hot;
	   string multi_redeemScript_hot;
	   string multi_address_cold;
	   string multi_redeemScript_cold;
	   address fee_payer() const { return miner_address;} 
	   void validate() const;
	   void get_required_authorities(vector<authority>& a)const {
		   a.push_back(authority(1, miner_address, 1));
	   }
   };

   struct miner_merge_signatures_operation :public base_operation
   {
	   struct fee_parameters_type
	   {
		   share_type fee = 0 * GRAPHENE_BLOCKCHAIN_PRECISION;
	   };
	   asset fee;
	   miner_id_type miner;
	   string chain_type;
	   address miner_address;
	   multisig_asset_transfer_id_type id;
	   address fee_payer() const { return miner_address; }
	   void validate() const;
	   void get_required_authorities(vector<authority>& a)const {
		   a.push_back(authority(1, miner_address, 1));
	   }
   };

   /// TODO: witness_resign_operation : public base_operation

} } // graphene::chain

FC_REFLECT( graphene::chain::miner_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::miner_create_operation, (fee)(miner_account)(url)(block_signing_key) )

FC_REFLECT( graphene::chain::witness_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::witness_update_operation, (fee)(witness)(witness_account)(new_url)(new_signing_key) )

FC_REFLECT(graphene::chain::miner_generate_multi_asset_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::miner_generate_multi_asset_operation, (fee)(miner)(chain_type)(miner_address)(multi_address_hot)(multi_redeemScript_hot)(multi_address_cold)(multi_redeemScript_cold))

FC_REFLECT(graphene::chain::miner_merge_signatures_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::miner_merge_signatures_operation, (fee)(miner)(chain_type)(miner_address)(id))