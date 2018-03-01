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
      void update_transaction_record( const signed_transaction& b );

      graphene::chain::database& database()
      {
         return _self.database();
      }

	  transaction_plugin& _self;
      flat_set<account_id_type> _tracked_accounts;
      bool _partial_operations = false;
      primary_index< simple_index< operation_history_object > >* _oho_index;
      uint32_t _max_ops_per_account = -1;
   private:
      /** add one history record, then check and remove the earliest history record */
      void add_account_history( const account_id_type account_id, const operation_history_id_type op_id );

};

transaction_plugin_impl::~transaction_plugin_impl()
{
   return;
}

void transaction_plugin_impl::update_transaction_record( const signed_transaction& trx )
{
   graphene::chain::database& db = database();
   db.create<transaction_object>([&](transaction_object & obj){
	   obj.trx = trx;
       obj.trx_id = trx.id();
   });
}

void transaction_plugin_impl::add_account_history( const account_id_type account_id, const operation_history_id_type op_id )
{
   graphene::chain::database& db = database();
   const auto& stats_obj = account_id(db).statistics(db);
   // add new entry
   const auto& ath = db.create<account_transaction_history_object>( [&]( account_transaction_history_object& obj ){
       obj.operation_id = op_id;
       obj.account = account_id;
       obj.sequence = stats_obj.total_ops + 1;
       obj.next = stats_obj.most_recent_op;
   });
   db.modify( stats_obj, [&]( account_statistics_object& obj ){
       obj.most_recent_op = ath.id;
       obj.total_ops = ath.sequence;
   });
   // remove the earliest account history entry if too many
   // _max_ops_per_account is guaranteed to be non-zero outside
   if( stats_obj.total_ops - stats_obj.removed_ops > _max_ops_per_account )
   {
      // look for the earliest entry
      const auto& his_idx = db.get_index_type<account_transaction_history_index>();
      const auto& by_seq_idx = his_idx.indices().get<by_seq>();
      auto itr = by_seq_idx.lower_bound( boost::make_tuple( account_id, 0 ) );
      // make sure don't remove the one just added
      if( itr != by_seq_idx.end() && itr->account == account_id && itr->id != ath.id )
      {
         // if found, remove the entry, and adjust account stats object
         const auto remove_op_id = itr->operation_id;
         const auto itr_remove = itr;
         ++itr;
         db.remove( *itr_remove );
         db.modify( stats_obj, [&]( account_statistics_object& obj ){
             obj.removed_ops = obj.removed_ops + 1;
         });
         // modify previous node's next pointer
         // this should be always true, but just have a check here
         if( itr != by_seq_idx.end() && itr->account == account_id )
         {
            db.modify( *itr, [&]( account_transaction_history_object& obj ){
               obj.next = account_transaction_history_id_type();
            });
         }
         // else need to modify the head pointer, but it shouldn't be true

         // remove the operation history entry (1.11.x) if configured and no reference left
         if( _partial_operations )
         {
            // check for references
            const auto& by_opid_idx = his_idx.indices().get<by_opid>();
            if( by_opid_idx.find( remove_op_id ) == by_opid_idx.end() )
            {
               // if no reference, remove
               db.remove( remove_op_id(db) );
            }
         }
      }
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
         ("track-account", boost::program_options::value<std::vector<std::string>>()->composing()->multitoken(), "Account ID to track history for (may specify multiple times)")
         ("partial-operations", boost::program_options::value<bool>(), "Keep only those operations in memory that are related to account history tracking")
         ("max-ops-per-account", boost::program_options::value<uint32_t>(), "Maximum number of operations per account will be kept in memory")
         ;
   cfg.add(cli);
}

void transaction_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   database().store_transactions.connect( [&]( const signed_transaction& b){ my->update_transaction_record(b); } );
   database().add_index< primary_index< transaction_index > >();
}

void transaction_plugin::plugin_startup()
{
}

flat_set<account_id_type> transaction_plugin::tracked_accounts() const
{
   return my->_tracked_accounts;
}

} }
