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
#include <graphene/chain/transfer_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/crosschain/crosschain.hpp>
#include <graphene/crosschain/crosschain_impl.hpp>
#include <graphene/chain/transaction_object.hpp>
namespace graphene { namespace chain {
void_result transfer_evaluator::do_evaluate( const transfer_operation& op )
{ try {
   
   const database& d = db();

   //const account_object& from_account    = op.from(d);
   //const account_object& to_account      = op.to(d);
   const asset_object&   asset_type      = op.amount.asset_id(d);

   try {
	   if (d.is_white(op.from_addr, operation(op).which()) || d.is_white(op.to_addr, operation(op).which()))
	   {
		   FC_ASSERT(op.amount.asset_id == asset_id_type(0),"invalid asset.");
	   }
	 
	   FC_ASSERT(op.to_addr.version != addressVersion::CONTRACT,"address should not be a contract address");
	   bool insufficient_balance =  d.get_balance(op.from_addr, asset_type.id).amount >= op.amount.amount;

	   FC_ASSERT(insufficient_balance,
		   "Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from addr '${a}' to '${t}'",
		   ("a", op.from_addr)("t", op.to_addr)("total_transfer", d.to_pretty_string(op.amount))("balance", d.to_pretty_string(d.get_balance(op.from_addr, asset_type.id))));
	   return void_result();
	   
   } FC_RETHROW_EXCEPTIONS( error, "Unable to transfer ${a} from ${f} to ${t}", ("a",d.to_pretty_string(op.amount))("f",op.from(d).name)("t",op.to(d).name) );

}  FC_CAPTURE_AND_RETHROW( (op) ) }

void_result transfer_evaluator::do_apply( const transfer_operation& o )
{ try {
   //db().adjust_balance( o.from, -o.amount );
   //db().adjust_balance( o.to, o.amount );
	database& d = db();
	d.adjust_balance(o.from_addr, -o.amount);
	d.adjust_balance(o.to_addr,o.amount);
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }



void_result override_transfer_evaluator::do_evaluate( const override_transfer_operation& op )
{ try {
   const database& d = db();

   const asset_object&   asset_type      = op.amount.asset_id(d);
   GRAPHENE_ASSERT(
      asset_type.can_override(),
      override_transfer_not_permitted,
      "override_transfer not permitted for asset ${asset}",
      ("asset", op.amount.asset_id)
      );
   FC_ASSERT( asset_type.issuer == op.issuer );

   const account_object& from_account    = op.from(d);
   const account_object& to_account      = op.to(d);

   FC_ASSERT( is_authorized_asset( d, to_account, asset_type ) );
   FC_ASSERT( is_authorized_asset( d, from_account, asset_type ) );
   // the above becomes no-op after hardfork because this check will then be performed in evaluator

   FC_ASSERT( d.get_balance( from_account, asset_type ).amount >= op.amount.amount,
              "", ("total_transfer",op.amount)("balance",d.get_balance(from_account, asset_type).amount) );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result override_transfer_evaluator::do_apply( const override_transfer_operation& o )
{ try {
   db().adjust_balance( o.from, -o.amount );
   db().adjust_balance( o.to, o.amount );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_transfer_from_cold_to_hot_evaluator::do_evaluate(const asset_transfer_from_cold_to_hot_operation& o)
{
	try {
		const auto& d = db();
		const auto& assets = d.get_index_type<asset_index>().indices().get<by_symbol>();
		FC_ASSERT(assets.find(o.chain_type) != assets.end());
		const auto& accounts = d.get_index_type<account_index>().indices().get<by_address>();
		const auto acct = accounts.find(o.addr);
		FC_ASSERT(acct != accounts.end());
		const auto& guards = d.get_index_type<guard_member_index>().indices().get<by_account>();
		FC_ASSERT(guards.find(acct->get_id()) != guards.end());
		
		auto& instance = crosschain::crosschain_manager::get_instance();
		if (!instance.contain_crosschain_handles(o.chain_type))
			return void_result();
		auto crosschain_interface = instance.get_crosschain_handle(o.chain_type);
		if (!crosschain_interface->valid_config())
			return void_result();
		FC_ASSERT(crosschain_interface->validate_other_trx(o.trx));
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result asset_transfer_from_cold_to_hot_evaluator::do_apply(const asset_transfer_from_cold_to_hot_operation& o)
{
	try {
		auto& d = db();
		d.create<multisig_asset_transfer_object>([&o](multisig_asset_transfer_object& obj) {
			obj.trx = o.trx;
			obj.chain_type = o.chain_type;
			obj.status = multisig_asset_transfer_object::waiting_signtures;
		});
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result sign_multisig_asset_evaluator::do_evaluate(const sign_multisig_asset_operation& o)
{
	try {
		const auto& d = db();
		auto id = o.multisig_trx_id;
		const auto& trxids = d.get_index_type<crosschain_transfer_index>().indices().get<by_id>();
		FC_ASSERT(trxids.find(id) != trxids.end());

		//if the addr is guard
		const auto& addrs = d.get_index_type<account_index>().indices().get<by_address>();
		const auto& guards = d.get_index_type<guard_member_index>().indices().get<by_account>();
		FC_ASSERT(guards.find(addrs.find(o.addr)->get_id())!= guards.end());

	}FC_CAPTURE_AND_RETHROW((o))
}

void_result sign_multisig_asset_evaluator::do_apply(const sign_multisig_asset_operation& o)
{
	try {
		auto& d = db();
		auto& trxids = d.get_index_type<crosschain_transfer_index>().indices().get<by_id>();
		auto iter = trxids.find(o.multisig_trx_id);
		d.modify(*iter, [&](multisig_asset_transfer_object& obj) {
			obj.signatures.insert(o.signature);
		}
		);

	}FC_CAPTURE_AND_RETHROW((o))
}

} } // graphene::chain
