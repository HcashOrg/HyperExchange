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
	   transaction_plugin_impl(transaction_plugin& _plugin)
         : _self( _plugin )
      { }
      virtual ~transaction_plugin_impl();


      /** this method is called as a callback after a block is applied
       * and will process/index all operations that were applied in the block.
       */
      void update_transaction_record( const signed_block& b );

      graphene::chain::database& database()
      {
         return _self.database();
      }

	  transaction_plugin& _self;
      flat_set<address> _tracked_addresses;
      bool _partial_operations = false;
      primary_index< simple_index< operation_history_object > >* _oho_index;
      uint32_t _max_ops_per_account = -1;
      /** add one history record, then check and remove the earliest history record */
      void add_transaction_history( const signed_transaction& trx );

};

transaction_plugin_impl::~transaction_plugin_impl()
{
   return;
}

void transaction_plugin_impl::update_transaction_record( const signed_block& b )
{
   graphene::chain::database& db = database();
   for (auto trx : b.transactions) {
	   db.create<trx_object>([&](trx_object& obj) {
		   obj.trx = trx;
		   obj.trx_id = trx.id();
	   });
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

	for (auto op : trx.operations)
	{
		if (op.which() == operation::tag<transfer_operation>::value)
		{
			auto op_transfer = op.get<transfer_operation>();
			addresses.insert(op_transfer.from_addr);
			addresses.insert(op_transfer.to_addr);
		}
	}


	const auto& trx_ids = db.get_index_type<trx_index>().indices().get<by_trx_id>();
	auto iter_ids = trx_ids.find(trx.id());
	if (iter_ids == trx_ids.end())
		return;
	for (auto addr : addresses)
	{
		auto iter = _tracked_addresses.find(addr);
		if (iter == _tracked_addresses.end())
			continue;
		db.create<history_transaction_object>([&](history_transaction_object& obj) {
			obj.addr = addr;
			obj.trx_obj_id = iter_ids->id;
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
   //database().store_history_transactions.connect([&](const signed_transaction& b) {my->add_transaction_history(b); });
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
