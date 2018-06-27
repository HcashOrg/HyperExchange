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
#include <graphene/db/object_database.hpp>
#include <graphene/db/undo_database.hpp>
#include <fc/reflect/variant.hpp>
#include <iostream>

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/transaction.hpp>
#include<graphene/chain/account_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/confidential_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/worker_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/lockbalance_object.hpp>
#include <graphene/chain/guard_lock_balance_object.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <graphene/chain/coldhot_transfer_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/contract_object.hpp>
#include <graphene/chain/pay_back_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/block_summary_object.hpp>
#include <graphene/chain/witness_schedule_object.hpp>
#include <graphene/chain/budget_record_object.hpp>
#include <graphene/chain/special_authority_object.hpp>
#include <graphene/chain/buyback_object.hpp>
#include <graphene/chain/fba_object.hpp>
#include <iostream>
namespace graphene { namespace db {
    using namespace graphene::chain;
void undo_database::enable()  { _disabled = false; }
void undo_database::disable() { _disabled = true; }

undo_database::session undo_database::start_undo_session( bool force_enable )
{
   if( _disabled && !force_enable ) return session(*this);
   bool disable_on_exit = _disabled  && force_enable;
   if( force_enable ) 
      _disabled = false;

   while( size() > max_size() )
      _stack.pop_front();

   _stack.emplace_back();
   ++_active_sessions;
   return session(*this, disable_on_exit );
}
void undo_database::on_create( const object& obj )
{
   if( _disabled ) return;

   if( _stack.empty() )
      _stack.emplace_back();
   auto& state = _stack.back();
   auto index_id = object_id_type( obj.id.space(), obj.id.type(), 0 );
   auto itr = state.old_index_next_ids.find( index_id );
   if( itr == state.old_index_next_ids.end() )
      state.old_index_next_ids[index_id] = obj.id;
   state.new_ids.insert(obj.id);
}
void undo_database::on_modify( const object& obj )
{
   if( _disabled ) return;

   if( _stack.empty() )
      _stack.emplace_back();
   auto& state = _stack.back();
   if( state.new_ids.find(obj.id) != state.new_ids.end() )
      return;
   auto itr =  state.old_values.find(obj.id);
   if( itr != state.old_values.end() ) return;
   state.old_values[obj.id] = obj.clone();
}
void undo_database::on_remove( const object& obj )
{
   if( _disabled ) return;

   if( _stack.empty() )
      _stack.emplace_back();
   undo_state& state = _stack.back();
   if( state.new_ids.count(obj.id) )
   {
      state.new_ids.erase(obj.id);
      return;
   }
   if( state.old_values.count(obj.id) )
   {
      state.removed[obj.id] = std::move(state.old_values[obj.id]);
      state.old_values.erase(obj.id);
      return;
   }
   if( state.removed.count(obj.id) ) return;
   state.removed[obj.id] = obj.clone();
}

void undo_database::undo()
{ 
	try {
   FC_ASSERT( !_disabled );
   FC_ASSERT( _active_sessions > 0 );
   disable();

   auto& state = _stack.back();
   for( auto& item : state.old_values )
   {
      _db.modify( _db.get_object( item.second->id ), [&]( object& obj ){ obj.move_from( *item.second ); } );
   }

   for( auto ritr = state.new_ids.begin(); ritr != state.new_ids.end(); ++ritr  )
   {
      _db.remove( _db.get_object(*ritr) );
   }

   for( auto& item : state.old_index_next_ids )
   {
      _db.get_mutable_index( item.first.space(), item.first.type() ).set_next_id( item.second );
   }

   for( auto& item : state.removed )
      _db.insert( std::move(*item.second) );

   _stack.pop_back();
   if( _stack.empty() )
      _stack.emplace_back();
   enable();
   --_active_sessions;
} FC_CAPTURE_AND_RETHROW() }

void undo_database::merge()
{
   FC_ASSERT( _active_sessions > 0 );
   FC_ASSERT( _stack.size() >=2 );
   auto& state = _stack.back();
   auto& prev_state = _stack[_stack.size()-2];

   // An object's relationship to a state can be:
   // in new_ids            : new
   // in old_values (was=X) : upd(was=X)
   // in removed (was=X)    : del(was=X)
   // not in any of above   : nop
   //
   // When merging A=prev_state and B=state we have a 4x4 matrix of all possibilities:
   //
   //                   |--------------------- B ----------------------|
   //
   //                +------------+------------+------------+------------+
   //                | new        | upd(was=Y) | del(was=Y) | nop        |
   //   +------------+------------+------------+------------+------------+
   // / | new        | N/A        | new       A| nop       C| new       A|
   // | +------------+------------+------------+------------+------------+
   // | | upd(was=X) | N/A        | upd(was=X)A| del(was=X)C| upd(was=X)A|
   // A +------------+------------+------------+------------+------------+
   // | | del(was=X) | N/A        | N/A        | N/A        | del(was=X)A|
   // | +------------+------------+------------+------------+------------+
   // \ | nop        | new       B| upd(was=Y)B| del(was=Y)B| nop      AB|
   //   +------------+------------+------------+------------+------------+
   //
   // Each entry was composed by labelling what should occur in the given case.
   //
   // Type A means the composition of states contains the same entry as the first of the two merged states for that object.
   // Type B means the composition of states contains the same entry as the second of the two merged states for that object.
   // Type C means the composition of states contains an entry different from either of the merged states for that object.
   // Type N/A means the composition of states violates causal timing.
   // Type AB means both type A and type B simultaneously.
   //
   // The merge() operation is defined as modifying prev_state in-place to be the state object which represents the composition of
   // state A and B.
   //
   // Type A (and AB) can be implemented as a no-op; prev_state already contains the correct value for the merged state.
   // Type B (and AB) can be implemented by copying from state to prev_state.
   // Type C needs special case-by-case logic.
   // Type N/A can be ignored or assert(false) as it can only occur if prev_state and state have illegal values
   // (a serious logic error which should never happen).
   //

   // We can only be outside type A/AB (the nop path) if B is not nop, so it suffices to iterate through B's three containers.

   // *+upd
   for( auto& obj : state.old_values )
   {
      if( prev_state.new_ids.find(obj.second->id) != prev_state.new_ids.end() )
      {
         // new+upd -> new, type A
         continue;
      }
      if( prev_state.old_values.find(obj.second->id) != prev_state.old_values.end() )
      {
         // upd(was=X) + upd(was=Y) -> upd(was=X), type A
         continue;
      }
      // del+upd -> N/A
      assert( prev_state.removed.find(obj.second->id) == prev_state.removed.end() );
      // nop+upd(was=Y) -> upd(was=Y), type B
      prev_state.old_values[obj.second->id] = std::move(obj.second);
   }

   // *+new, but we assume the N/A cases don't happen, leaving type B nop+new -> new
   for( auto id : state.new_ids )
      prev_state.new_ids.insert(id);

   // old_index_next_ids can only be updated, iterate over *+upd cases
   for( auto& item : state.old_index_next_ids )
   {
      if( prev_state.old_index_next_ids.find( item.first ) == prev_state.old_index_next_ids.end() )
      {
         // nop+upd(was=Y) -> upd(was=Y), type B
         prev_state.old_index_next_ids[item.first] = item.second;
         continue;
      }
      else
      {
         // upd(was=X)+upd(was=Y) -> upd(was=X), type A
         // type A implementation is a no-op, as discussed above, so there is no code here
         continue;
      }
   }

   // *+del
   for( auto& obj : state.removed )
   {
      if( prev_state.new_ids.find(obj.second->id) != prev_state.new_ids.end() )
      {
         // new + del -> nop (type C)
         prev_state.new_ids.erase(obj.second->id);
         continue;
      }
      auto it = prev_state.old_values.find(obj.second->id);
      if( it != prev_state.old_values.end() )
      {
         // upd(was=X) + del(was=Y) -> del(was=X)
         prev_state.removed[obj.second->id] = std::move(it->second);
         prev_state.old_values.erase(obj.second->id);
         continue;
      }
      // del + del -> N/A
      assert( prev_state.removed.find( obj.second->id ) == prev_state.removed.end() );
      // nop + del(was=Y) -> del(was=Y)
      prev_state.removed[obj.second->id] = std::move(obj.second);
   }
   _stack.pop_back();
   --_active_sessions;
}
void undo_database::commit()
{
   FC_ASSERT( _active_sessions > 0 );
   --_active_sessions;
}

void undo_database::pop_commit()
{
   FC_ASSERT( _active_sessions == 0 );
   FC_ASSERT( !_stack.empty() );

   disable();
   try {
      auto& state = _stack.back();

      for( auto& item : state.old_values )
      {
         _db.modify( _db.get_object( item.second->id ), [&]( object& obj ){ obj.move_from( *item.second ); } );
      }

      for( auto ritr = state.new_ids.begin(); ritr != state.new_ids.end(); ++ritr  )
      {
         _db.remove( _db.get_object(*ritr) );
      }

      for( auto& item : state.old_index_next_ids )
      {
         _db.get_mutable_index( item.first.space(), item.first.type() ).set_next_id( item.second );
      }

      for( auto& item : state.removed )
         _db.insert( std::move(*item.second) );

      _stack.pop_back();
   }
   catch ( const fc::exception& e )
   {
      elog( "error popping commit ${e}", ("e", e.to_detail_string() )  );
      enable();
      throw;
   }
   enable();
}
void undo_database::set_max_size(size_t new_max_size) 
{ 
	_max_size = new_max_size; 
}
size_t undo_database::max_size() const { 
	return _max_size; 
}
const undo_state& undo_database::head()const
{
   FC_ASSERT( !_stack.empty() );
   return _stack.back();
}

