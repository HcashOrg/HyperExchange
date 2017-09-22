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
#include <graphene/chain/protocol/chain_parameters.hpp>

namespace graphene { namespace chain { 

   /**
    * @brief Create a guard_member object, as a bid to hold a guard_member seat on the network.
    * @ingroup operations
    *
    * Accounts which wish to become guard_members may use this operation to create a guard_member object which stakeholders may
    * vote on to approve its position as a guard_member.
    */
   struct guard_member_create_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 5000 * GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset                                 fee;
      /// The account which owns the committee_member. This account pays the fee for this operation.
      account_id_type                       guard_member_account;
      string                                url;

      account_id_type fee_payer()const { return guard_member_account; }
      void            validate()const;
   };

   /**
    * @brief Update a committee_member object.
    * @ingroup operations
    *
    * Currently the only field which can be updated is the `url`
    * field.
    */
   struct guard_member_update_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset                                 fee;
      /// The committee member to update.
      account_id_type              guard_member_account;
      /// The account which owns the guard_member. This account pays the fee for this operation.
	  address                       owner_addr;
      optional< string >                    new_url;
	  optional<bool >                       formal;
      address fee_payer()const { return owner_addr; }
      void            validate()const;
   };

   /**
    * @brief Used by guard_members to update the global parameters of the blockchain.
    * @ingroup operations
    *
    * This operation allows the committee_members to update the global parameters on the blockchain. These control various
    * tunable aspects of the chain, including block and maintenance intervals, maximum data sizes, the fees charged by
    * the network, etc.
    *
    * This operation may only be used in a proposed transaction, and a proposed transaction which contains this
    * operation must have a review period specified in the current global parameters before it may be accepted.
    */
   struct committee_member_update_global_parameters_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset             fee;
      chain_parameters  new_parameters;

      account_id_type fee_payer()const { return account_id_type(); }
      void            validate()const;
   };
   struct committee_member_execute_coin_destory_operation : public base_operation
   {
	   struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
	   asset             fee;
	   asset			 loss_asset;
	   uint8_t			 commitee_member_handle_percent;
	   account_id_type fee_payer()const { return account_id_type(); }
	   void            validate()const;
   };

   /**
   * @brief Used by guard_members to resign from guard_memeber role.
   * @ingroup operations
   *
   * This operation allows the guard_members to resign or be resigned from guard_member role.
   *
   * This operation may only be used in a proposed transaction, and a proposed transaction which contains this
   * operation must have a review period specified in the current global parameters before it may be accepted.
   */
   struct guard_member_resign_operation : public base_operation
   {
       struct fee_parameters_type { uint64_t fee = 5000 * GRAPHENE_BLOCKCHAIN_PRECISION; };

       asset                                 fee;
       account_id_type                       guard_member_account;  //!< guard memeber to resign

       account_id_type fee_payer()const { return guard_member_account; }
       void            validate()const;
   };

} } // graphene::chain
FC_REFLECT( graphene::chain::guard_member_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::guard_member_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::committee_member_update_global_parameters_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::guard_member_resign_operation::fee_parameters_type, (fee) )

FC_REFLECT(graphene::chain::committee_member_execute_coin_destory_operation::fee_parameters_type,(fee))

FC_REFLECT( graphene::chain::guard_member_create_operation, (fee)(guard_member_account)(url) )
FC_REFLECT( graphene::chain::guard_member_update_operation, (fee)(guard_member_account)(owner_addr)(new_url)(formal) )
FC_REFLECT( graphene::chain::committee_member_update_global_parameters_operation, (fee)(new_parameters) );
FC_REFLECT(graphene::chain::committee_member_execute_coin_destory_operation, (fee)(loss_asset)(commitee_member_handle_percent));
FC_REFLECT( graphene::chain::guard_member_resign_operation, (fee)(guard_member_account) )
