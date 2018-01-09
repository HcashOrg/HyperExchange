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
#include <graphene/chain/witness_evaluator.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <graphene/crosschain/crosschain_interface_emu.hpp>
#include <graphene/crosschain/crosschain.hpp>

namespace graphene { namespace chain {

void_result miner_create_evaluator::do_evaluate( const miner_create_operation& op )
{ try {
   //account cannot be a guard
   auto & iter = db().get_index_type<guard_member_index>().indices().get<by_account>();
   FC_ASSERT(iter.find(op.miner_account) == iter.end(),"account cannot be a guard.");
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type miner_create_evaluator::do_apply( const miner_create_operation& op )
{ try {
   vote_id_type vote_id;
   db().modify(db().get_global_properties(), [&vote_id](global_property_object& p) {
      vote_id = get_next_vote_id(p, vote_id_type::witness);
   });

   const auto& new_miner_object = db().create<miner_object>( [&]( miner_object& obj ){
         obj.miner_account  = op.miner_account;
         obj.signing_key      = op.block_signing_key;
         obj.vote_id          = vote_id;
         obj.url              = op.url;
   });
   return new_miner_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_update_evaluator::do_evaluate( const witness_update_operation& op )
{ try {
   FC_ASSERT(db().get(op.witness).miner_account == op.witness_account);
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_update_evaluator::do_apply( const witness_update_operation& op )
{ try {
   database& _db = db();
   _db.modify(
      _db.get(op.witness),
      [&]( miner_object& wit )
      {
         if( op.new_url.valid() )
            wit.url = *op.new_url;
		 if (op.new_signing_key.valid())
		 {
			 wit.signing_key = *op.new_signing_key;
			 wit.last_change_signing_key_block_num = _db.head_block_num()+1;
		 }
      });
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }
void_result miner_generate_multi_asset_evaluator::do_evaluate(const miner_generate_multi_asset_operation& o)
{
	try {
		//FC_ASSERT(db().get(o.miner).miner_account == o.miner);
		//need to check the status of miner...
		const auto& miners = db().get_index_type<miner_index>().indices().get<by_id>();
		auto miner = miners.find(o.miner);
		FC_ASSERT(miner != miners.end());
		const auto& accounts = db().get_index_type<account_index>().indices().get<by_id>();
		const auto acct = accounts.find(miner->miner_account);
		FC_ASSERT(acct->addr == o.miner_address);
		const auto& assets = db().get_index_type<asset_index>().indices().get<by_symbol>();
		FC_ASSERT(assets.find(o.chain_type) != assets.end());
		//if the multi-addr correct or not.
		vector<string> symbol_addrs_cold;
		vector<string> symbol_addrs_hot;
		const auto& addr = db().get_index_type<multisig_address_index>().indices().get<by_account_chain_type>();
		auto addr_range = addr.equal_range(boost::make_tuple(o.chain_type));
		std::for_each(
			addr_range.first, addr_range.second, [&symbol_addrs_cold, &symbol_addrs_hot](const multisig_address_object& obj) {
			if (obj.multisig_account_pair_object_id == multisig_account_pair_id_type())
			{
				symbol_addrs_cold.push_back(obj.new_address_cold);
				symbol_addrs_hot.push_back(obj.new_address_hot);
			}
		}
		);

		const auto& guard_ids = db().get_global_properties().active_committee_members;
		FC_ASSERT(symbol_addrs_cold.size()==guard_ids.size() && symbol_addrs_hot.size()==guard_ids.size());
		auto& instance = graphene::crosschain::crosschain_manager::get_instance();
		auto crosschain_interface = instance.get_crosschain_handle(o.chain_type);
		auto multi_addr_cold = crosschain_interface->create_multi_sig_account(o.chain_type + "_cold", symbol_addrs_cold, std::ceill(symbol_addrs_cold.size() * 2 / 3));
		auto multi_addr_hot = crosschain_interface->create_multi_sig_account(o.chain_type + "_hot", symbol_addrs_hot, std::ceill(symbol_addrs_hot.size() * 2 / 3));

		FC_ASSERT(o.multi_address_cold == multi_addr_cold["address"]);
		FC_ASSERT(o.multi_redeemScript_cold == multi_addr_cold["redeemScript"]);
		FC_ASSERT(o.multi_address_hot == multi_addr_hot["address"]);
		FC_ASSERT(o.multi_redeemScript_hot == multi_addr_hot["redeemScript"]);


	}FC_CAPTURE_AND_RETHROW((o))
}

void_result miner_generate_multi_asset_evaluator::do_apply(const miner_generate_multi_asset_operation& o)
{
	try
	{
		//update the latest multi-addr in database
		auto& account_pair_index = db().get_index_type<multisig_account_pair_index>().indices().get<by_multisig_account_pair>();
		auto& iter = account_pair_index.find(boost::make_tuple(o.multi_address_hot,o.multi_address_cold,o.chain_type));
		if (iter != account_pair_index.end())
		{
			db().remove(*iter);
		}
		const auto& new_acnt_object = db().create<multisig_account_pair_object>([&](multisig_account_pair_object& obj) {
			obj.bind_account_hot = o.multi_address_hot;
			obj.redeemScript_hot = o.multi_redeemScript_hot;
			obj.redeemScript_cold = o.multi_redeemScript_cold;
			obj.bind_account_cold = o.multi_address_cold;
			obj.chain_type = o.chain_type;
			obj.effective_block_num = 0;
		});
		//we need change the status of the multisig_address_object
		auto &guard_change_idx = db().get_index_type<multisig_address_index>().indices().get<by_account_chain_type>();
		for (auto& itr : guard_change_idx)
		{
			db().modify(itr, [&](multisig_address_object& obj) {
				obj.multisig_account_pair_object_id = new_acnt_object.id;
			});
		}

	}FC_CAPTURE_AND_RETHROW((o))
}

void_result miner_merge_signatures_evaluator::do_evaluate(const miner_merge_signatures_operation& o)
{
	try {
		//FC_ASSERT(db().get(o.miner).miner_account == o.miner);
		//need to check the status of miner...
		const auto& miners = db().get_index_type<miner_index>().indices().get<by_id>();
		auto miner = miners.find(o.miner);
		FC_ASSERT(miner != miners.end());
		const auto& accounts = db().get_index_type<account_index>().indices().get<by_id>();
		const auto acct = accounts.find(miner->miner_account);
		FC_ASSERT(acct->addr == o.miner_address);
		const auto& assets = db().get_index_type<asset_index>().indices().get<by_symbol>();
		FC_ASSERT(assets.find(o.chain_type) != assets.end());
		//TODO
		//need to evalute the correction of the crosschain trx
		auto& instance = graphene::crosschain::crosschain_manager::get_instance();
		auto crosschain_interface = instance.get_crosschain_handle("EMU");


	}FC_CAPTURE_AND_RETHROW((o))
}

void_result miner_merge_signatures_evaluator::do_apply(const miner_merge_signatures_operation& o)
{
	try {
		auto& instance = graphene::crosschain::crosschain_manager::get_instance();
		auto crosschain_interface = instance.get_crosschain_handle("EMU");
		auto& transfers = db().get_index_type<crosschain_transfer_index>().indices().get<by_id>();
		auto trx = transfers.find(o.id);
		FC_ASSERT(trx != transfers.end());
		crosschain_interface->broadcast_transaction(trx->trx);
		db().modify(*trx, [&](multisig_asset_transfer_object& obj) {
			obj.status = multisig_asset_transfer_object::waiting;
		});
	}FC_CAPTURE_AND_RETHROW((o))
}

} } // graphene::chain