 void undo_database::save_to_file(const fc::string & path)
{
    std::deque<serializable_undo_state>  out_stack;
    for (auto& item : _stack)
    {
        out_stack.push_back(item.get_serializable_undo_state());
    }
	if (out_stack.size() > 0)
	{
		printf("undo size save:%d\n", out_stack.size());
		fc::json::save_to_file(out_stack, path);
	}
        
}

 void undo_database::reset()
 {
     _stack.clear();
 }
 void undo_database::from_file(const fc::string & path)
{
	 if (!fc::exists(path))
		 return;
     try {
         std::deque<serializable_undo_state>  out_stack = fc::json::from_file(path).as<std::deque<serializable_undo_state>>();
         _stack.resize(out_stack.size());
         int num = 0;
         for (auto i : out_stack)
         {

             for (auto it = i.old_values.begin(); it != i.old_values.end(); it++)
             {
                 _stack[num].old_values[it->first] = it->second.to_object();
             }
             _stack[num].old_index_next_ids = i.old_index_next_ids;
             _stack[num].new_ids = i.new_ids;
             for (auto it = i.removed.begin(); it != i.removed.end(); it++)
             {
                 _stack[num].old_values[it->first] = it->second.to_object();
             }
             num++;
         }
	 }
	 catch (...)
	 {
		 FC_CAPTURE_AND_THROW(deserialize_undo_database_failed,(path));
	 }
}

inline serializable_undo_state undo_state::get_serializable_undo_state()
{
    serializable_undo_state res;
    for (auto i = old_values.begin(); i != old_values.end(); i++)
    {
        res.old_values[i->first] = serializable_obj(*(i->second));
    }
    res.old_index_next_ids = old_index_next_ids;
    res.new_ids = new_ids;
    for (auto i = removed.begin(); i != removed.end(); i++)
    {
        res.removed[i->first] = serializable_obj(*(i->second));
    }
    return res;
}

undo_state::undo_state(const serializable_undo_state & sta)
{
    for (auto i = sta.old_values.begin(); i != sta.old_values.end(); i++)
    {
        old_values[i->first] = i->second.to_object();
    }
    old_index_next_ids = sta.old_index_next_ids;
    new_ids = sta.new_ids;
    for (auto i = sta.removed.begin(); i != sta.removed.end(); i++)
    {
        removed[i->first] = i->second.to_object();
    }
}

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T> 
std::unique_ptr<object> create_obj_unique_ptr(const variant& var)
{
    std::unique_ptr<object> res = make_unique<T>(var.as<T>());
    return res;
}
inline db::serializable_obj::serializable_obj(const object & obj) :obj(obj.to_variant())
{
    s = obj.id.space();
    t = obj.id.type();
}
inline std::unique_ptr<object> to_protocol_object(uint8_t t,const variant& var)
{
    switch (t)
    {
    case account_object_type:      
        return create_obj_unique_ptr<account_object>(var);
        break;
    case asset_object_type: 
        return create_obj_unique_ptr<asset_object>(var);
        break;
    case force_settlement_object_type:  
        return create_obj_unique_ptr<force_settlement_object>(var);
        break;
    case guard_member_object_type:   
        return create_obj_unique_ptr<guard_member_object>(var);
        break;
    case miner_object_type:  
        return create_obj_unique_ptr<miner_object>(var);
        break;
    case limit_order_object_type:    
        return create_obj_unique_ptr<limit_order_object>(var);
        break;
    case call_order_object_type:   
        return create_obj_unique_ptr<call_order_object>(var);
        break;
    case custom_object_type:   
        //return create_obj_unique_ptr<custom_object>(var);
        break;
    case proposal_object_type:     
        return create_obj_unique_ptr<proposal_object>(var);
        break;
    case operation_history_object_type:   
        return create_obj_unique_ptr<operation_history_object>(var);
        break;
    case withdraw_permission_object_type:  
        return create_obj_unique_ptr<withdraw_permission_object>(var);
        break;
    case vesting_balance_object_type:    
        return create_obj_unique_ptr<vesting_balance_object>(var);
        break;
    case worker_object_type:     
        return create_obj_unique_ptr<worker_object>(var);
        break;
    case balance_object_type:    
        return create_obj_unique_ptr<balance_object>(var);
        break;
    case lockbalance_object_type:    
        return create_obj_unique_ptr<lockbalance_object>(var);
        break;
    case crosschain_trx_object_type:  
        return create_obj_unique_ptr<crosschain_trx_object>(var);
        break;
    case coldhot_transfer_object_type:
        return create_obj_unique_ptr<coldhot_transfer_object>(var);
        break;
    case guard_lock_balance_object_type: 
        return create_obj_unique_ptr<guard_lock_balance_object>(var);
        break;
    case multisig_transfer_object_type: 
        return create_obj_unique_ptr<multisig_asset_transfer_object>(var);
        break;
    case acquired_crosschain_object_type:  
        return create_obj_unique_ptr<acquired_crosschain_trx_object>(var);
        break;
    case crosschain_transaction_history_count_object_type:
        return create_obj_unique_ptr<crosschain_transaction_history_count_object>(var);
        break;
    case contract_object_type:  
        return create_obj_unique_ptr<contract_object>(var);
        break;
    case contract_balance_object_type:
        return create_obj_unique_ptr<contract_balance_object>(var);
        break;
    case contract_storage_object_type:  
        return create_obj_unique_ptr<contract_storage_object>(var);
        break;
    case contract_storage_diff_type:  
        return create_obj_unique_ptr<transaction_contract_storage_diff_object>(var);
        break;
    case contract_event_notify_object_type:   
        return create_obj_unique_ptr<contract_event_notify_object>(var);
        break;
    case contract_invoke_result_object_type:    
        return create_obj_unique_ptr<contract_invoke_result_object>(var);
        break;
    case pay_back_object_type: 
        return create_obj_unique_ptr<pay_back_object>(var);
        break;
    case contract_storage_change_object_type:
        return create_obj_unique_ptr<contract_storage_change_object>(var);
    default:
        return NULL;
    }
    return NULL;
}
inline std::unique_ptr<object> to_implementation_object(uint8_t t, const variant& var)
{
    switch (t)
    {
       case impl_global_property_object_type:
           return create_obj_unique_ptr< global_property_object>(var);
       case impl_dynamic_global_property_object_type: 
           return create_obj_unique_ptr< dynamic_global_property_object>(var);
       case impl_asset_dynamic_data_type: 
           return create_obj_unique_ptr< asset_dynamic_data_object>(var);
       case impl_asset_bitasset_data_type: 
           return create_obj_unique_ptr< asset_bitasset_data_object>(var);
       case impl_account_balance_object_type: 
           return create_obj_unique_ptr< account_balance_object>(var);
       case impl_account_binding_object_type:
           return create_obj_unique_ptr<account_binding_object>(var);
           break;
       case impl_account_statistics_object_type: 
           return create_obj_unique_ptr< account_statistics_object>(var);
       case impl_transaction_object_type: 
           return create_obj_unique_ptr< transaction_object>(var);
       case impl_trx_object_type: 
           return create_obj_unique_ptr< trx_object>(var);
       case impl_history_transaction_object_type: 
           return create_obj_unique_ptr< history_transaction_object>(var);
       case impl_block_summary_object_type: 
           return create_obj_unique_ptr< block_summary_object>(var);
       case impl_account_transaction_history_object_type: 
           return create_obj_unique_ptr <account_transaction_history_object> (var);
       case impl_chain_property_object_type: 
           return create_obj_unique_ptr< chain_property_object>(var);
       case impl_witness_schedule_object_type: 
           return create_obj_unique_ptr< witness_schedule_object>(var);
       case impl_budget_record_object_type: 
           return create_obj_unique_ptr< budget_record_object >(var);
       case impl_blinded_balance_object_type: 
           return create_obj_unique_ptr< blinded_balance_object >(var);
       case impl_special_authority_object_type: 
           return create_obj_unique_ptr< special_authority_object >(var);
       case impl_buyback_object_type: 
           return create_obj_unique_ptr< buyback_object >(var);
       case impl_fba_accumulator_object_type:
           return create_obj_unique_ptr< fba_accumulator_object >(var);
       case impl_multisig_account_binding_object_type:
           return create_obj_unique_ptr< multisig_account_pair_object >(var);       
       case impl_multisig_address_object_type:
           return create_obj_unique_ptr< multisig_address_object >(var);
           break;
       case impl_guarantee_obj_type: 
           return create_obj_unique_ptr< guarantee_object >(var);
       case impl_address_transaction_history_object_type:
           return create_obj_unique_ptr<address_transaction_history_object>(var);
       default:
           break;
    }
    return NULL;
}
std::unique_ptr<object> db::serializable_obj::to_object() const
{
    switch (s)
    {
    case chain::protocol_ids:
        return to_protocol_object(t,obj);
    case  chain::implementation_ids:
        return  to_implementation_object(t,obj);
    default:
        throw;
    }
}
serializable_undo_state::serializable_undo_state(const serializable_undo_state & sta) 
{
    this->new_ids = sta.new_ids;
    this->old_index_next_ids = sta.old_index_next_ids;
    this->old_values = sta.old_values;
    this->removed = sta.removed;
}
}

} // graphene::db
