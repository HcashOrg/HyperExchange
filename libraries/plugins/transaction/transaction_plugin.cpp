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

#include <graphene/transaction/transaction_plugin.hpp>

#include <graphene/app/impacted.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>
#include <iostream>
namespace graphene { namespace transaction {

namespace detail
{


class transaction_plugin_impl
{
   public:
	   transaction_plugin_impl(transaction_plugin& _plugin);
       virtual ~transaction_plugin_impl();


      /** this method is called as a callback after a block is applied
       * and will process/index all operations that were applied in the block.
       */
      void update_transaction_record( const signed_block& b );
	  void erase_transaction_records(const vector<signed_transaction>& trxs);
      graphene::chain::database& database()
      {
         return _self.database();
      }

	  transaction_plugin& _self;
      flat_set<address> _tracked_addresses;
      bool _partial_operations = false;
      /** add one history record, then check and remove the earliest history record */
      void add_transaction_history( const signed_transaction& trx );

};

transaction_plugin_impl::transaction_plugin_impl(transaction_plugin& plugin):_self(plugin){}
transaction_plugin_impl::~transaction_plugin_impl(){}

void transaction_plugin_impl::erase_transaction_records(const vector<signed_transaction>& trxs)
{
	const auto& db = database();
	for (auto tx : trxs)
	{
		leveldb::WriteOptions write_options;
		db.get_levelDB()->Delete(write_options, tx.id().str());
	}
}

void transaction_plugin_impl::update_transaction_record( const signed_block& b )
{
   graphene::chain::database& db = database();
   for (auto trx : b.transactions) {
	   leveldb::WriteOptions write_options;
	   trx_object obj;
	   obj.trx = trx;
	   obj.trx_id = trx.id();
	   obj.block_num = b.block_num();
	   leveldb::Status sta = db.get_levelDB()->Put(write_options, obj.trx_id.str(), fc::json::to_string(obj));
	   if (!sta.ok())
	   {
		   elog("Put error: ${error}", ("error", (trx.id().str() + ":" + sta.ToString()).c_str()));
		   FC_ASSERT(false, "Put Data to transaction failed");
		   return;
	   }
	   add_transaction_history(trx);
   }   
}

void transaction_plugin_impl::add_transaction_history(const signed_transaction& trx)
{
	graphene::chain::database& db = database();
	if (_tracked_addresses.size() == 0)
		return;
	auto chain_id = db.get_chain_id();
	auto signatures = trx.get_signature_keys(chain_id);
	flat_set<address> addresses;
	for (auto sig : signatures)
	{
		addresses.insert(address(sig));
	}
	auto res=db.get_contract_invoke_result(trx.id());
	for (const auto& it : res)
	{
		for (const auto& deposit_it : it.deposit_to_address)
		{
			addresses.insert(deposit_it.first.first);
		}
	}
	for (auto op : trx.operations)
	{
		if (op.which() == operation::tag<transfer_operation>::value)
		{
			auto op_transfer = op.get<transfer_operation>();
			addresses.insert(op_transfer.from_addr);
			addresses.insert(op_transfer.to_addr);
		}
		else if (op.which() == operation::tag<graphene::chain::crosschain_record_operation>::value)
		{
			auto op_record = op.get<graphene::chain::crosschain_record_operation>();
			const auto& tunnel_idx = db.get_index_type<account_binding_index>().indices().get<by_binded_account>();
			const auto tunnel_itr = tunnel_idx.find(boost::make_tuple(op_record.cross_chain_trx.from_account, op_record.cross_chain_trx.asset_symbol));
			addresses.insert(tunnel_itr->owner);
		}
		auto id = operation_gurantee_id(op);
		if (id.valid())
		{
			const auto& obj = db.get(*id);
			addresses.insert(obj.owner_addr);
		}
		
	}
	auto trx_op = db.fetch_trx(trx.id());
	if (!trx_op.valid())
		return;

	for (auto addr : addresses)
	{
		auto iter = _tracked_addresses.find(addr);
		if (iter == _tracked_addresses.end())
			continue;
		db.create<history_transaction_object>([&](history_transaction_object& obj) {
			obj.addr = addr;
			obj.trx_id = trx_op->trx_id;
			obj.block_num = trx_op->block_num;
		});
	}
}

} // end namespace detail

transaction_plugin::transaction_plugin() :
   my( new detail::transaction_plugin_impl(*this) )
{
}

transaction_plugin::~transaction_plugin()
{
}

std::string transaction_plugin::plugin_name()const
{
   return "transaction_plugin";
}

void transaction_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
   cli.add_options()
         ("track-address", boost::program_options::value<std::vector<std::string>>()->composing()->multitoken(), "address to track history for (may specify multiple times)");
   cfg.add(cli);
}

void transaction_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   database().applied_block.connect( [&]( const signed_block& b){ my->update_transaction_record(b); } );
   database().removed_trxs.connect([&](const vector<signed_transaction>& b) {my->erase_transaction_records(b); });
   database().add_index <primary_index<trx_index         > >();
   database().add_index <primary_index<history_transaction_index > >();
   LOAD_VALUE_SET(options, "track-address", my->_tracked_addresses, graphene::chain::address);
}

void transaction_plugin::plugin_startup()
{
}

flat_set<address> transaction_plugin::tracked_address() const
{
   return my->_tracked_addresses;
}

void transaction_plugin::add_tracked_address(vector<address> addrs)
{
	for (auto addr : addrs)
		my->_tracked_addresses.insert(addr);
}


} }
