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

#include <graphene/app/database_api.hpp>
#include <graphene/chain/get_config.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>

#include <fc/crypto/hex.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/contract_object.hpp>
#include<graphene/chain/witness_schedule_object.hpp>
#include <cctype>
#include <fc/ntp.hpp>
#include <cfenv>
#include <iostream>
#include <fc/variant_object.hpp>
#define GET_REQUIRED_FEES_MAX_RECURSION 4

typedef std::map< std::pair<graphene::chain::asset_id_type, graphene::chain::asset_id_type>, std::vector<fc::variant> > market_queue_type;

namespace graphene { namespace app {

class database_api_impl;


class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
   public:
      database_api_impl( graphene::chain::database& db );
      ~database_api_impl();

      // Objects
      fc::variants get_objects(const vector<object_id_type>& ids)const;

      // Subscriptions
      void set_subscribe_callback( std::function<void(const variant&)> cb, bool notify_remove_create );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      void cancel_all_subscriptions();

      // Blocks and transactions
      optional<block_header> get_block_header(uint32_t block_num)const;
      map<uint32_t, optional<block_header>> get_block_header_batch(const vector<uint32_t> block_nums)const;
      optional<signed_block> get_block(uint32_t block_num)const;
      processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;
	  optional<graphene::chain::full_transaction> get_transaction_by_id(transaction_id_type id) const;
	  void set_acquire_block_num(const string& symbol, const uint32_t& block_num)const;
	  std::vector<acquired_crosschain_trx_object> get_acquire_transaction(const int & type, const string & trxid);
      // Globals
      chain_property_object get_chain_properties()const;
      global_property_object get_global_properties()const;
      fc::variant_object get_config()const;
      chain_id_type get_chain_id()const;
      dynamic_global_property_object get_dynamic_global_properties()const;
      // Keys
      vector<vector<account_id_type>> get_key_references( vector<public_key_type> key )const;
     bool is_public_key_registered(string public_key) const;

      // Accounts
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;
	  vector<optional<account_object>> get_accounts(const vector<string>& account_names)const;
	  vector<optional<account_object>> get_accounts(const vector<address>& account_addres)const;
      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );
      optional<account_object> get_account_by_name( string name )const;
      vector<account_id_type> get_account_references( account_id_type account_id )const;
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;
      map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_account_count()const;

      // Balances
      vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;
      vector<balance_object> get_balance_objects( const vector<address>& addrs )const;
      vector<asset> get_vested_balances( const vector<balance_id_type>& objs )const;
      vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;
	  optional<whiteOperationList_object> get_whiteOperation(const address& addr) const;
      // Assets
      vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;
      vector<asset_object>           list_assets(const string& lower_bound_symbol, uint32_t limit)const;
      vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

      // Markets / feeds
      vector<limit_order_object>         get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const;
      vector<call_order_object>          get_call_orders(asset_id_type a, uint32_t limit)const;
      vector<force_settlement_object>    get_settle_orders(asset_id_type a, uint32_t limit)const;
      vector<call_order_object>          get_margin_positions( const account_id_type& id )const;
      void subscribe_to_market(std::function<void(const variant&)> callback, asset_id_type a, asset_id_type b);
      void unsubscribe_from_market(asset_id_type a, asset_id_type b);
      market_ticker                      get_ticker( const string& base, const string& quote )const;
      market_volume                      get_24_volume( const string& base, const string& quote )const;
      order_book                         get_order_book( const string& base, const string& quote, unsigned limit = 50 )const;
      vector<market_trade>               get_trade_history( const string& base, const string& quote, fc::time_point_sec start, fc::time_point_sec stop, unsigned limit = 100 )const;

      // Witnesses
      vector<optional<miner_object>> get_miners(const vector<miner_id_type>& witness_ids)const;
      fc::optional<miner_object> get_miner_by_account(account_id_type account)const;
      map<string, miner_id_type> lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_miner_count()const;

      // Committee members
      vector<optional<guard_member_object>> get_guard_members(const vector<guard_member_id_type>& committee_member_ids)const;
      fc::optional<guard_member_object> get_guard_member_by_account(account_id_type account)const;
      map<string, guard_member_id_type> lookup_guard_member_accounts(const string& lower_bound_name, uint32_t limit,bool formal=true)const;

      // Votes
      vector<variant> lookup_vote_ids( const vector<vote_id_type>& votes )const;

      // Authority / validation
      std::string get_transaction_hex(const signed_transaction& trx)const;
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      set<address> get_potential_address_signatures( const signed_transaction& trx )const;
      bool verify_authority( const signed_transaction& trx )const;
      bool verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;
      processed_transaction validate_transaction( const signed_transaction& trx ,bool testing=false)const;
      vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;

      // Proposed transactions
      vector<proposal_object> get_proposed_transactions( account_id_type id )const;
	  vector<proposal_object> get_proposer_transactions(account_id_type id)const;
	  vector<proposal_object> get_voter_transactions_waiting(address addr)const;
	  vector<referendum_object> get_referendum_transactions_waiting(address addr) const;
	  optional<referendum_object> get_referendum_object(const referendum_id_type& id) const;
      // Blinded balances
      //vector<blinded_balance_object> get_blinded_balances( const flat_set<commitment_type>& commitments )const;
	  //Lock balance
	  vector<lockbalance_object> get_account_lock_balance(const account_id_type& id)const;
	  vector<lockbalance_object> get_asset_lock_balance(const asset_id_type& asset) const;
	  vector<lockbalance_object> get_miner_lock_balance(const miner_id_type& miner) const;
	  vector<optional<account_binding_object>> get_binding_account(const string& account, const string& symbol) const;
	  vector<guard_lock_balance_object> get_guard_lock_balance(const guard_member_id_type& id)const;
	  vector<guard_lock_balance_object> get_guard_asset_lock_balance(const asset_id_type& id)const;
	  vector<optional<multisig_address_object>> get_multisig_address_obj(const string& symbol, const account_id_type& guard) const;
	  vector<multisig_asset_transfer_object> get_multisigs_trx() const;
	  optional<multisig_asset_transfer_object> lookup_multisig_asset(multisig_asset_transfer_id_type id) const;
	  vector<crosschain_trx_object> get_crosschain_transaction(const transaction_stata& crosschain_trx_state,const transaction_id_type& id)const;
	  vector<crosschain_trx_object> get_account_crosschain_transaction(const string& account,const transaction_id_type& id)const;
	  vector<optional<crosschain_trx_object>> get_crosschain_transaction_by_blocknum(const string& symbol,const string& account,const uint32_t& start_block_num,const uint32_t& stop_block_num,const transaction_stata& crosschain_trx_state)const;
	  vector<optional<multisig_address_object>> get_multi_account_guard(const string & multi_address, const string& symbol)const;
	  std::map<std::string, asset> get_pay_back_balances(const address & pay_back_owner)const;
	  vector<vote_result_object> get_vote_result_objs(const vote_object_id_type& id) const;
	  vector<vote_object> get_votes_by_addr(const address& addr) const;
	  std::map<std::string, share_type> get_bonus_balances(const address & owner)const;
	  vector<coldhot_transfer_object> get_coldhot_transaction(const coldhot_trx_state& coldhot_tx_state, const transaction_id_type& id)const;
	  vector<optional<coldhot_transfer_object>> get_coldhot_transaction_by_blocknum(const string& symbol, const uint32_t& start_block_num, const uint32_t& stop_block_num, const coldhot_trx_state& crosschain_trx_state)const;
	  vector<optional<multisig_account_pair_object>> get_multisig_account_pair(const string& symbol) const;
	  optional<multisig_account_pair_object> lookup_multisig_account_pair(const multisig_account_pair_id_type& id) const;

      //contract 
      contract_object get_contract_object(const string& contract_address)const ;
	  contract_object get_contract_object_by_name(const string& contract_name) const;
      vector<asset> get_contract_balance(const address & contract_address) const;
	  vector<optional<guarantee_object>> list_guarantee_object(const string& chain_type,bool all) const;
	  optional<guarantee_object> get_gurantee_object(const guarantee_object_id_type id) const;
	  vector<optional<guarantee_object>> get_guarantee_orders(const address& addr, bool all) const;
      optional<contract_event_notify_object> get_contract_event_notify_by_id(const contract_event_notify_object_id_type& id);
      vector<contract_invoke_result_object> get_contract_invoke_object(const transaction_id_type& trx_id)const;
      vector<address> get_contract_addresses_by_owner(const address&)const;
      vector<contract_object> get_contracts_by_owner(const address&addr )const ;
      vector<contract_event_notify_object> get_contract_events(const address&)const;
	  vector<contract_event_notify_object> get_contract_events(const address &addr, uint64_t start, uint64_t range) const;
      vector<contract_object> get_contract_registered(const uint32_t start_with, const uint32_t num)const ;

      vector<contract_blocknum_pair> get_contract_storage_changed(const uint32_t block_num , const uint32_t num)const ;
	  map<account_id_type, vector<asset>> get_citizen_lockbalance_info(const miner_id_type& id) const;
	  vector<miner_id_type> list_scheduled_citizens() const;
	  vector<fc::optional<eth_multi_account_trx_object>> get_eths_multi_create_account_trx(const eth_multi_account_trx_state trx_state, const transaction_id_type trx_id)const;
	  vector<fc::optional<otc_contract_object>> get_otc_contract_object(const string from_asset, const string to_asset, const uint32_t limit)const;
	  fc::uint128 get_pledge() const;
   //private:
      template<typename T>
      void subscribe_to_item( const T& i )const
      {
         auto vec = fc::raw::pack(i);
         if( !_subscribe_callback )
            return;

         if( !is_subscribed_to_item(i) )
         {
            idump((i));
            _subscribe_filter.insert( vec.data(), vec.size() );//(vecconst char*)&i, sizeof(i) );
         }
      }

      template<typename T>
      bool is_subscribed_to_item( const T& i )const
      {
         if( !_subscribe_callback )
            return false;

         return _subscribe_filter.contains( i );
      }

      bool is_impacted_account( const flat_set<account_id_type>& accounts)
      {
         if( !_subscribed_accounts.size() || !accounts.size() )
            return false;

         return std::any_of(accounts.begin(), accounts.end(), [this](const account_id_type& account) {
            return _subscribed_accounts.find(account) != _subscribed_accounts.end();
         });
      }

      template<typename T>
      void enqueue_if_subscribed_to_market(const object* obj, market_queue_type& queue, bool full_object=true)
      {
         const T* order = dynamic_cast<const T*>(obj);
         FC_ASSERT( order != nullptr);

         auto market = order->get_market();

         auto sub = _market_subscriptions.find( market );
         if( sub != _market_subscriptions.end() ) {
            queue[market].emplace_back( full_object ? obj->to_variant() : fc::variant(obj->id) );
         }
      }

      void broadcast_updates( const vector<variant>& updates );
      void broadcast_market_updates( const market_queue_type& queue);
      void handle_object_changed(bool force_notify, bool full_object, const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts, std::function<const object*(object_id_type id)> find_object);

      /** called every time a block is applied to report the objects that were changed */
      void on_objects_new(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts);
      void on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts);
      void on_objects_removed(const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_id_type>& impacted_accounts);
      void on_applied_block();

      bool _notify_remove_create = false;
      mutable fc::bloom_filter _subscribe_filter;
      std::set<account_id_type> _subscribed_accounts;
      std::function<void(const fc::variant&)> _subscribe_callback;
      std::function<void(const fc::variant&)> _pending_trx_callback;
      std::function<void(const fc::variant&)> _block_applied_callback;

      boost::signals2::scoped_connection                                                                                           _new_connection;
      boost::signals2::scoped_connection                                                                                           _change_connection;
      boost::signals2::scoped_connection                                                                                           _removed_connection;
      boost::signals2::scoped_connection                                                                                           _applied_block_connection;
      boost::signals2::scoped_connection                                                                                           _pending_trx_connection;
      map< pair<asset_id_type,asset_id_type>, std::function<void(const variant&)> >      _market_subscriptions;
      graphene::chain::database&                                                                                                            _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api( graphene::chain::database& db )
   : my( new database_api_impl( db ) ) {}

database_api::~database_api() {}

database_api_impl::database_api_impl( graphene::chain::database& db ):_db(db)
{
   wlog("creating database api ${x}", ("x",int64_t(this)) );
   _new_connection = _db.new_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts) {
                                on_objects_new(ids, impacted_accounts);
                                });
   _change_connection = _db.changed_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts) {
                                on_objects_changed(ids, impacted_accounts);
                                });
   _removed_connection = _db.removed_objects.connect([this](const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_id_type>& impacted_accounts) {
                                on_objects_removed(ids, objs, impacted_accounts);
                                });
   _applied_block_connection = _db.applied_block.connect([this](const signed_block&){ on_applied_block(); });

   _pending_trx_connection = _db.on_pending_transaction.connect([this](const signed_transaction& trx ){
                         if( _pending_trx_callback ) _pending_trx_callback( fc::variant(trx) );
                      });
}

database_api_impl::~database_api_impl()
{
   elog("freeing database api ${x}", ("x",int64_t(this)) );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Objects                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

fc::variants database_api::get_objects(const vector<object_id_type>& ids)const
{
   return my->get_objects( ids );
}

fc::variants database_api_impl::get_objects(const vector<object_id_type>& ids)const
{
   if( _subscribe_callback )  {
      for( auto id : ids )
      {
         if( id.type() == operation_history_object_type && id.space() == protocol_ids ) continue;
         if( id.type() == impl_account_transaction_history_object_type && id.space() == implementation_ids ) continue;

         this->subscribe_to_item( id );
      }
   }

   fc::variants result;
   result.reserve(ids.size());

   std::transform(ids.begin(), ids.end(), std::back_inserter(result),
                  [this](object_id_type id) -> fc::variant {
      if(auto obj = _db.find_object(id))
         return obj->to_variant();
      return {};
   });

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_subscribe_callback( std::function<void(const variant&)> cb, bool notify_remove_create )
{
   my->set_subscribe_callback( cb, notify_remove_create );
}

void database_api_impl::set_subscribe_callback( std::function<void(const variant&)> cb, bool notify_remove_create )
{
   //edump((clear_filter));
   _subscribe_callback = cb;
   _notify_remove_create = notify_remove_create;
   _subscribed_accounts.clear();

   static fc::bloom_parameters param;
   param.projected_element_count    = 10000;
   param.false_positive_probability = 1.0/100;
   param.maximum_size = 1024*8*8*2;
   param.compute_optimal_parameters();
   _subscribe_filter = fc::bloom_filter(param);
}

static void load_apis_in_contract_object_if_native(contract_object& cont) {
	// if is native contract, get contract apis and fill it
    if(cont.type_of_contract == contract_type::native_contract) {
        auto native_contract = native_contract_finder::create_native_contract_by_key(nullptr, cont.native_contract_key, cont.contract_address);
        if(native_contract) { 
                cont.code.abi = native_contract->apis();
                //cont.code.abi.clear();
                //for(const auto& api : native_contract->apis()) {
                //      cont.code.abi.push(api);
                //}
                cont.code.offline_abi = native_contract->offline_apis();
                //cont.code.offline_abi.clear();
                //for(const auto& api : native_contract->offline_apis()) {
                //        cont.code.offline_abi.push(api);
                //}
                cont.code.events = native_contract->events();
                //cont.code.events.clear();
                //for(const auto& event : native_contract->events()) {
                //        cont.code.events.push(event);
                //}
        }
    }
}

contract_object database_api::get_contract_object(const string& contract_address) const
{
    auto cont =  my->get_contract_object(contract_address);
    load_apis_in_contract_object_if_native(cont);
    return cont;
}
ContractEntryPrintable database_api::get_simple_contract_info(const string & contract_address_or_name) const
{
	std::string contract_address;
	try {
		auto temp = graphene::chain::address(contract_address_or_name);
		FC_ASSERT(temp.version == addressVersion::CONTRACT);
		contract_address = temp.operator fc::string();
	}
	catch (fc::exception& e)
	{
		auto cont =get_contract_object_by_name(contract_address_or_name);
		contract_address = string(cont.contract_address);
	}
	ContractEntryPrintable result = get_contract_object(contract_address);
	result.code_printable.printable_code = "";
	return result;
}
ContractEntryPrintable database_api::get_contract_info_by_name(const string& contract_name)const
{
    return my->get_contract_object_by_name(contract_name);
}
fc::ntp_info database_api::get_ntp_info() const
{
	return fc::time_point::get_ntp_info();
}
std::pair<bool, fc::variant_object> database_api::get_sync_mode_network_info() {
	return std::make_pair(bool(my->_db.sync_mode), my->_db._network_get_info_data);
}

void database_api::ntp_update_time() const
{
	time_point::ntp_update_time();
}

void database_api::set_gas_limit_in_block(const share_type& new_limit)
{
	my->_db.set_gas_limit_in_block(new_limit);
}

std::vector<fc::ip::endpoint> database_api::get_midware_eps()
{
	return abstract_crosschain_interface::get_midware_eps();
}

ContractEntryPrintable database_api::get_contract_info(const string& contract_address)const
{
    return my->get_contract_object(contract_address);
}
contract_object database_api::get_contract_object_by_name(const string& contract_name) const
{
	auto cont =  my->get_contract_object_by_name(contract_name);
	load_apis_in_contract_object_if_native(cont);
	return cont;
}
contract_object database_api_impl::get_contract_object(const string& contract_address) const
{
    try {
        auto cont = _db.get_contract(address(contract_address));
	load_apis_in_contract_object_if_native(cont);
	return cont;
    }FC_CAPTURE_AND_RETHROW((contract_address))
}
contract_object database_api_impl::get_contract_object_by_name(const string& contract_name) const
{
	try {
		auto res = _db.get_contract_of_name(contract_name);
		return res;
	}FC_CAPTURE_AND_RETHROW((contract_name))
}
vector<asset> database_api_impl::get_contract_balance(const address & contract_address) const
{
    vector<asset> res;
    auto& db=_db.get_index_type<balance_index>().indices().get<by_owner>();
    for (auto it : db)
    {
        if (it.owner == contract_address)
        {
            res.push_back(it.balance);
        }
    }
    return res;
}
void database_api::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   my->set_pending_transaction_callback( cb );
}

void database_api_impl::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   _pending_trx_callback = cb;
}

void database_api::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   my->set_block_applied_callback( cb );
}

void database_api_impl::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   _block_applied_callback = cb;
}

void database_api::cancel_all_subscriptions()
{
   my->cancel_all_subscriptions();
}
vector<asset> database_api::get_contract_balance(const address & contract_address) const
{
    return my->get_contract_balance(contract_address);
}
vector<address> database_api::get_contract_addresses_by_owner_address(const address&addr)const
{
    return my->get_contract_addresses_by_owner(addr);
}

vector<string>  database_api::get_contract_addresses_by_owner(const std::string& addr)const
{
    address owner_addr;
    if (address::is_valid(addr))
    {
        owner_addr = address(addr);
    }
    else
    {
        auto acct = get_account(addr);
        owner_addr = acct.addr;
    }
    auto addr_res = get_contract_addresses_by_owner_address(owner_addr);
    vector<string> res;
    for (auto& out : addr_res)
    {
        res.push_back(string(out));
    }
    return res;
}
vector<contract_object> database_api::get_contract_objs_by_owner(const address&addr)const 
{
    return my->get_contracts_by_owner(addr);
}

vector<ContractEntryPrintable> database_api::get_contracts_by_owner(const std::string& addr) const
{
    vector<ContractEntryPrintable> res;
    address owner_addr;
    if (address::is_valid(addr))
    {
        owner_addr = address(addr);
    }
    else
    {
        auto acct = get_account(addr);
        owner_addr = acct.addr;
    }
    auto objs = get_contract_objs_by_owner(owner_addr);
    for (auto& obj : objs)
    {
        res.push_back(obj);
    }
    return res;
}

vector<contract_hash_entry> database_api::get_contracts_hash_entry_by_owner(const std::string& addr) const
{
    address owner_addr;
    if (address::is_valid(addr))
    {
        owner_addr = address(addr);
    }
    else
    {
        auto acct = get_account(addr);
        owner_addr = acct.addr;
    }
    auto contracts = get_contract_objs_by_owner(owner_addr);
    vector<contract_hash_entry> res;
    for (auto& co : contracts)
    {
        res.push_back(co);
    }
    return res;
}
void database_api_impl::cancel_all_subscriptions()
{
   set_subscribe_callback( std::function<void(const fc::variant&)>(), true);
   _market_subscriptions.clear();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

optional<block_header> database_api::get_block_header(uint32_t block_num)const
{
   return my->get_block_header( block_num );
}

optional<block_header> database_api_impl::get_block_header(uint32_t block_num) const
{
   auto result = _db.fetch_block_by_number(block_num);
   if(result)
      return *result;
   return {};
}
map<uint32_t, optional<block_header>> database_api::get_block_header_batch(const vector<uint32_t> block_nums)const
{
   return my->get_block_header_batch( block_nums );
}

map<uint32_t, optional<block_header>> database_api_impl::get_block_header_batch(const vector<uint32_t> block_nums) const
{
   map<uint32_t, optional<block_header>> results;
   for (const uint32_t block_num : block_nums)
   {
      results[block_num] = get_block_header(block_num);
   }
   return results;
}
vector<optional<multisig_account_pair_object> > database_api_impl::get_multisig_account_pair(const string& symbol) const
{
    const auto& multisig_accounts_byid = _db.get_index_type<multisig_account_pair_index>().indices().get<by_id>();
	//std::cout <<"multisig account size: "<< multisig_accounts_byid.size() << std::endl;
	vector<optional<multisig_account_pair_object>> result;
	for (const auto & one_multisig_obj : multisig_accounts_byid)
	{
		if (one_multisig_obj.chain_type == symbol)
			result.push_back(one_multisig_obj);
	}
	//std::transform(multisig_accounts_byid.begin(), multisig_accounts_byid.end(), std::back_inserter(result),
	//	[&multisig_accounts_byid,&symbol](const multisig_account_pair_object& obj) -> optional<multisig_account_pair_object> {
	//	if (obj.chain_type == symbol)
	//		return obj;
	//	//return  optional<multisig_account_pair_object>() ;
	//});
	return result;
}
optional<multisig_account_pair_object> database_api_impl::lookup_multisig_account_pair(const multisig_account_pair_id_type& id) const
{
	const auto& multisig_accounts_byid = _db.get_index_type<multisig_account_pair_index>().indices().get<by_id>();
	const auto iter = multisig_accounts_byid.find(id);
	if (iter != multisig_accounts_byid.end())
		return *iter;
	return optional < multisig_account_pair_object>();
}

map<account_id_type, vector<asset>> database_api_impl::get_citizen_lockbalance_info(const miner_id_type& id) const
{
	map<account_id_type, vector<asset>> result;
	const auto& index = _db.get_index_type<lockbalance_index>().indices().get<by_lock_miner_asset>();
	auto range = index.equal_range(boost::make_tuple(id));
	for (auto localbalance_obj : boost::make_iterator_range(range.first, range.second)) {
		if (localbalance_obj.lockto_miner_account != id)
			continue;
		if (result.count(localbalance_obj.lock_balance_account) == 0)
		{
			vector<asset> temp;
			temp.push_back(asset(localbalance_obj.lock_asset_amount,localbalance_obj.lock_asset_id));
			result[localbalance_obj.lock_balance_account] = temp;
		}
		else
		{
			result[localbalance_obj.lock_balance_account].push_back(asset(localbalance_obj.lock_asset_amount, localbalance_obj.lock_asset_id));
		}
	}
	return result;
}


optional<signed_block> database_api::get_block(uint32_t block_num)const
{
	auto block = my->get_block(block_num);
   return block;
}
vector<full_transaction> database_api::fetch_block_transactions(uint32_t block_num)const
{
	vector<full_transaction> results;
	auto block = my->get_block(block_num);
	for (const auto& tx : block->transactions) {
		//std::cout << "database get_block by numberx " << tx.id().str() << std::endl;
		results.push_back(tx);
	}
	return results;
}
void database_api::set_acquire_block_num(const string& symbol, const uint32_t& block_num)const {
	my->set_acquire_block_num(symbol, block_num);
}
std::vector<acquired_crosschain_trx_object> database_api::get_acquire_transaction(const int & type, const string & trxid) {
	return my->get_acquire_transaction(type, trxid);
}
std::vector<acquired_crosschain_trx_object> database_api_impl::get_acquire_transaction(const int & type, const string & trxid) {
	vector<acquired_crosschain_trx_object> results;
	if ("" != trxid)
	{
		auto& ac_db = _db.get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
		auto ac_iter = ac_db.find(trxid);
		FC_ASSERT(ac_iter != ac_db.end(), "no transaction found");
		results.push_back(*ac_iter);
		
	}
	else {
		auto range = _db.get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_state>().equal_range((acquired_trx_state)type);
		for (auto crosschain_obj : boost::make_iterator_range(range.first, range.second)) {
			results.push_back(crosschain_obj);
		}
	}
	return results;
}
void database_api_impl::set_acquire_block_num(const string& symbol, const uint32_t& block_num)const {
	auto& trx_iters = _db.get_index_type<graphene::chain::transaction_history_count_index>().indices().get<graphene::chain::by_history_asset_symbol>();
	auto symbol_iter = trx_iters.find(symbol);
	FC_ASSERT((symbol_iter != trx_iters.end()), "symbol not found!");
	_db.modify(*symbol_iter, [&](crosschain_transaction_history_count_object& obj) {
		obj.local_count = block_num;
	});
}
optional<signed_block> database_api_impl::get_block(uint32_t block_num)const
{
   return _db.fetch_block_by_number(block_num);
}

processed_transaction database_api::get_transaction( uint32_t block_num, uint32_t trx_in_block )const
{
   return my->get_transaction( block_num, trx_in_block );
}

optional<graphene::chain::full_transaction> database_api::get_transaction_by_id(transaction_id_type trx_id) const
{
	return my->get_transaction_by_id(trx_id);
}

optional<signed_transaction> database_api::get_recent_transaction_by_id( const transaction_id_type& id )const
{
   try {
      return my->_db.get_recent_transaction( id );
   } catch ( ... ) {
      return optional<signed_transaction>();
   }
}

processed_transaction database_api_impl::get_transaction(uint32_t block_num, uint32_t trx_num)const
{
   auto opt_block = _db.fetch_block_by_number(block_num);
   FC_ASSERT( opt_block );
   FC_ASSERT( opt_block->transactions.size() > trx_num );
   return opt_block->transactions[trx_num];
}

optional<graphene::chain::full_transaction> database_api_impl::get_transaction_by_id(transaction_id_type id) const
{
	const auto res_ids = _db.fetch_trx(id);
	FC_ASSERT(res_ids.valid());
	full_transaction res = res_ids->trx;
	res.block_num = res_ids->block_num;
	auto invoke_res = _db.get_contract_invoke_result(id);
	if (invoke_res.size() == 0)
		return res;
	for (auto& ir : invoke_res)
	{
		FC_ASSERT(ir.op_num < res.operations.size());
		auto& op = res.operations[ir.op_num];
		switch (op.which())
		{
		case operation::tag<contract_invoke_operation>::value:
			op.get<contract_invoke_operation>().fee.amount = ir.acctual_fee;
			break;
		case operation::tag<contract_register_operation>::value:
			op.get<contract_register_operation>().fee.amount = ir.acctual_fee;
			break;
		case operation::tag<native_contract_register_operation>::value:
			op.get<native_contract_register_operation>().fee.amount = ir.acctual_fee;
			break;
		case operation::tag<contract_upgrade_operation>::value:
			op.get<contract_upgrade_operation>().fee.amount = ir.acctual_fee;
			break;
		case operation::tag<transfer_contract_operation>::value:
			op.get<transfer_contract_operation>().fee.amount = ir.acctual_fee;
			break;
		default:
			FC_THROW("Invoke result exsited but no operation related to contract");
		}
	}
	return res;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

chain_property_object database_api::get_chain_properties()const
{
   return my->get_chain_properties();
}

chain_property_object database_api_impl::get_chain_properties()const
{
   return _db.get(chain_property_id_type());
}

global_property_object database_api::get_global_properties()const
{
   return my->get_global_properties();
}

global_property_object database_api_impl::get_global_properties()const
{
   return _db.get(global_property_id_type());
}

fc::variant_object database_api::get_config()const
{
   return my->get_config();
}

fc::variant_object database_api_impl::get_config()const
{
   return graphene::chain::get_config();
}

chain_id_type database_api::get_chain_id()const
{
   return my->get_chain_id();
}

chain_id_type database_api_impl::get_chain_id()const
{
   return _db.get_chain_id();
}

dynamic_global_property_object database_api::get_dynamic_global_properties()const
{
   return my->get_dynamic_global_properties();
}

void database_api::set_guarantee_id(guarantee_object_id_type id)
{
	return ;
}
dynamic_global_property_object database_api_impl::get_dynamic_global_properties()const
{
   return _db.get(dynamic_global_property_id_type());
}




//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<vector<account_id_type>> database_api::get_key_references( vector<public_key_type> key )const
{
   return my->get_key_references( key );
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
vector<vector<account_id_type>> database_api_impl::get_key_references( vector<public_key_type> keys )const
{
   wdump( (keys) );
   vector< vector<account_id_type> > final_result;
   final_result.reserve(keys.size());

   for( auto& key : keys )
   {

      address a1( pts_address(key, false, 56) );
      address a2( pts_address(key, true, 56) );
      address a3( pts_address(key, false, 0)  );
      address a4( pts_address(key, true, 0)  );
      address a5( key );

      subscribe_to_item( key );
      subscribe_to_item( a1 );
      subscribe_to_item( a2 );
      subscribe_to_item( a3 );
      subscribe_to_item( a4 );
      subscribe_to_item( a5 );

      const auto& idx = _db.get_index_type<account_index>();
      const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
      const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
      auto itr = refs.account_to_key_memberships.find(key);
      vector<account_id_type> result;

      for( auto& a : {a1,a2,a3,a4,a5} )
      {
          auto itr = refs.account_to_address_memberships.find(a);
          if( itr != refs.account_to_address_memberships.end() )
          {
             result.reserve( itr->second.size() );
             for( auto item : itr->second )
             {
                wdump((a)(item)(item(_db).name));
                result.push_back(item);
             }
          }
      }

      if( itr != refs.account_to_key_memberships.end() )
      {
         result.reserve( itr->second.size() );
         for( auto item : itr->second ) result.push_back(item);
      }
      final_result.emplace_back( std::move(result) );
   }

   for( auto i : final_result )
      subscribe_to_item(i);

   return final_result;
}

bool database_api::is_public_key_registered(string public_key) const
{
    return my->is_public_key_registered(public_key);
}

bool database_api_impl::is_public_key_registered(string public_key) const
{
    // Short-circuit
    if (public_key.empty()) {
        return false;
    }

    // Search among all keys using an existing map of *current* account keys
    public_key_type key;
    try {
        key = public_key_type(public_key);
    } catch ( ... ) {
        // An invalid public key was detected
        return false;
    }
    const auto& idx = _db.get_index_type<account_index>();
    const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
    const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
    auto itr = refs.account_to_key_memberships.find(key);
    bool is_known = itr != refs.account_to_key_memberships.end();

    return is_known;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<account_object>> database_api::get_accounts(const vector<account_id_type>& account_ids)const
{
   return my->get_accounts( account_ids );
}

vector<optional<account_object>> database_api::get_accounts_addr(const vector<address>& account_addres)const
{
	return my->get_accounts(account_addres);
}

vector<optional<account_object>> database_api_impl::get_accounts(const vector<address>& account_addres)const
{
	const auto& accounts_by_addr = _db.get_index_type<account_index>().indices().get<by_address>();
	vector<optional<account_object> > result;
	result.reserve(account_addres.size());
	std::transform(account_addres.begin(), account_addres.end(), std::back_inserter(result),
		[&accounts_by_addr](const address& addr) -> optional<account_object> {
		auto itr = accounts_by_addr.find(addr);
		return itr == accounts_by_addr.end() ? optional<account_object>() : *itr;
	});
	return result;
}

vector<optional<account_object>> database_api_impl::get_accounts(const vector<account_id_type>& account_ids)const
{
   vector<optional<account_object>> result; result.reserve(account_ids.size());
   std::transform(account_ids.begin(), account_ids.end(), std::back_inserter(result),
                  [this](account_id_type id) -> optional<account_object> {
      if(auto o = _db.find(id))
      {
         subscribe_to_item( id );
         return *o;
      }
      return {};
   });
   return result;
}


std::map<string,full_account> database_api::get_full_accounts( const vector<string>& names_or_ids, bool subscribe )
{
   return my->get_full_accounts( names_or_ids, subscribe );
}
account_object database_api::get_account(const string& account_name_or_id) const
{
    FC_ASSERT(account_name_or_id.size() > 0);

    if (auto id = maybe_id<account_id_type>(account_name_or_id))
    {
        // It's an ID
        return get_account_by_id(*id);
    }
    else {

        auto rec = lookup_account_names({ account_name_or_id }).front();
        FC_ASSERT(rec && rec->name == account_name_or_id);
        return *rec;
    }
}
account_object database_api::get_account_by_id(const account_id_type& id) const
{
    vector<account_id_type> input;
    input.push_back(id);
    auto output = my->get_accounts(input);
    FC_ASSERT(output.front().valid());
    return *(output.front());
}
asset_object database_api::get_asset(const string& asset_name_or_id) const
{
    FC_ASSERT(asset_name_or_id.size() > 0);

    if (auto id = maybe_id<asset_id_type>(asset_name_or_id))
    {
        // It's an ID
        auto obj = get_asset_by_id(*id);
        FC_ASSERT(obj.valid());
        return *obj;
    }
    else {
        // It's a symbol
        auto rec = lookup_asset_symbols({ asset_name_or_id }).front();
        FC_ASSERT(rec.valid());
        FC_ASSERT(rec->symbol == asset_name_or_id);
        return *rec;
    }
    return asset_object();
}
optional<asset_object> database_api::get_asset_by_id(const asset_id_type& id)const
{
    vector<asset_id_type> input;
    input.push_back(id);
    auto out = get_assets(input);
    if (out.front().valid())
        return *(out.front());
    return optional<asset_object>();
}

account_id_type database_api::get_account_id(const string& account_name_or_id) const
{
    return get_account(account_name_or_id).get_id();
}
address database_api::get_account_addr(const string& account_name) const
{
    return my->_db.get_account_address(account_name);
}
asset_id_type database_api::get_asset_id(const string& asset_name_or_id) const
{
    return get_asset(asset_name_or_id).id;
}
std::map<std::string, full_account> database_api_impl::get_full_accounts( const vector<std::string>& names_or_ids, bool subscribe)
{
   idump((names_or_ids));
   std::map<std::string, full_account> results;

   for (const std::string& account_name_or_id : names_or_ids)
   {
      const account_object* account = nullptr;
	  if (std::isdigit(account_name_or_id[0]))
		  
		  account = _db.find(fc::variant(account_name_or_id).as<account_id_type>());
      else
      {
         const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
         auto itr = idx.find(account_name_or_id);
         if (itr != idx.end())
            account = &*itr;
      }
      if (account == nullptr)
         continue;

      if( subscribe )
      {
         FC_ASSERT( std::distance(_subscribed_accounts.begin(), _subscribed_accounts.end()) <= 100 );
         _subscribed_accounts.insert( account->get_id() );
         subscribe_to_item( account->id );
      }

      // fc::mutable_variant_object full_account;
      full_account acnt;
      acnt.account = *account;
      acnt.statistics = account->statistics(_db);
      acnt.registrar_name = account->registrar(_db).name;
      acnt.referrer_name = account->referrer(_db).name;
      acnt.lifetime_referrer_name = account->lifetime_referrer(_db).name;
      acnt.votes = lookup_vote_ids( vector<vote_id_type>(account->options.votes.begin(),account->options.votes.end()) );

      // Add the account itself, its statistics object, cashback balance, and referral account names
      /*
      full_account("account", *account)("statistics", account->statistics(_db))
            ("registrar_name", account->registrar(_db).name)("referrer_name", account->referrer(_db).name)
            ("lifetime_referrer_name", account->lifetime_referrer(_db).name);
            */
      if (account->cashback_vb)
      {
         acnt.cashback_balance = account->cashback_balance(_db);
      }
      // Add the account's proposals
      const auto& proposal_idx = _db.get_index_type<proposal_index>();
      const auto& pidx = dynamic_cast<const primary_index<proposal_index>&>(proposal_idx);
      const auto& proposals_by_account = pidx.get_secondary_index<graphene::chain::required_approval_index>();
      auto  required_approvals_itr = proposals_by_account._account_to_proposals.find( account->id );
      if( required_approvals_itr != proposals_by_account._account_to_proposals.end() )
      {
         acnt.proposals.reserve( required_approvals_itr->second.size() );
         for( auto proposal_id : required_approvals_itr->second )
            acnt.proposals.push_back( proposal_id(_db) );
      }


      // Add the account's balances
      auto balance_range = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->id));
      //vector<account_balance_object> balances;
      std::for_each(balance_range.first, balance_range.second,
                    [&acnt](const account_balance_object& balance) {
                       acnt.balances.emplace_back(balance);
                    });

      // Add the account's vesting balances
      auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(vesting_range.first, vesting_range.second,
                    [&acnt](const vesting_balance_object& balance) {
                       acnt.vesting_balances.emplace_back(balance);
                    });

      // Add the account's orders
      auto order_range = _db.get_index_type<limit_order_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(order_range.first, order_range.second,
                    [&acnt] (const limit_order_object& order) {
                       acnt.limit_orders.emplace_back(order);
                    });
      auto call_range = _db.get_index_type<call_order_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(call_range.first, call_range.second,
                    [&acnt] (const call_order_object& call) {
                       acnt.call_orders.emplace_back(call);
                    });
      auto settle_range = _db.get_index_type<force_settlement_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(settle_range.first, settle_range.second,
                    [&acnt] (const force_settlement_object& settle) {
                       acnt.settle_orders.emplace_back(settle);
                    });

      // get assets issued by user
      auto asset_range = _db.get_index_type<asset_index>().indices().get<by_issuer>().equal_range(account->id);
      std::for_each(asset_range.first, asset_range.second,
                    [&acnt] (const asset_object& asset) {
                       acnt.assets.emplace_back(asset.id);
                    });

      // get withdraws permissions
      auto withdraw_range = _db.get_index_type<withdraw_permission_index>().indices().get<by_from>().equal_range(account->id);
      std::for_each(withdraw_range.first, withdraw_range.second,
                    [&acnt] (const withdraw_permission_object& withdraw) {
                       acnt.withdraws.emplace_back(withdraw);
                    });


      results[account_name_or_id] = acnt;
   }
   return results;
}

optional<account_object> database_api::get_account_by_name(const string& name )const
{
   return my->get_account_by_name( name );
}

optional<account_object> database_api_impl::get_account_by_name( string name )const
{
   const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = idx.find(name);
   if (itr != idx.end())
      return *itr;
   return optional<account_object>();
}

vector<account_id_type> database_api::get_account_references( account_id_type account_id )const
{
   return my->get_account_references( account_id );
}

vector<account_id_type> database_api_impl::get_account_references( account_id_type account_id )const
{
   const auto& idx = _db.get_index_type<account_index>();
   const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
   auto itr = refs.account_to_account_memberships.find(account_id);
   vector<account_id_type> result;

   if( itr != refs.account_to_account_memberships.end() )
   {
      result.reserve( itr->second.size() );
      for( auto item : itr->second ) result.push_back(item);
   }
   return result;
}

vector<optional<account_object>> database_api::lookup_account_names(const vector<string>& account_names)const
{
   return my->lookup_account_names( account_names );
}

vector <multisig_asset_transfer_object> database_api::get_multisigs_trx() const
{
	return {  };
}

optional<multisig_asset_transfer_object> database_api::lookup_multisig_asset(multisig_asset_transfer_id_type id)const
{
	return  my->lookup_multisig_asset(id);
}


vector<optional<account_object>> database_api_impl::lookup_account_names(const vector<string>& account_names)const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   vector<optional<account_object> > result;
   result.reserve(account_names.size());
   std::transform(account_names.begin(), account_names.end(), std::back_inserter(result),
                  [&accounts_by_name](const string& name) -> optional<account_object> {
      auto itr = accounts_by_name.find(name);
      return itr == accounts_by_name.end()? optional<account_object>() : *itr;
   });
   return result;
}

vector<multisig_asset_transfer_object> database_api_impl::get_multisigs_trx() const
{

	const auto& multisigs_trx = _db.get_index_type<crosschain_transfer_index>().indices().get<by_status>().\
		                   equal_range(multisig_asset_transfer_object::waiting_signtures);
	vector<multisig_asset_transfer_object> result;
	for (auto iter : boost::make_iterator_range(multisigs_trx.first,multisigs_trx.second))
	{
		result.emplace_back(iter);
	}
	return result;
}

optional<multisig_asset_transfer_object> database_api_impl::lookup_multisig_asset(multisig_asset_transfer_id_type id) const
{
	if (auto iter = _db.find(id))
	{
		return *iter;
	}
	return optional<multisig_asset_transfer_object>();

}

map<string,account_id_type> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_accounts( lower_bound_name, limit );
}

map<string,account_id_type> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   map<string,account_id_type> result;

   for( auto itr = accounts_by_name.lower_bound(lower_bound_name);
        limit-- && itr != accounts_by_name.end();
        ++itr )
   {
      result.insert(make_pair(itr->name, itr->get_id()));
      if( limit == 1 )
         subscribe_to_item( itr->get_id() );
   }

   return result;
}

uint64_t database_api::get_account_count()const
{
   return my->get_account_count();
}

uint64_t database_api_impl::get_account_count()const
{
   return _db.get_index_type<account_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Balances                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<asset> database_api::get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const
{
   return my->get_account_balances( id, assets );
}
vector<asset> database_api::list_account_balances(const string& id) const
{
    if (auto real_id = maybe_id<account_id_type>(id))
        return get_account_balances(*real_id, flat_set<asset_id_type>());
    return get_account_balances(get_account(id).id, flat_set<asset_id_type>());
}
optional<whiteOperationList_object> database_api_impl::get_whiteOperation(const address& addr) const {
	optional<whiteOperationList_object> result;
	const auto& whiteOpIdx = _db.get_index_type<whiteOperation_index>().indices().get<by_address>();
	auto iter = whiteOpIdx.find(addr);
	if (iter != whiteOpIdx.end())
		result = *iter;
	return result;
}

vector<asset> database_api_impl::get_account_balances(account_id_type acnt, const flat_set<asset_id_type>& assets)const
{
   vector<asset> result;
   if (assets.empty())
   {
      // if the caller passes in an empty list of assets, return balances for all assets the account owns
      const account_balance_index& balance_index = _db.get_index_type<account_balance_index>();
      auto range = balance_index.indices().get<by_account_asset>().equal_range(boost::make_tuple(acnt));
      for (const account_balance_object& balance : boost::make_iterator_range(range.first, range.second))
         result.push_back(asset(balance.get_balance()));
   }
   else
   {
      result.reserve(assets.size());

      std::transform(assets.begin(), assets.end(), std::back_inserter(result),
                     [this, acnt](asset_id_type id) { return _db.get_balance(acnt, id); });
   }

   return result;
}

vector<asset> database_api::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const
{
   return my->get_named_account_balances( name, assets );
}
vector<asset> database_api::get_account_balances_by_str(const string& account)const
{
    auto acc = get_account(account);
    auto add = acc.addr;
    FC_ASSERT(address() != acc.addr, "account is not in the chain.");
    vector<address> vec;
    vec.push_back(add);
    auto vec_balance = get_balance_objects(vec);
    vector<asset> ret;
    for (auto balance : vec_balance)
    {
        ret.push_back(balance.balance);
    }

    return ret;
}
vector<asset> database_api_impl::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets) const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = accounts_by_name.find(name);
   FC_ASSERT( itr != accounts_by_name.end() );
   return get_account_balances(itr->get_id(), assets);
}

vector<balance_object> database_api::get_balance_objects( const vector<address>& addrs )const
{
   return my->get_balance_objects( addrs );
}
vector<asset> database_api::get_addr_balances(const string& addr) const
{
    if (address::is_valid(addr) == false)
    {
        return vector<asset>();
    }
    auto add = address(addr);
    vector<address> vec;
    vec.push_back(add);
    auto vec_balance = get_balance_objects(vec);
    vector<asset> ret;
    for (auto balance : vec_balance)
    {
        ret.push_back(balance.balance);
    }

    return ret;

}
vector<balance_object> database_api_impl::get_balance_objects( const vector<address>& addrs )const
{
   try
   {
      const auto& bal_idx = _db.get_index_type<balance_index>();
      const auto& by_owner_idx = bal_idx.indices().get<by_owner>();

      vector<balance_object> result;

      for( const auto& owner : addrs )
      {
         subscribe_to_item( owner );
         auto itr = by_owner_idx.lower_bound( boost::make_tuple( owner, asset_id_type(0) ) );
         while( itr != by_owner_idx.end() && itr->owner == owner )
         {
            result.push_back( *itr );
            ++itr;
         }
      }
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (addrs) )
}

vector<asset> database_api::get_vested_balances( const vector<balance_id_type>& objs )const
{
   return my->get_vested_balances( objs );
}

vector<asset> database_api_impl::get_vested_balances( const vector<balance_id_type>& objs )const
{
   try
   {
      vector<asset> result;
      result.reserve( objs.size() );
      auto now = _db.head_block_time();
      for( auto obj : objs )
         result.push_back( obj(_db).available( now ) );
      return result;
   } FC_CAPTURE_AND_RETHROW( (objs) )
}

vector<vesting_balance_object> database_api::get_vesting_balances( account_id_type account_id )const
{
   return my->get_vesting_balances( account_id );
}

vector< vesting_balance_object_with_info > database_api::get_vesting_balances_with_info(const string& account_name)const
{
    try {
        fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>(account_name);
        std::vector<vesting_balance_object_with_info> result;
        fc::time_point_sec now = get_dynamic_global_properties().time;
        
        if (vbid)
        {
            auto obj=get_objects({ *vbid }).front().as<vesting_balance_object>();
            result.emplace_back(obj,now);
            return result;
        }
      
        // try casting to avoid a round-trip if we were given an account ID
        fc::optional<account_id_type> acct_id = maybe_id<account_id_type>(account_name);
        if (!acct_id)
            acct_id = get_account(account_name).id;
      
        vector< vesting_balance_object > vbos = get_vesting_balances(*acct_id);
        if (vbos.size() == 0)
            return result;
      
        for (const vesting_balance_object& vbo : vbos)
            result.emplace_back(vbo, now);
      
        return result;
    } FC_CAPTURE_AND_RETHROW((account_name))
}
vector<vesting_balance_object> database_api_impl::get_vesting_balances( account_id_type account_id )const
{
   try
   {
      vector<vesting_balance_object> result;
      auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account_id);
      std::for_each(vesting_range.first, vesting_range.second,
                    [&result](const vesting_balance_object& balance) {
                       result.emplace_back(balance);
                    });
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (account_id) );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Assets                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<asset_object>> database_api::get_assets(const vector<asset_id_type>& asset_ids)const
{
   return my->get_assets( asset_ids );
}

vector<optional<asset_object>> database_api_impl::get_assets(const vector<asset_id_type>& asset_ids)const
{
   vector<optional<asset_object>> result; result.reserve(asset_ids.size());
   std::transform(asset_ids.begin(), asset_ids.end(), std::back_inserter(result),
                  [this](asset_id_type id) -> optional<asset_object> {
      if(auto o = _db.find(id))
      {
         subscribe_to_item( id );
         return *o;
      }
      return {};
   });
   return result;
}

vector<asset_object> database_api::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   return my->list_assets( lower_bound_symbol, limit );
}

vector<asset_object> database_api_impl::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   FC_ASSERT( limit <= 100 );
   const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
   vector<asset_object> result;
   result.reserve(limit);

   auto itr = assets_by_symbol.lower_bound(lower_bound_symbol);

   if( lower_bound_symbol == "" )
      itr = assets_by_symbol.begin();

   while(limit-- && itr != assets_by_symbol.end())
      result.emplace_back(*itr++);

   return result;
}

vector<optional<asset_object>> database_api::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   return my->lookup_asset_symbols( symbols_or_ids );
}

optional<asset_dynamic_data_object> database_api::get_asset_dynamic_data(const string& symbols_or_ids) const
{
	vector<string> symbol;
	symbol.push_back(symbols_or_ids);
	auto vec = lookup_asset_symbols(symbol);
	if (vec.size() == 0)
		return optional<asset_dynamic_data_object>();
	auto obj = vec.front();
	return obj->dynamic_data(my->_db);
}

vector<optional<asset_object>> database_api_impl::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
   vector<optional<asset_object> > result;
   result.reserve(symbols_or_ids.size());
   std::transform(symbols_or_ids.begin(), symbols_or_ids.end(), std::back_inserter(result),
                  [this, &assets_by_symbol](const string& symbol_or_id) -> optional<asset_object> {
      if( !symbol_or_id.empty() && std::isdigit(symbol_or_id[0]) )
      {
         auto ptr = _db.find(variant(symbol_or_id).as<asset_id_type>());
         return ptr == nullptr? optional<asset_object>() : *ptr;
      }
      auto itr = assets_by_symbol.find(symbol_or_id);
      return itr == assets_by_symbol.end()? optional<asset_object>() : *itr;
   });
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Markets / feeds                                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<limit_order_object> database_api::get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const
{
   return my->get_limit_orders( a, b, limit );
}

/**
 *  @return the limit orders for both sides of the book for the two assets specified up to limit number on each side.
 */
vector<limit_order_object> database_api_impl::get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const
{
   const auto& limit_order_idx = _db.get_index_type<limit_order_index>();
   const auto& limit_price_idx = limit_order_idx.indices().get<by_price>();

   vector<limit_order_object> result;

   uint32_t count = 0;
   auto limit_itr = limit_price_idx.lower_bound(price::max(a,b));
   auto limit_end = limit_price_idx.upper_bound(price::min(a,b));
   while(limit_itr != limit_end && count < limit)
   {
      result.push_back(*limit_itr);
      ++limit_itr;
      ++count;
   }
   count = 0;
   limit_itr = limit_price_idx.lower_bound(price::max(b,a));
   limit_end = limit_price_idx.upper_bound(price::min(b,a));
   while(limit_itr != limit_end && count < limit)
   {
      result.push_back(*limit_itr);
      ++limit_itr;
      ++count;
   }

   return result;
}

vector<call_order_object> database_api::get_call_orders(asset_id_type a, uint32_t limit)const
{
   return my->get_call_orders( a, limit );
}

vector<call_order_object> database_api_impl::get_call_orders(asset_id_type a, uint32_t limit)const
{
   const auto& call_index = _db.get_index_type<call_order_index>().indices().get<by_price>();
   const asset_object& mia = _db.get(a);
   price index_price = price::min(mia.bitasset_data(_db).options.short_backing_asset, mia.get_id());

   return vector<call_order_object>(call_index.lower_bound(index_price.min()),
                                    call_index.lower_bound(index_price.max()));
}

vector<force_settlement_object> database_api::get_settle_orders(asset_id_type a, uint32_t limit)const
{
   return my->get_settle_orders( a, limit );
}

vector<force_settlement_object> database_api_impl::get_settle_orders(asset_id_type a, uint32_t limit)const
{
   const auto& settle_index = _db.get_index_type<force_settlement_index>().indices().get<by_expiration>();
   const asset_object& mia = _db.get(a);
   return vector<force_settlement_object>(settle_index.lower_bound(mia.get_id()),
                                          settle_index.upper_bound(mia.get_id()));
}

vector<call_order_object> database_api::get_margin_positions( const account_id_type& id )const
{
   return my->get_margin_positions( id );
}

vector<call_order_object> database_api_impl::get_margin_positions( const account_id_type& id )const
{
   try
   {
      const auto& idx = _db.get_index_type<call_order_index>();
      const auto& aidx = idx.indices().get<by_account>();
      auto start = aidx.lower_bound( boost::make_tuple( id, asset_id_type(0) ) );
      auto end = aidx.lower_bound( boost::make_tuple( id+1, asset_id_type(0) ) );
      vector<call_order_object> result;
      while( start != end )
      {
         result.push_back(*start);
         ++start;
      }
      return result;
   } FC_CAPTURE_AND_RETHROW( (id) )
}

void database_api::subscribe_to_market(std::function<void(const variant&)> callback, asset_id_type a, asset_id_type b)
{
   my->subscribe_to_market( callback, a, b );
}

void database_api_impl::subscribe_to_market(std::function<void(const variant&)> callback, asset_id_type a, asset_id_type b)
{
   if(a > b) std::swap(a,b);
   FC_ASSERT(a != b);
   _market_subscriptions[ std::make_pair(a,b) ] = callback;
}

void database_api::unsubscribe_from_market(asset_id_type a, asset_id_type b)
{
   my->unsubscribe_from_market( a, b );
}

void database_api_impl::unsubscribe_from_market(asset_id_type a, asset_id_type b)
{
   if(a > b) std::swap(a,b);
   FC_ASSERT(a != b);
   _market_subscriptions.erase(std::make_pair(a,b));
}

market_ticker database_api::get_ticker( const string& base, const string& quote )const
{
    return my->get_ticker( base, quote );
}

market_ticker database_api_impl::get_ticker( const string& base, const string& quote )const
{
    const auto assets = lookup_asset_symbols( {base, quote} );
    FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
    FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

    market_ticker result;
    result.base = base;
    result.quote = quote;
    result.latest = 0;
    result.lowest_ask = 0;
    result.highest_bid = 0;
    result.percent_change = 0;
    result.base_volume = 0;
    result.quote_volume = 0;

    try {
        const fc::time_point_sec now = fc::time_point::now();
        const fc::time_point_sec yesterday = fc::time_point_sec( now.sec_since_epoch() - 86400 );
        const auto batch_size = 100;

        vector<market_trade> trades = get_trade_history( base, quote, now, yesterday, batch_size );
        if( !trades.empty() )
        {
            result.latest = trades[0].price;

            while( !trades.empty() )
            {
                for( const market_trade& t: trades )
                {
                    result.base_volume += t.value;
                    result.quote_volume += t.amount;
                }

                trades = get_trade_history( base, quote, trades.back().date, yesterday, batch_size );
            }

            const auto last_trade_yesterday = get_trade_history( base, quote, yesterday, fc::time_point_sec(), 1 );
            if( !last_trade_yesterday.empty() )
            {
                const auto price_yesterday = last_trade_yesterday[0].price;
                result.percent_change = ( (result.latest / price_yesterday) - 1 ) * 100;
            }
        }
        else
        {
            const auto last_trade = get_trade_history( base, quote, now, fc::time_point_sec(), 1 );
            if( !last_trade.empty() )
                result.latest = last_trade[0].price;
        }

        const auto orders = get_order_book( base, quote, 1 );
        if( !orders.asks.empty() ) result.lowest_ask = orders.asks[0].price;
        if( !orders.bids.empty() ) result.highest_bid = orders.bids[0].price;
    } FC_CAPTURE_AND_RETHROW( (base)(quote) )

    return result;
}

market_volume database_api::get_24_volume( const string& base, const string& quote )const
{
    return my->get_24_volume( base, quote );
}

market_volume database_api_impl::get_24_volume( const string& base, const string& quote )const
{
    const auto ticker = get_ticker( base, quote );

    market_volume result;
    result.base = ticker.base;
    result.quote = ticker.quote;
    result.base_volume = ticker.base_volume;
    result.quote_volume = ticker.quote_volume;

    return result;
}

order_book database_api::get_order_book( const string& base, const string& quote, unsigned limit )const
{
   return my->get_order_book( base, quote, limit);
}

order_book database_api_impl::get_order_book( const string& base, const string& quote, unsigned limit )const
{
   using boost::multiprecision::uint128_t;
   FC_ASSERT( limit <= 50 );

   order_book result;
   result.base = base;
   result.quote = quote;

   auto assets = lookup_asset_symbols( {base, quote} );
   FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
   FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

   auto base_id = assets[0]->id;
   auto quote_id = assets[1]->id;
   auto orders = get_limit_orders( base_id, quote_id, limit );


   auto asset_to_real = [&]( const asset& a, int p ) { return double(a.amount.value)/pow( 10, p ); };
   auto price_to_real = [&]( const price& p )
   {
      if( p.base.asset_id == base_id )
         return asset_to_real( p.base, assets[0]->precision ) / asset_to_real( p.quote, assets[1]->precision );
      else
         return asset_to_real( p.quote, assets[0]->precision ) / asset_to_real( p.base, assets[1]->precision );
   };

   for( const auto& o : orders )
   {
      if( o.sell_price.base.asset_id == base_id )
      {
         order ord;
         ord.price = price_to_real( o.sell_price );
         ord.quote = asset_to_real( share_type( ( uint128_t( o.for_sale.value ) * o.sell_price.quote.amount.value ) / o.sell_price.base.amount.value ), assets[1]->precision );
         ord.base = asset_to_real( o.for_sale, assets[0]->precision );
         result.bids.push_back( ord );
      }
      else
      {
         order ord;
         ord.price = price_to_real( o.sell_price );
         ord.quote = asset_to_real( o.for_sale, assets[1]->precision );
         ord.base = asset_to_real( share_type( ( uint128_t( o.for_sale.value ) * o.sell_price.quote.amount.value ) / o.sell_price.base.amount.value ), assets[0]->precision );
         result.asks.push_back( ord );
      }
   }

   return result;
}

vector<market_trade> database_api::get_trade_history( const string& base,
                                                      const string& quote,
                                                      fc::time_point_sec start,
                                                      fc::time_point_sec stop,
                                                      unsigned limit )const
{
   return my->get_trade_history( base, quote, start, stop, limit );
}

vector<market_trade> database_api_impl::get_trade_history( const string& base,
                                                           const string& quote,
                                                           fc::time_point_sec start,
                                                           fc::time_point_sec stop,
                                                           unsigned limit )const
{
   FC_ASSERT( limit <= 100 );

   auto assets = lookup_asset_symbols( {base, quote} );
   FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
   FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

   auto base_id = assets[0]->id;
   auto quote_id = assets[1]->id;

   if( base_id > quote_id ) std::swap( base_id, quote_id );
   const auto& history_idx = _db.get_index_type<graphene::market_history::history_index>().indices().get<by_key>();
   history_key hkey;
   hkey.base = base_id;
   hkey.quote = quote_id;
   hkey.sequence = std::numeric_limits<int64_t>::min();

   auto price_to_real = [&]( const share_type a, int p ) { return double( a.value ) / pow( 10, p ); };

   if ( start.sec_since_epoch() == 0 )
      start = fc::time_point_sec( fc::time_point::now() );

   uint32_t count = 0;
   auto itr = history_idx.lower_bound( hkey );
   vector<market_trade> result;

   while( itr != history_idx.end() && count < limit && !( itr->key.base != base_id || itr->key.quote != quote_id || itr->time < stop ) )
   {
      if( itr->time < start )
      {
         market_trade trade;

         if( assets[0]->id == itr->op.receives.asset_id )
         {
            trade.amount = price_to_real( itr->op.pays.amount, assets[1]->precision );
            trade.value = price_to_real( itr->op.receives.amount, assets[0]->precision );
         }
         else
         {
            trade.amount = price_to_real( itr->op.receives.amount, assets[1]->precision );
            trade.value = price_to_real( itr->op.pays.amount, assets[0]->precision );
         }

         trade.date = itr->time;
         trade.price = trade.value / trade.amount;

         result.push_back( trade );
         ++count;
      }

      // Trades are tracked in each direction.
      ++itr;
      ++itr;
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<miner_object>> database_api::get_miners(const vector<miner_id_type>& witness_ids)const
{
   return my->get_miners( witness_ids );
}

vector<worker_object> database_api::get_workers_by_account(account_id_type account)const
{
    const auto& idx = my->_db.get_index_type<worker_index>().indices().get<by_account>();
    auto itr = idx.find(account);
    vector<worker_object> result;

    if( itr != idx.end() && itr->worker_account == account )
    {
       result.emplace_back( *itr );
       ++itr;
    }

    return result;
}


vector<optional<miner_object>> database_api_impl::get_miners(const vector<miner_id_type>& witness_ids)const
{
   vector<optional<miner_object>> result; result.reserve(witness_ids.size());
   std::transform(witness_ids.begin(), witness_ids.end(), std::back_inserter(result),
                  [this](miner_id_type id) -> optional<miner_object> {
      if(auto o = _db.find(id))
         return *o;
      return {};
   });
   return result;
}

fc::optional<miner_object> database_api::get_miner_by_account(account_id_type account)const
{
   return my->get_miner_by_account( account );
}

miner_object database_api::get_miner(const string& owner_account) const
{
    try
    {
        fc::optional<miner_id_type> miner_id = maybe_id<miner_id_type>(owner_account);
        if (miner_id)
        {
            std::vector<miner_id_type> ids_to_get;
            ids_to_get.push_back(*miner_id);
            std::vector<fc::optional<miner_object>> miner_objects = get_miners(ids_to_get);
            if (miner_objects.front())
                return *miner_objects.front();
            FC_THROW("No witness is registered for id ${id}", ("id", owner_account));
        }
        else
        {
            // then maybe it's the owner account
            try
            {
                account_id_type owner_account_id = get_account_id(owner_account);
                fc::optional<miner_object> witness = get_miner_by_account(owner_account_id);
                if (witness)
                    return *witness;
                else
                    FC_THROW("No witness is registered for account ${account}", ("account", owner_account));
            }
            catch (const fc::exception&)
            {
                FC_THROW("No account or witness named ${account}", ("account", owner_account));
            }
        }
    }
    FC_CAPTURE_AND_RETHROW((owner_account))
}
fc::optional<miner_object> database_api_impl::get_miner_by_account(account_id_type account) const
{
   const auto& idx = _db.get_index_type<miner_index>().indices().get<by_account>();
   auto itr = idx.find(account);
   if( itr != idx.end() )
      return *itr;
   return {};
}

map<string, miner_id_type> database_api::lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_miner_accounts( lower_bound_name, limit );
}

map<string, miner_id_type> database_api_impl::lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& witnesses_by_id = _db.get_index_type<miner_index>().indices().get<by_id>();

   // we want to order witnesses by account name, but that name is in the account object
   // so the miner_index doesn't have a quick way to access it.
   // get all the names and look them all up, sort them, then figure out what
   // records to return.  This could be optimized, but we expect the
   // number of witnesses to be few and the frequency of calls to be rare
   std::map<std::string, miner_id_type> miners_by_account_name;
   for (const miner_object& miner : witnesses_by_id)
       if (auto account_iter = _db.find(miner.miner_account))
           if (account_iter->name >= lower_bound_name) // we can ignore anything below lower_bound_name
               miners_by_account_name.insert(std::make_pair(account_iter->name, miner.id));

   auto end_iter = miners_by_account_name.begin();
   while (end_iter != miners_by_account_name.end() && limit--)
       ++end_iter;
   miners_by_account_name.erase(end_iter, miners_by_account_name.end());
   return miners_by_account_name;
}

vector<miner_id_type> database_api::list_scheduled_citizens() const 
{
	return my->list_scheduled_citizens();
}

vector<miner_id_type> database_api_impl::list_scheduled_citizens() const
{
	const witness_schedule_object& wso = witness_schedule_id_type()(_db);
	return wso.current_shuffled_miners;

}

fc::uint128_t database_api_impl::get_pledge()const
{
	fc::uint128_t result = 0;
	const auto& citizen_ids = _db.get_index_type<miner_index>().indices().get<by_id>();
	for (const auto& id : citizen_ids)
	{
		result += id.pledge_weight;
	}
	return result;
}

uint64_t database_api::get_miner_count()const
{
   return my->get_miner_count();
}

uint64_t database_api_impl::get_miner_count()const
{
   return _db.get_index_type<miner_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Committee members                                                //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<guard_member_object>> database_api::get_guard_members(const vector<guard_member_id_type>& committee_member_ids)const
{
   return my->get_guard_members( committee_member_ids );
}
map<string, guard_member_id_type> database_api::list_guard_members(const string& lowerbound, uint32_t limit)
{
    return lookup_guard_member_accounts(lowerbound, limit, false);
}
map<string, guard_member_id_type> database_api::list_all_guards(const string& lowerbound, uint32_t limit)const
{
    return lookup_guard_member_accounts(lowerbound, limit, true);
}
guard_member_object database_api::get_guard_member(const string& owner_account)const
{
    try
    {
        fc::optional<guard_member_id_type> committee_member_id = maybe_id<guard_member_id_type>(owner_account);
        if (committee_member_id)
        {
            std::vector<guard_member_id_type> ids_to_get;
            ids_to_get.push_back(*committee_member_id);
            std::vector<fc::optional<guard_member_object>> guard_member_objects = get_guard_members(ids_to_get);
            if (guard_member_objects.front())
                return *guard_member_objects.front();
            FC_THROW("No committee_member is registered for id ${id}", ("id", owner_account));
        }
        else
        {
            // then maybe it's the owner account
            try
            {
                account_id_type owner_account_id = get_account_id(owner_account);
                fc::optional<guard_member_object> committee_member = get_guard_member_by_account(owner_account_id);
                if (committee_member)
                    return *committee_member;
                else
                    FC_THROW("No committee_member is registered for account ${account}", ("account", owner_account));
            }
            catch (const fc::exception&)
            {
                FC_THROW("No account or committee_member named ${account}", ("account", owner_account));
            }
        }
    }
    FC_CAPTURE_AND_RETHROW((owner_account))
}
vector<optional<guard_member_object>> database_api_impl::get_guard_members(const vector<guard_member_id_type>& committee_member_ids)const
{
   vector<optional<guard_member_object>> result; result.reserve(committee_member_ids.size());
   std::transform(committee_member_ids.begin(), committee_member_ids.end(), std::back_inserter(result),
                  [this](guard_member_id_type id) -> optional<guard_member_object> {
      if(auto o = _db.find(id))
         return *o;
      return {};
   });
   return result;
}

fc::optional<guard_member_object> database_api::get_guard_member_by_account(account_id_type account)const
{
   return my->get_guard_member_by_account( account );
}

fc::optional<guard_member_object> database_api_impl::get_guard_member_by_account(account_id_type account) const
{
   const auto& idx = _db.get_index_type<guard_member_index>().indices().get<by_account>();
   auto itr = idx.find(account);
   if( itr != idx.end() )
      return *itr;
   return {};
}
vector<optional<guarantee_object>> database_api_impl::get_guarantee_orders(const address& addr, bool all) const
{
	vector<optional<guarantee_object>> result;
	const auto& idx = _db.get_index_type<guarantee_index>().indices().get<by_owner>();
	const auto& range = idx.equal_range(addr);
	std::for_each(range.first, range.second, [&](const guarantee_object& obj) {
		if (all == true)
		{
			result.emplace_back(obj);
		}
		else
		{
			if (obj.finished != true)
				result.emplace_back(obj);
		}
	});
	return result;
}
vector<optional<guarantee_object>> database_api_impl::list_guarantee_object(const string& chain_type,bool all) const
{
	vector<optional<guarantee_object>> result;
	const auto& idx = _db.get_index_type<guarantee_index>().indices().get<by_symbol>();
	const auto& range = idx.equal_range(chain_type);
	std::for_each(range.first, range.second, [&](const guarantee_object& obj) {
		if (all == true)
		{
			result.emplace_back(obj);
		}
		else
		{
			if (obj.finished != true)
				result.emplace_back(obj);
		}
	});
	//sort result
	std::sort(result.begin(), result.end(), [](optional<guarantee_object> a, optional<guarantee_object> b) {
		auto price_a = price(a->asset_orign,a->asset_target);
		auto price_b = price(b->asset_orign,b->asset_target);
		return price_a < price_b;
	});
	return result;
}
optional<guarantee_object> database_api_impl::get_gurantee_object(const guarantee_object_id_type id) const
{
	auto& gurantee_idx = _db.get_index_type<guarantee_index>().indices();
	auto& gurantee_objs = gurantee_idx.get<by_id>();

	auto ret = gurantee_objs.find(id);
	if (ret != gurantee_objs.end())
	{
		return *ret;
	}
		
	return optional<guarantee_object>();
}

optional<contract_event_notify_object> database_api_impl::get_contract_event_notify_by_id(const contract_event_notify_object_id_type & id)
{
    auto& indices = _db.get_index_type<contract_event_notify_index>().indices();
    auto& idx= indices.get<by_id>();
    auto it=idx.find(id);
    if (it != idx.end())
    {
        return *it;
    }
    return optional<contract_event_notify_object>();

}

vector<address> database_api_impl::get_contract_addresses_by_owner(const address& addr)const
{
    return _db.get_contract_address_by_owner(addr);
}
vector<contract_object> database_api_impl::get_contracts_by_owner(const address& addr) const
{
    return _db.get_contract_by_owner(addr);
}
vector<contract_event_notify_object> database_api_impl::get_contract_events(const address &addr) const
{
    return _db.get_contract_events_by_contract_ordered(addr);
}
vector<contract_event_notify_object> database_api_impl::get_contract_events(const address &addr,uint64_t start,uint64_t range) const
{
	return _db.get_contract_events_by_block_and_addr_ordered(addr,start,range);
}

vector<contract_object> database_api_impl::get_contract_registered(const uint32_t start_with, const uint32_t num)const
{
    return _db.get_registered_contract_according_block(start_with, num);
}
vector<contract_blocknum_pair> database_api_impl::get_contract_storage_changed(const uint32_t block_num, const uint32_t num)const
{
    return _db.get_contract_changed(block_num, num);
}
vector<contract_blocknum_pair> database_api::get_contract_storage_changed(const uint32_t block_num) const
{
    return my->get_contract_storage_changed(block_num,0);
}
vector<fc::optional<eth_multi_account_trx_object>> database_api::get_eths_multi_create_account_trx(const eth_multi_account_trx_state trx_state, const transaction_id_type trx_id)const {
	return my->get_eths_multi_create_account_trx(trx_state, trx_id);
}
vector<fc::optional<eth_multi_account_trx_object>> database_api_impl::get_eths_multi_create_account_trx(const eth_multi_account_trx_state trx_state, const transaction_id_type trx_id) const {
	vector<fc::optional<eth_multi_account_trx_object>> ret;
	auto range = _db.get_index_type<eth_multi_account_trx_index>().indices().get<by_eth_mulacc_tx_state>().equal_range(trx_state);
	for (auto iter : boost::make_iterator_range(range.first, range.second)) {
		if (trx_id == transaction_id_type()) {
			ret.push_back(iter);
		}
		else if (iter.multi_account_create_trx_id == trx_id) {
			ret.push_back(iter);
			break;
		}
	}
	return ret;
}
vector<fc::optional<otc_contract_object>> database_api::get_otc_contract_object(string from_asset, string to_asset, uint32_t limit) {
	return my->get_otc_contract_object(from_asset, to_asset, limit);
}
bool compareDes(const fc::optional<otc_contract_object> &value1, const fc::optional<otc_contract_object> &value2)
{
	return value1->price > value2->price;
}
vector<fc::optional<otc_contract_object>> database_api_impl::get_otc_contract_object(const string from_asset,const string to_asset,const uint32_t limit)const {
	vector<fc::optional<otc_contract_object>> ret;
	auto range = _db.get_index_type<otc_contract_index_index>().indices().get<by_from_to_asset>().equal_range(boost::make_tuple(from_asset,to_asset));
	uint32_t count = 0;
	for (auto iter : boost::make_iterator_range(range.first, range.second)) {
		ret.emplace_back(iter);
		std::sort(ret.begin(), ret.end(), compareDes);
		if (count < limit)
		{
			continue;
		}
		++count;
	}
	return ret;
}
vector<contract_blocknum_pair> database_api::get_contract_registered(const uint32_t block_num) const
{
    vector<contract_blocknum_pair> res;
    auto contracts=my->get_contract_registered(block_num,0);
    for(auto co:contracts)
    {
        contract_blocknum_pair rec;
        rec.block_num = co.registered_block;
        rec.contract_address = co.contract_address.operator fc::string();
        res.push_back(rec);
    }
    return res;
}
vector<graphene::chain::contract_invoke_result_object> database_api_impl::get_contract_invoke_object(const transaction_id_type& trx_id) const
{
    try {
        return _db.get_contract_invoke_result(trx_id);
    }FC_CAPTURE_AND_RETHROW((trx_id))
}

map<string, guard_member_id_type> database_api::lookup_guard_member_accounts(const string& lower_bound_name, uint32_t limit,bool formal)const
{
   return my->lookup_guard_member_accounts( lower_bound_name, limit ,formal);
}

map<string, guard_member_id_type> database_api_impl::lookup_guard_member_accounts(const string& lower_bound_name, uint32_t limit,bool formal)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& committee_members_by_id = _db.get_index_type<guard_member_index>().indices().get<by_id>();

   // we want to order committee_members by account name, but that name is in the account object
   // so the guard_member_index doesn't have a quick way to access it.
   // get all the names and look them all up, sort them, then figure out what
   // records to return.  This could be optimized, but we expect the
   // number of committee_members to be few and the frequency of calls to be rare
   std::map<std::string, guard_member_id_type> committee_members_by_account_name;
   for (const guard_member_object& committee_member : committee_members_by_id)
   {
	   if (formal == false)
	   {
		   if (!committee_member.formal)
			   continue;
	   }
	   
	   if (auto account_iter = _db.find(committee_member.guard_member_account))
		   if (account_iter->name >= lower_bound_name) // we can ignore anything below lower_bound_name
			   committee_members_by_account_name.insert(std::make_pair(account_iter->name, committee_member.id));
   }
   auto end_iter = committee_members_by_account_name.begin();
   while (end_iter != committee_members_by_account_name.end() && limit--)
       ++end_iter;
   committee_members_by_account_name.erase(end_iter, committee_members_by_account_name.end());
   return committee_members_by_account_name;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Votes                                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<variant> database_api::lookup_vote_ids( const vector<vote_id_type>& votes )const
{
   return my->lookup_vote_ids( votes );
}

vector<variant> database_api_impl::lookup_vote_ids( const vector<vote_id_type>& votes )const
{
   FC_ASSERT( votes.size() < 1000, "Only 1000 votes can be queried at a time" );

   const auto& witness_idx = _db.get_index_type<miner_index>().indices().get<by_vote_id>();
   const auto& committee_idx = _db.get_index_type<guard_member_index>().indices().get<by_vote_id>();
   const auto& for_worker_idx = _db.get_index_type<worker_index>().indices().get<by_vote_for>();
   const auto& against_worker_idx = _db.get_index_type<worker_index>().indices().get<by_vote_against>();

   vector<variant> result;
   result.reserve( votes.size() );
   for( auto id : votes )
   {
      switch( id.type() )
      {
         case vote_id_type::committee:
         {
            auto itr = committee_idx.find( id );
			if (itr != committee_idx.end())
			{
				if(itr->formal)
					result.emplace_back(variant(*itr));
			}
               
            else
               result.emplace_back( variant() );
            break;
         }
         case vote_id_type::witness:
         {
            auto itr = witness_idx.find( id );
            if( itr != witness_idx.end() )
               result.emplace_back( variant( *itr ) );
            else
               result.emplace_back( variant() );
            break;
         }
         case vote_id_type::worker:
         {
            auto itr = for_worker_idx.find( id );
            if( itr != for_worker_idx.end() ) {
               result.emplace_back( variant( *itr ) );
            }
            else {
               auto itr = against_worker_idx.find( id );
               if( itr != against_worker_idx.end() ) {
                  result.emplace_back( variant( *itr ) );
               }
               else {
                  result.emplace_back( variant() );
               }
            }
            break;
         }
         case vote_id_type::VOTE_TYPE_COUNT: break; // supress unused enum value warnings
      }
   }
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx)const
{
    return my->get_transaction_hex(trx);
}
std::string database_api::serialize_transaction(const signed_transaction& trx)const
{
    return my->get_transaction_hex(trx);
}

transaction_id_type database_api::get_transaction_id(const signed_transaction& trx)const
{ 
    return trx.id(); 
}
std::string database_api_impl::get_transaction_hex(const signed_transaction& trx)const
{
   return fc::to_hex(fc::raw::pack(trx));
}

set<public_key_type> database_api::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   return my->get_required_signatures( trx, available_keys );
}

set<public_key_type> database_api_impl::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   wdump((trx)(available_keys));
   auto result = trx.get_required_signatures( _db.get_chain_id(),
                                       available_keys,
                                       [&]( account_id_type id ){ return &id(_db).active; },
                                       [&]( account_id_type id ){ return &id(_db).owner; },
                                       _db.get_global_properties().parameters.max_authority_depth );
   wdump((result));
   return result;
}

set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
{
   return my->get_potential_signatures( trx );
}
set<address> database_api::get_potential_address_signatures( const signed_transaction& trx )const
{
   return my->get_potential_address_signatures( trx );
}

set<public_key_type> database_api_impl::get_potential_signatures( const signed_transaction& trx )const
{
   wdump((trx));
   set<public_key_type> result;
   trx.get_required_signatures(
      _db.get_chain_id(),
      flat_set<public_key_type>(),
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).active;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).owner;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      _db.get_global_properties().parameters.max_authority_depth
   );

   wdump((result));
   return result;
}

set<address> database_api_impl::get_potential_address_signatures( const signed_transaction& trx )const
{
   set<address> result;
   trx.get_required_signatures(
      _db.get_chain_id(),
      flat_set<public_key_type>(),
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).active;
         for( const auto& k : auth.get_addresses() )
            result.insert(k);
         return &auth;
      },
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).owner;
         for( const auto& k : auth.get_addresses() )
            result.insert(k);
         return &auth;
      },
      _db.get_global_properties().parameters.max_authority_depth
   );
   return result;
}

bool database_api::verify_authority( const signed_transaction& trx )const
{
   return my->verify_authority( trx );
}

bool database_api_impl::verify_authority( const signed_transaction& trx )const
{
   trx.verify_authority( _db.get_chain_id(),
                         [&]( account_id_type id ){ return &id(_db).active; },
                         [&]( account_id_type id ){ return &id(_db).owner; },
                          _db.get_global_properties().parameters.max_authority_depth );
   return true;
}

bool database_api::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const
{
   return my->verify_account_authority( name_or_id, signers );
}

bool database_api_impl::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& keys )const
{
   FC_ASSERT( name_or_id.size() > 0);
   const account_object* account = nullptr;
   if (std::isdigit(name_or_id[0]))
      account = _db.find(fc::variant(name_or_id).as<account_id_type>());
   else
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(name_or_id);
      if (itr != idx.end())
         account = &*itr;
   }
   FC_ASSERT( account, "no such account" );


   /// reuse trx.verify_authority by creating a dummy transfer
   signed_transaction trx;
   transfer_operation op;
   op.from = account->id;
   trx.operations.emplace_back(op);

   return verify_authority( trx );
}

processed_transaction database_api::validate_transaction( const signed_transaction& trx , bool testing)const
{
   return my->validate_transaction( trx, testing);
}

processed_transaction database_api_impl::validate_transaction( const signed_transaction& trx ,bool testing)const
{
   return _db.validate_transaction(trx, testing);
}

vector< fc::variant > database_api::get_required_fees( const vector<operation>& ops, asset_id_type id )const
{
   return my->get_required_fees( ops, id );
}

/**
 * Container method for mutually recursive functions used to
 * implement get_required_fees() with potentially nested proposals.
 */
struct get_required_fees_helper
{
   get_required_fees_helper(
      const fee_schedule& _current_fee_schedule,
      const price& _core_exchange_rate,
      uint32_t _max_recursion
      )
      : current_fee_schedule(_current_fee_schedule),
        core_exchange_rate(_core_exchange_rate),
        max_recursion(_max_recursion)
   {}

   fc::variant set_op_fees( operation& op )
   {
      if( op.which() == operation::tag<proposal_create_operation>::value )
      {
         return set_proposal_create_op_fees( op );
      }
      else
      {
         asset fee = current_fee_schedule.set_fee( op, core_exchange_rate );
         fc::variant result;
         fc::to_variant( fee, result );
         return result;
      }
   }

   fc::variant set_proposal_create_op_fees( operation& proposal_create_op )
   {
      proposal_create_operation& op = proposal_create_op.get<proposal_create_operation>();
      std::pair< asset, fc::variants > result;
      for( op_wrapper& prop_op : op.proposed_ops )
      {
         FC_ASSERT( current_recursion < max_recursion );
         ++current_recursion;
         result.second.push_back( set_op_fees( prop_op.op ) );
         --current_recursion;
      }
      // we need to do this on the boxed version, which is why we use
      // two mutually recursive functions instead of a visitor
      result.first = current_fee_schedule.set_fee( proposal_create_op, core_exchange_rate );
      fc::variant vresult;
      fc::to_variant( result, vresult );
      return vresult;
   }

   const fee_schedule& current_fee_schedule;
   const price& core_exchange_rate;
   uint32_t max_recursion;
   uint32_t current_recursion = 0;
};

vector< fc::variant > database_api_impl::get_required_fees( const vector<operation>& ops, asset_id_type id )const
{
   vector< operation > _ops = ops;
   //
   // we copy the ops because we need to mutate an operation to reliably
   // determine its fee, see #435
   //

   vector< fc::variant > result;
   result.reserve(ops.size());
   const asset_object& a = id(_db);
   get_required_fees_helper helper(
      _db.current_fee_schedule(),
      a.options.core_exchange_rate,
      GET_REQUIRED_FEES_MAX_RECURSION );
   for( operation& op : _ops )
   {
      result.push_back( helper.set_op_fees( op ) );
   }
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Proposed transactions                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<proposal_object> database_api::get_proposed_transactions( account_id_type id )const
{
   return my->get_proposed_transactions( id );
}

vector<proposal_object> database_api::get_proposer_transactions(account_id_type id)const
{
	return my->get_proposer_transactions(id);
}

vector<proposal_object> database_api::get_voter_transactions_waiting(address addr)const
{
	return my->get_voter_transactions_waiting(addr);
}
vector<referendum_object> database_api::get_referendum_transactions_waiting(address add) const {
	return my->get_referendum_transactions_waiting(add);
}
vector<proposal_object>  database_api::get_proposal(const string& proposer)const 
{
    auto acc = get_account(proposer);
    FC_ASSERT(acc.get_id() != account_object().get_id(), "the propser doesnt exist in the chain.");

    return get_proposer_transactions(acc.get_id());
}
optional<referendum_object> database_api::get_referendum_object(const referendum_id_type& id)const
{
	return my->get_referendum_object(id);
}

vector<proposal_object>  database_api::get_proposal_for_voter(const string& voter) const
{
    auto acc = get_account(voter);
    FC_ASSERT(acc.get_id() != account_object().get_id(), "the propser doesnt exist in the chain.");

    return get_voter_transactions_waiting(acc.addr);
}
vector<proposal_object> database_api_impl::get_voter_transactions_waiting(address addr)const
{
	const auto& idx = _db.get_index_type<proposal_index>();
	vector<proposal_object> result;

	idx.inspect_all_objects([&](const object& obj) {
		const proposal_object& p = static_cast<const proposal_object&>(obj);
		auto accounts = p.required_account_approvals;
		if (accounts.find(addr) != accounts.end())
			result.push_back(p);
	});
	return result;
}
vector<referendum_object> database_api_impl::get_referendum_transactions_waiting(address addr) const 
{
	const auto& idx = _db.get_index_type<referendum_index>();
	vector<referendum_object> result;

	idx.inspect_all_objects([&](const object& obj) {
		const referendum_object& p = static_cast<const referendum_object&>(obj);
		auto accounts = p.required_account_approvals;
		if (accounts.find(addr) != accounts.end())
			result.push_back(p);
	});
	return result;
}

optional<referendum_object> database_api_impl::get_referendum_object(const referendum_id_type& id) const
{
	optional<referendum_object> result;
	try {
		result = _db.get(id);
	}
	catch (const fc::exception& e)
	{
		return result;
	}
	return result;
}

/** TODO: add secondary index that will accelerate this process */
vector<proposal_object> database_api_impl::get_proposer_transactions(account_id_type id)const
{
	const auto& idx = _db.get_index_type<proposal_index>();
	vector<proposal_object> result;

	idx.inspect_all_objects([&](const object& obj) {
		const proposal_object& p = static_cast<const proposal_object&>(obj);
		if (p.proposer == id)
			result.push_back(p);
	});
	return result;
}


/** TODO: add secondary index that will accelerate this process */
vector<proposal_object> database_api_impl::get_proposed_transactions( account_id_type id )const
{
   const auto& idx = _db.get_index_type<proposal_index>();
   vector<proposal_object> result;

   idx.inspect_all_objects( [&](const object& obj){
           const proposal_object& p = static_cast<const proposal_object&>(obj);
           if( p.required_active_approvals.find( id ) != p.required_active_approvals.end() )
              result.push_back(p);
           else if ( p.required_owner_approvals.find( id ) != p.required_owner_approvals.end() )
              result.push_back(p);
           else if ( p.available_active_approvals.find( id ) != p.available_active_approvals.end() )
              result.push_back(p);
   });
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blinded balances                                                 //
//                                                                  //
//////////////////////////////////////////////////////////////////////
/*
vector<blinded_balance_object> database_api::get_blinded_balances( const flat_set<commitment_type>& commitments )const
{
   return my->get_blinded_balances( commitments );
}

vector<blinded_balance_object> database_api_impl::get_blinded_balances( const flat_set<commitment_type>& commitments )const
{
   vector<blinded_balance_object> result; result.reserve(commitments.size());
   const auto& bal_idx = _db.get_index_type<blinded_balance_index>();
   const auto& by_commitment_idx = bal_idx.indices().get<by_commitment>();
   for( const auto& c : commitments )
   {
      auto itr = by_commitment_idx.find( c );
      if( itr != by_commitment_idx.end() )
         result.push_back( *itr );
   }
   return result;
}*/
fc::uint128_t database_api::get_pledge() const {
	return my->get_pledge();
}
vector<lockbalance_object> database_api::get_account_lock_balance(const account_id_type& id)const {
	return my->get_account_lock_balance(id);
}
vector<lockbalance_object> database_api::get_asset_lock_balance(const asset_id_type& asset) const {
	return my->get_asset_lock_balance(asset);
}
vector<lockbalance_object> database_api::get_miner_lock_balance(const miner_id_type& miner) const {
	return my->get_miner_lock_balance(miner);
}

optional<whiteOperationList_object> database_api::get_whiteOperation(const address& addr) const {
	return my->get_whiteOperation(addr);
}

vector<lockbalance_object> database_api_impl::get_account_lock_balance(const account_id_type& id)const{
	const auto& lb_index = _db.get_index_type<lockbalance_index>();
	vector<lockbalance_object> result;
	lb_index.inspect_all_objects([&](const object& obj) {
		const lockbalance_object& p = static_cast<const lockbalance_object&>(obj);
		if (p.lock_balance_account == id && p.lock_asset_amount > 0) {
			result.emplace_back(p);
		}
	});
	return result;
}
vector<lockbalance_object> database_api_impl::get_asset_lock_balance(const asset_id_type& asset) const {
	const auto& lb_index = _db.get_index_type<lockbalance_index>();
	vector<lockbalance_object> result;
	lb_index.inspect_all_objects([&](const object& obj) {
		const lockbalance_object& p = static_cast<const lockbalance_object&>(obj);
		if (p.lock_asset_id == asset) {
			result.emplace_back(p);
		}
	});
	return result;
}
vector<lockbalance_object> database_api_impl::get_miner_lock_balance(const miner_id_type& miner) const{
	const auto& lb_index = _db.get_index_type<lockbalance_index>();
	vector<lockbalance_object> result;
	lb_index.inspect_all_objects([&](const object& obj) {
		const lockbalance_object& p = static_cast<const lockbalance_object&>(obj);
		if (p.lockto_miner_account == miner) {
			result.emplace_back(p);
		}
	});
	return result;
}
vector<guard_lock_balance_object> database_api::get_guard_lock_balance(const guard_member_id_type& id)const {
	return my->get_guard_lock_balance(id);
}

vector<optional<account_binding_object>> database_api::get_binding_account(const string& account, const string& symbol) const
{
    if (address::is_valid(account))
    {
        return my->get_binding_account(account, symbol);
    }
    auto acct = get_account(account);
    FC_ASSERT(acct.addr != address());
    return get_binding_account(string(acct.addr), symbol);
}

vector<optional<account_binding_object>> database_api_impl::get_binding_account(const string& account, const string& symbol) const
{
	vector<optional<account_binding_object>> result;
	const auto& binding_accounts = _db.get_index_type<account_binding_index>().indices().get<by_account_binding>();

	const auto accounts = binding_accounts.equal_range(boost::make_tuple(address(account), symbol));
	for (auto acc : boost::make_iterator_range(accounts.first, accounts.second))
	{
		result.emplace_back(acc);
	}
	return result;
}

vector<optional<multisig_address_object>> database_api::get_multisig_address_obj(const string& symbol, const account_id_type& guard) const {
	return my->get_multisig_address_obj(symbol,guard);
}

vector<optional<multisig_account_pair_object>> database_api::get_multisig_account_pair(const string& symbol) const {
	return my->get_multisig_account_pair(symbol);
}

optional<multisig_account_pair_object> database_api::get_current_multisig_account(const string& symbol) const
{
	optional<multisig_account_pair_object> result;
	auto vec = get_multisig_account_pair(symbol);
	int max = 0;
	for (auto op_m : vec)
	{
		if (!op_m.valid())
			continue;
		if (max < op_m->effective_block_num)
		{
			max = op_m->effective_block_num;
			result = op_m;
		}
	}
	return result;
}

map<account_id_type, vector<asset>> database_api::get_citizen_lockbalance_info(const miner_id_type& id) const
{
	return my->get_citizen_lockbalance_info(id);
}

optional<contract_storage_object> database_api::get_contract_storage(const address& contract_address, const string& storage_name) const
{
	return my->_db.get_contract_storage_object(contract_address, storage_name);
}

optional<multisig_account_pair_object> database_api::lookup_multisig_account_pair(const multisig_account_pair_id_type& id) const
{
	return my->lookup_multisig_account_pair(id);
}
vector<optional<guarantee_object>> database_api::list_guarantee_object(const string& chain_type,bool all) const
{
	return my->list_guarantee_object(chain_type,all);
}
vector<optional<guarantee_object>> database_api::list_guarantee_order(const string& chain_type, bool all )const
{
    try {
        auto asset_obj = get_asset(chain_type);
        return list_guarantee_object(chain_type, all);

    }FC_CAPTURE_AND_RETHROW((chain_type))
}
vector<optional<guarantee_object>> database_api::get_guarantee_orders(const address& addr, bool all) const
{
	return my->get_guarantee_orders(addr, all);
}
optional<guarantee_object> database_api::get_gurantee_object(const guarantee_object_id_type id) const
{
	return my->get_gurantee_object(id);
}
vector<contract_invoke_result_object> database_api::get_contract_invoke_object(const string& trx_id)const
{
    return my->get_contract_invoke_object(transaction_id_type(trx_id));
}
std::pair<asset, share_type> database_api::register_contract_testing(const string& pubkey_str, const string& hex)
{
	try {


		contract_register_operation contract_register_op;

		//juge if the name has been registered in the chain
		auto owner_pubkey = public_key_type(pubkey_str);

		contract_register_op.gas_price = 0;
		contract_register_op.init_cost = (GRAPHENE_CONTRACT_TESTING_GAS);
		contract_register_op.owner_addr = address(owner_pubkey);
		contract_register_op.owner_pubkey = owner_pubkey;

		auto contract_code = ContractHelper::load_contract_from_hex(hex);
		contract_register_op.contract_code = contract_code;
		contract_register_op.contract_code.code_hash = contract_register_op.contract_code.GetHash();
		contract_register_op.register_time = fc::time_point::now() + fc::seconds(1);
		contract_register_op.contract_id = contract_register_op.calculate_contract_id();
		contract_register_op.fee.amount = 0;
		contract_register_op.fee.asset_id = asset_id_type(0);

		signed_transaction tx;
		tx.operations.push_back(contract_register_op);
		auto current_fees = get_global_properties().parameters.current_fees;
		for (auto& op : tx.operations)
			current_fees->set_fee(op);
		signed_transaction signed_tx(tx);
		auto dyn_props = get_dynamic_global_properties();
		signed_tx.set_reference_block(dyn_props.head_block_id);
		signed_tx.set_expiration(dyn_props.time + fc::seconds(30));
		signed_tx.validate();
			auto trx_res = validate_transaction(signed_tx, true);

		share_type gas_count = 0;
		for (auto op_res : trx_res.operation_results)
		{
			try { gas_count += op_res.get<contract_operation_result_info>().gas_count; }
			catch (...)
			{

			}
		}

		asset res_data_fee = signed_tx.operations[0].get<contract_register_operation>().fee;

		std::pair<asset, share_type> res = make_pair(res_data_fee, gas_count);
		return res;

	}FC_CAPTURE_AND_RETHROW((pubkey_str)(hex))
}
std::pair<asset, share_type> database_api::transfer_to_contract_testing(string pubkey, string to, string amount, string asset_symbol, const string& param)
{
	transfer_contract_operation transfer_to_contract_op;
	fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
	FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
	asset transfer_asset = asset_obj->amount_from_string(amount);
	//juge if the name has been registered in the chain

	public_key_type caller_pubkey = public_key_type(pubkey);

	transfer_to_contract_op.gas_price = 0;
	transfer_to_contract_op.invoke_cost = GRAPHENE_CONTRACT_TESTING_GAS;
	transfer_to_contract_op.caller_addr = address(caller_pubkey);
	transfer_to_contract_op.caller_pubkey = caller_pubkey;
	transfer_to_contract_op.contract_id = address(to);
	transfer_to_contract_op.fee.amount = 0;
	transfer_to_contract_op.fee.asset_id = asset_id_type(0);
	transfer_to_contract_op.amount = transfer_asset;
	transfer_to_contract_op.param = param;

	signed_transaction tx;
	tx.operations.push_back(transfer_to_contract_op);
	auto current_fees = get_global_properties().parameters.current_fees;
	for (auto& op : tx.operations)
		current_fees->set_fee(op);


	auto dyn_props = get_dynamic_global_properties();
	tx.set_reference_block(dyn_props.head_block_id);
	tx.set_expiration(dyn_props.time + fc::seconds(30));
	tx.validate();
	auto trx_res = validate_transaction(tx, true);
	share_type gas_count = 0;
	for (auto op_res : trx_res.operation_results)
	{
		try { gas_count += op_res.get<contract_operation_result_info>().gas_count; }
		catch (...)
		{

		}
	}
	asset res_data_fee = tx.operations[0].get<transfer_contract_operation>().fee;
	std::pair<asset, share_type> res = make_pair(res_data_fee, gas_count);
	return res;
}
execution_result database_api::invoke_contract_testing(const string & pubkey, const string & contract_address_or_name, const string & contract_api, const string & contract_arg)
{
	try {

		public_key_type caller_pubkey = public_key_type(pubkey);
		contract_invoke_operation contract_invoke_op;

		//juge if the name has been registered in the chain



		std::string contract_address;
		contract_object cont;
		bool is_valid_address = true;
		try {
			auto temp = graphene::chain::address(contract_address_or_name);
			FC_ASSERT(temp.version == addressVersion::CONTRACT);
		}
		catch (fc::exception& e)
		{
			is_valid_address = false;
		}
		if (!is_valid_address)
		{
			cont = get_contract_object_by_name(contract_address_or_name);
			contract_address = string(cont.contract_address);
		}
		else
		{
			cont = get_contract_object(contract_address_or_name);
			contract_address = string(cont.contract_address);
		}
		contract_invoke_op.gas_price = 0;
		contract_invoke_op.invoke_cost = GRAPHENE_CONTRACT_TESTING_GAS;
		contract_invoke_op.caller_addr = caller_pubkey;
		contract_invoke_op.caller_pubkey = caller_pubkey;
		contract_invoke_op.contract_id = address(contract_address);
		contract_invoke_op.contract_api = contract_api;
		contract_invoke_op.contract_arg = contract_arg;
		contract_invoke_op.fee.amount = 0;
		contract_invoke_op.fee.asset_id = asset_id_type(0);

		signed_transaction tx;
		tx.operations.push_back(contract_invoke_op);
		auto current_fees = get_global_properties().parameters.current_fees;
		for (auto& op : tx.operations)
			current_fees->set_fee(op);

		auto dyn_props = get_dynamic_global_properties();
		tx.set_reference_block(dyn_props.head_block_id);
		tx.set_expiration(dyn_props.time + fc::seconds(30));
		tx.validate();
		signed_transaction signed_tx(tx);
		auto trx_res = validate_transaction(signed_tx, true);
		share_type gas_count = 0;
		string api_result = "";
		for (auto op_res : trx_res.operation_results)
		{
			try { gas_count += op_res.get<contract_operation_result_info>().gas_count; 
			api_result = op_res.get<contract_operation_result_info>().api_result;
			}
			catch (...)
			{

			}
		}
		asset res_data_fee = signed_tx.operations[0].get<contract_invoke_operation>().fee;	
		execution_result res;
		res.fee = res_data_fee;
		res.gas_count = gas_count;
		res.result = api_result;
		return res;;
	}FC_CAPTURE_AND_RETHROW((pubkey)(contract_address_or_name)(contract_api)(contract_arg))

}
string database_api::invoke_contract_offline(const string & caller_pubkey_str, const string & contract_address_or_name, const string & contract_api, const string & contract_arg)
{
	try {


		contract_invoke_operation contract_invoke_op;

		//juge if the name has been registered in the chain

		public_key_type caller_pubkey(caller_pubkey_str);
		contract_object cont;
		std::string contract_address;
		//try {
		 //   auto temp = graphene::chain::address(contract_address_or_name);
		 //   FC_ASSERT(temp.version == addressVersion::CONTRACT);
		//}
		//catch (fc::exception& e)
		//{
		 //   cont = _remote_db->get_contract_object_by_name(contract_address_or_name);
		 //   contract_address = string(cont.contract_address);
		//}
		bool is_valid_address = true;
		try {
			auto temp = graphene::chain::address(contract_address_or_name);
			FC_ASSERT(temp.version == addressVersion::CONTRACT);
		}
		catch (fc::exception& e)
		{
			is_valid_address = false;
		}
		if (!is_valid_address)
		{
			cont = get_contract_object_by_name(contract_address_or_name);
			contract_address = string(cont.contract_address);
		}
		else
		{
			cont = get_contract_object(contract_address_or_name);
			contract_address = string(cont.contract_address);
		}
		if(cont.type_of_contract == contract_type::native_contract) {
			// ignore check of api
		} else {
			auto& abi = cont.code.offline_abi;
			if (abi.find(contract_api) == abi.end())
				FC_CAPTURE_AND_THROW(blockchain::contract_engine::contract_api_not_found);
		}

		contract_invoke_op.gas_price = 0;
		contract_invoke_op.invoke_cost = GRAPHENE_CONTRACT_TESTING_GAS;
		contract_invoke_op.caller_addr = caller_pubkey;
		contract_invoke_op.caller_pubkey = caller_pubkey;
		contract_invoke_op.contract_id = address(contract_address);
		contract_invoke_op.contract_api = contract_api;
		contract_invoke_op.contract_arg = contract_arg;
		contract_invoke_op.fee.amount = 0;
		contract_invoke_op.fee.asset_id = asset_id_type(0);
		//contract_invoke_op.invoke_cost = GRAPHENE_CONTRACT_TESTING_GAS;
		//contract_invoke_op.guarantee_id = get_guarantee_id();
		signed_transaction tx;
		tx.operations.push_back(contract_invoke_op);
		auto current_fees = get_global_properties().parameters.current_fees;
		for (auto& op : tx.operations)
			current_fees->set_fee(op);

		auto dyn_props = get_dynamic_global_properties();
		tx.set_reference_block(dyn_props.head_block_id);
		tx.set_expiration(dyn_props.time + fc::seconds(30));
		tx.validate();
		signed_transaction signed_tx(tx);
		auto trx_res = validate_transaction(signed_tx, true);
		share_type gas_count = 0;
		string res = "some error happened, not api result get";
		for (auto op_res : trx_res.operation_results)
		{
			try {
				res = op_res.get<contract_operation_result_info>().api_result;
			}
			catch (...)
			{
				break;
			}
		}
		return res;
	}FC_CAPTURE_AND_RETHROW((caller_pubkey_str)(contract_address_or_name)(contract_api)(contract_arg))
}

graphene::chain::vector<graphene::chain::transaction_id_type> database_api::get_contract_history(const string& contract_id, uint64_t start, uint64_t end)
{
	return my->_db.get_contract_related_transactions(address(contract_id), start, end);
}

vector<contract_event_notify_object> database_api::get_contract_events(const address&addr)const
{
    return my->get_contract_events(addr);
}
vector<contract_event_notify_object> database_api::get_contract_events_in_range(const address &addr, uint64_t start, uint64_t range) const
{
	return my->get_contract_events(addr,start,range);
}
optional<contract_event_notify_object> database_api::get_contract_event_notify_by_id(const contract_event_notify_object_id_type& id)
{
    return my->get_contract_event_notify_by_id(id);
}
vector<contract_event_notify_object> database_api::get_contract_event_notify(const address& contract_id, const transaction_id_type& trx_id, const string& event_name) const
{
    return my->_db.get_contract_event_notify(contract_id,trx_id, event_name);
}
vector<guard_lock_balance_object> database_api::get_guard_asset_lock_balance(const asset_id_type& id)const {
	return my->get_guard_asset_lock_balance(id);
}
variant_object database_api::decoderawtransaction(const string& raw_trx, const string& symbol) const
{
    try {
        auto cross_mgr = graphene::privatekey_management::crosschain_management::get_instance();
        return cross_mgr.decoderawtransaction(raw_trx, symbol);

    }FC_CAPTURE_AND_RETHROW((raw_trx)(symbol))
}
share_type database_api::get_miner_pay_per_block(uint32_t block_num) const
{
	return my->_db.get_miner_pay_per_block(block_num);
}
vector<optional<crosschain_trx_object>> database_api::get_crosschain_transaction_by_blocknum(const string& symbol,
	const string& account,
	const uint32_t& start_block_num,
	const uint32_t& stop_block_num,
	const transaction_stata& crosschain_trx_state)const {
	return my->get_crosschain_transaction_by_blocknum(symbol, account, start_block_num, stop_block_num, crosschain_trx_state);
}
vector<graphene::chain::crosschain_trx_object> database_api::get_crosschain_transaction(const transaction_stata& crosschain_trx_state, const transaction_id_type& id)const{
	return my->get_crosschain_transaction(crosschain_trx_state,id);
}
vector<crosschain_trx_object>  database_api::get_account_crosschain_transaction(const string& account,const transaction_id_type& id)const {
	return my->get_account_crosschain_transaction(account,id);
}
vector<optional<multisig_address_object>> database_api::get_multi_account_guard(const string & multi_address, const string& symbol) const {
	return my->get_multi_account_guard(multi_address, symbol);
}
std::map<std::string, asset> database_api::get_pay_back_balances(const address & pay_back_owner)const {
	return my->get_pay_back_balances(pay_back_owner);
}

vector<vote_result_object> database_api::get_vote_result_objs(const vote_object_id_type& id) const {
	return my->get_vote_result_objs(id);
}
vector<vote_object> database_api::get_votes_by_addr(const address& addr)const
{
	return my->get_votes_by_addr(addr);
}


std::map<std::string, share_type> database_api::get_bonus_balances(const address & owner)const {
	return my->get_bonus_balances(owner);
}
std::map<string,asset> database_api::get_address_pay_back_balance(const address& owner_addr, std::string asset_symbol) const {
	return my->_db.get_pay_back_balacne(owner_addr,asset_symbol);
}
vector<coldhot_transfer_object> database_api::get_coldhot_transaction(const coldhot_trx_state& coldhot_tx_state, const transaction_id_type& id)const {
	return my->get_coldhot_transaction(coldhot_tx_state, id);
}
vector<optional<coldhot_transfer_object>> database_api::get_coldhot_transaction_by_blocknum(const string& symbol,
	const uint32_t& start_block_num,
	const uint32_t& stop_block_num,
	const coldhot_trx_state& crosschain_trx_state)const {
	return my->get_coldhot_transaction_by_blocknum(symbol, start_block_num, stop_block_num, crosschain_trx_state);
}
vector<optional<coldhot_transfer_object>> database_api_impl::get_coldhot_transaction_by_blocknum(const string& symbol, const uint32_t& start_block_num, const uint32_t& stop_block_num, const coldhot_trx_state& crosschain_trx_state)const {
	vector<optional<coldhot_transfer_object>> ret;
	FC_ASSERT(stop_block_num >= start_block_num, "End num must bigger than start num");
	auto start_trx_id = transaction_id_type("0000000000000000000000000000000000000000");
	auto stop_trx_id = transaction_id_type("ffffffffffffffffffffffffffffffffffffffff");
	//struct by_symbol_block_num_state;
	//struct by_block_num_state;
	if ("" == symbol) {
		const auto& coldhot_tx_db = _db.get_index_type<coldhot_transfer_index>().indices().get<by_block_num_state>();
		auto itr = coldhot_tx_db.upper_bound(boost::make_tuple(crosschain_trx_state, start_block_num,  start_trx_id));
		auto itr_stop = coldhot_tx_db.lower_bound(boost::make_tuple(crosschain_trx_state, stop_block_num, stop_trx_id));
		for (; itr != itr_stop; itr++) {
			ret.push_back(*itr);
		}
	}
	else {
		const auto& coldhot_tx_db = _db.get_index_type<coldhot_transfer_index>().indices().get<by_symbol_block_num_state>();
		auto itr = coldhot_tx_db.upper_bound(boost::make_tuple(symbol, crosschain_trx_state, start_block_num,  start_trx_id));
		auto itr_stop = coldhot_tx_db.lower_bound(boost::make_tuple(symbol, crosschain_trx_state, stop_block_num, stop_trx_id));
		for (; itr != itr_stop; itr++) {
			ret.push_back(*itr);
		}
	}
	return ret;
}
vector<coldhot_transfer_object> database_api_impl::get_coldhot_transaction(const coldhot_trx_state& coldhot_tx_state, const transaction_id_type& id)const {
	if (id == transaction_id_type()){
		auto coldhot_trx_range = _db.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().equal_range(coldhot_tx_state);
		vector<coldhot_transfer_object> result(coldhot_trx_range.first, coldhot_trx_range.second);
		return result;
	}
	else {
		const auto& coldhot_tx_db = _db.get_index_type<coldhot_transfer_index>().indices().get<by_current_trxidstate>();
		const auto coldhot_tx_iter = coldhot_tx_db.find(boost::make_tuple(id, coldhot_tx_state));
		vector<coldhot_transfer_object> result;
		result.push_back(*coldhot_tx_iter);
		return result;
	}
}
vector<crosschain_trx_object>  database_api_impl::get_account_crosschain_transaction(const string& account, const transaction_id_type& id)const {
	vector<crosschain_trx_object> result;
	if (account == "" && id != transaction_id_type()) {
		auto& cross_trx_db = _db.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
		auto cross_trx_iter = cross_trx_db.find(id);
		if (cross_trx_iter != cross_trx_db.end()){
			result.push_back(*cross_trx_iter);
		}
	}
	else {
		auto cross_trx_range = _db.get_index_type<crosschain_trx_index>().indices().get<by_withdraw_link_account>().equal_range(account);
		for (const auto crosschain_trx_obj : boost::make_iterator_range(cross_trx_range.first, cross_trx_range.second)) {
			result.push_back(crosschain_trx_obj);
		}
	}
	return result;
}
vector<crosschain_trx_object> database_api_impl::get_crosschain_transaction(const transaction_stata& crosschain_trx_state, const transaction_id_type& id)const{
	vector<crosschain_trx_object> result;
	if (id == transaction_id_type()){
		_db.get_index_type<crosschain_trx_index >().inspect_all_objects([&](const object& o)
		{
			const crosschain_trx_object& p = static_cast<const crosschain_trx_object&>(o);
			if (p.trx_state == crosschain_trx_state) {
				result.push_back(p);
			}
		});
	}
	else{
		const auto & cct_db = _db.get_index_type<crosschain_trx_index>().indices().get<by_trx_type_state>();
		const auto cct_trx_iter = cct_db.find(boost::make_tuple(id, crosschain_trx_state));
		FC_ASSERT(cct_trx_iter != cct_db.end(), "Tansaction exist error");
		result.push_back(*cct_trx_iter);
	}
	
	return result;
}
vector<optional<crosschain_trx_object>> database_api_impl::get_crosschain_transaction_by_blocknum(const string& symbol, 
	const string& account, 
	const uint32_t& start_block_num, 
	const uint32_t& stop_block_num, 
	const transaction_stata& crosschain_trx_state)const {
	vector<optional<crosschain_trx_object>> ret;
	FC_ASSERT(stop_block_num >= start_block_num, "End num must bigger than start num");
	auto start_trx_id = transaction_id_type("0000000000000000000000000000000000000000");
	auto stop_trx_id = transaction_id_type("ffffffffffffffffffffffffffffffffffffffff");
	if ("" == symbol) {
		if ("" == account) {
			const auto & cct_db = _db.get_index_type<crosschain_trx_index>().indices().get<by_block_num_state>();
			auto itr = cct_db.upper_bound(boost::make_tuple(crosschain_trx_state, start_block_num,  start_trx_id));
			auto itr_stop = cct_db.lower_bound(boost::make_tuple(crosschain_trx_state, stop_block_num, stop_trx_id));
			for (; itr != itr_stop;itr++){
				ret.push_back(*itr);
			}
		}
		else {
			const auto & cct_db = _db.get_index_type<crosschain_trx_index>().indices().get<by_account_block_num_state>();
			auto itr = cct_db.upper_bound(boost::make_tuple(account, crosschain_trx_state, start_block_num, start_trx_id));
			auto itr_stop = cct_db.lower_bound(boost::make_tuple(account, crosschain_trx_state, stop_block_num,  stop_trx_id));
			for (; itr != itr_stop; itr++) {
				ret.push_back(*itr);
			}
		}
	}
	else{
		if ("" == account) {
			const auto & cct_db = _db.get_index_type<crosschain_trx_index>().indices().get<by_symbol_block_num_state>();
			auto itr = cct_db.upper_bound(boost::make_tuple(symbol, crosschain_trx_state, start_block_num,  start_trx_id));
			auto itr_stop = cct_db.lower_bound(boost::make_tuple(symbol, crosschain_trx_state, stop_block_num,  stop_trx_id));
			for (; itr != itr_stop; itr++) {
				ret.push_back(*itr);
			}
		}
		else {
			const auto & cct_db = _db.get_index_type<crosschain_trx_index>().indices().get<by_account_symbol_block_num_state>();
			auto itr = cct_db.upper_bound(boost::make_tuple(account, symbol, crosschain_trx_state, start_block_num,  start_trx_id));
			auto itr_stop = cct_db.lower_bound(boost::make_tuple(account, symbol, crosschain_trx_state, stop_block_num,  stop_trx_id));
			for (; itr != itr_stop; itr++) {
				ret.push_back(*itr);
			}
		}
	}
	return ret;
}

std::map<std::string, share_type> database_api_impl::get_bonus_balances(const address& owner) const {
	return _db.get_bonus_balance(owner);
}

vector<vote_result_object> database_api_impl::get_vote_result_objs(const vote_object_id_type& id) const {
	vector<vote_result_object> result;
	const auto& vote_result_idx = _db.get_index_type<vote_result_index>().indices().get<by_vote>();
	auto range = vote_result_idx.equal_range(boost::make_tuple(id));

	for (const auto iter : boost::make_iterator_range(range.first, range.second))
	{
		result.push_back(iter);
	}
	return result;
}

vector<vote_object> database_api_impl::get_votes_by_addr(const address& addr) const {
	vector<vote_object> result;
	const auto citizen_obj = _db.get_citizen_obj(addr);
	FC_ASSERT(citizen_obj.valid());
	const auto& vote_idx = _db.get_index_type<vote_index>().indices().get<by_state>();
	const auto range = vote_idx.equal_range(false);
	std::for_each(range.first, range.second, [&result](const vote_object& obj ) {
		result.push_back(obj);
	});
	return result;
}

std::map<std::string, asset> database_api_impl::get_pay_back_balances(const address & pay_back_owner)const {
	const auto & payback_db = _db.get_index_type<payback_index>().indices().get<by_payback_address>().equal_range(pay_back_owner);
	std::map<string, asset> results;
	FC_ASSERT(payback_db.first != payback_db.second, "pay back owner doesnt exist");
	for (auto payback_address_iter : boost::make_iterator_range(payback_db.first, payback_db.second)) {
		const auto& miner_obj = _db.get(payback_address_iter.miner_id);
		const auto& miner_acc = _db.get(miner_obj.miner_account);
		results[miner_acc.name] = payback_address_iter.one_owner_balance;
	}
	return results;
}
vector<optional<multisig_address_object>> database_api_impl::get_multi_account_guard(const string & multi_address, const string& symbol)const {
	vector<optional<multisig_address_object>> result;
	auto& hot_db = _db.get_index_type<multisig_account_pair_index>().indices().get<by_bindhot_chain_type>();
	auto& cold_db = _db.get_index_type<multisig_account_pair_index>().indices().get<by_bindcold_chain_type>();
	auto hot_iterange = hot_db.equal_range(boost::make_tuple(multi_address, symbol));
	auto cold_iterange = cold_db.equal_range(boost::make_tuple(multi_address, symbol));
	multisig_account_pair_id_type multi_id;
	vector<multisig_account_pair_object> multisig_objs;
	if (hot_iterange.first != hot_iterange.second) {
		multisig_objs.assign(hot_iterange.first, hot_iterange.second);
		while (multisig_objs.size())
		{
			if (multisig_objs.back().effective_block_num)
			{
				multi_id = multisig_objs.rbegin()->id;
				break;
			}
			else
				multisig_objs.pop_back();
		}
	}
	if (cold_iterange.first != cold_iterange.second) {
		multisig_objs.assign(cold_iterange.first, cold_iterange.second);
		while (multisig_objs.size())
		{
			if (multisig_objs.back().effective_block_num)
			{
				multi_id = multisig_objs.rbegin()->id;
				break;
			}
			else
				multisig_objs.pop_back();
		}
	}
	FC_ASSERT(multi_id != multisig_account_pair_id_type(), "Can`t find coldhot multiaddress");
	auto guard_range = _db.get_index_type<multisig_address_index>().indices().get<by_multisig_account_pair_id>().equal_range(multi_id);
	for (auto guard : boost::make_iterator_range(guard_range.first,guard_range.second))
	{
		result.push_back(guard);
	}
	return result;
}
vector<guard_lock_balance_object> database_api_impl::get_guard_lock_balance(const guard_member_id_type& id)const {
	const auto& glb_index = _db.get_index_type<guard_lock_balance_index>();
	vector<guard_lock_balance_object> result;
	glb_index.inspect_all_objects([&](const object& obj) {
		const guard_lock_balance_object& p = static_cast<const guard_lock_balance_object&>(obj);
		if (p.lock_balance_account == id) {
			result.emplace_back(p);
		}
	});
	return result;
}
vector<guard_lock_balance_object> database_api_impl::get_guard_asset_lock_balance(const asset_id_type& id)const {
	const auto& glb_index = _db.get_index_type<guard_lock_balance_index>();
	vector<guard_lock_balance_object> result;
	glb_index.inspect_all_objects([&](const object& obj) {
		const guard_lock_balance_object& p = static_cast<const guard_lock_balance_object&>(obj);
		if (p.lock_asset_id == id) {
			result.emplace_back(p);
		}
	});
	return result;
}

vector<optional<multisig_address_object>> database_api_impl::get_multisig_address_obj(const string& symbol,const account_id_type& guard) const {
	vector<optional<multisig_address_object>> result;
	const auto& multisig_addr_by_guard = _db.get_index_type<multisig_address_index>().indices().get<by_account_chain_type>();
	const auto iter_range = multisig_addr_by_guard.equal_range(boost::make_tuple(symbol,guard));
	std::for_each(iter_range.first, iter_range.second, [&](multisig_address_object obj) {
		result.emplace_back(obj);
	});
	
	return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Private methods                                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api_impl::broadcast_updates( const vector<variant>& updates )
{
   if( updates.size() && _subscribe_callback ) {
      auto capture_this = shared_from_this();
      fc::async([capture_this,updates](){
          if(capture_this->_subscribe_callback)
            capture_this->_subscribe_callback( fc::variant(updates) );
      });
   }
}

void database_api_impl::broadcast_market_updates( const market_queue_type& queue)
{
   if( queue.size() )
   {
      auto capture_this = shared_from_this();
      fc::async([capture_this, this, queue](){
          for( const auto& item : queue )
          {
            auto sub = _market_subscriptions.find(item.first);
            if( sub != _market_subscriptions.end() )
                sub->second( fc::variant(item.second ) );
          }
      });
   }
}

void database_api_impl::on_objects_removed( const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_id_type>& impacted_accounts)
{
   handle_object_changed(_notify_remove_create, false, ids, impacted_accounts,
      [objs](object_id_type id) -> const object* {
         auto it = std::find_if(
               objs.begin(), objs.end(),
               [id](const object* o) {return o != nullptr && o->id == id;});

         if (it != objs.end())
            return *it;

         return nullptr;
      }
   );
}

void database_api_impl::on_objects_new(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts)
{
   handle_object_changed(_notify_remove_create, true, ids, impacted_accounts,
      std::bind(&object_database::find_object, &_db, std::placeholders::_1)
   );
}

void database_api_impl::on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts)
{
   handle_object_changed(false, true, ids, impacted_accounts,
      std::bind(&object_database::find_object, &_db, std::placeholders::_1)
   );
}

void database_api_impl::handle_object_changed(bool force_notify, bool full_object, const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts, std::function<const object*(object_id_type id)> find_object)
{
   if( _subscribe_callback )
   {
      vector<variant> updates;

      for(auto id : ids)
      {
         if( force_notify || is_subscribed_to_item(id) || is_impacted_account(impacted_accounts) )
         {
            if( full_object )
            {
               auto obj = find_object(id);
               if( obj )
               {
                  updates.emplace_back( obj->to_variant() );
               }
            }
            else
            {
               updates.emplace_back( id );
            }
         }
      }

      broadcast_updates(updates);
   }

   if( _market_subscriptions.size() )
   {
      market_queue_type broadcast_queue;

      for(auto id : ids)
      {
         if( id.is<call_order_object>() )
         {
            enqueue_if_subscribed_to_market<call_order_object>( find_object(id), broadcast_queue, full_object );
         }
         else if( id.is<limit_order_object>() )
         {
            enqueue_if_subscribed_to_market<limit_order_object>( find_object(id), broadcast_queue, full_object );
         }
      }

      broadcast_market_updates(broadcast_queue);
   }
}

/** note: this method cannot yield because it is called in the middle of
 * apply a block.
 */
void database_api_impl::on_applied_block()
{
   if (_block_applied_callback)
   {
      auto capture_this = shared_from_this();
      block_id_type block_id = _db.head_block_id();
      fc::async([this,capture_this,block_id](){
         _block_applied_callback(fc::variant(block_id));
      });
   }

   if(_market_subscriptions.size() == 0)
      return;

   //const auto& ops = _db.get_applied_operations();
   //map< std::pair<asset_id_type,asset_id_type>, vector<pair<operation, operation_result>> > subscribed_markets_ops;
   //for(const optional< operation_history_object >& o_op : ops)
   //{
   //   if( !o_op.valid() )
   //      continue;
   //   const operation_history_object& op = *o_op;

   //   std::pair<asset_id_type,asset_id_type> market;
   //   switch(op.op.which())
   //   {
   //      /*  This is sent via the object_changed callback
   //      case operation::tag<limit_order_create_operation>::value:
   //         market = op.op.get<limit_order_create_operation>().get_market();
   //         break;
   //      */
   //      case operation::tag<fill_order_operation>::value:
   //         market = op.op.get<fill_order_operation>().get_market();
   //         break;
   //         /*
   //      case operation::tag<limit_order_cancel_operation>::value:
   //      */
   //      default: break;
   //   }
   //   if(_market_subscriptions.count(market))
   //      subscribed_markets_ops[market].push_back(std::make_pair(op.op, op.result));
   //}
   /// we need to ensure the database_api is not deleted for the life of the async operation
  /* auto capture_this = shared_from_this();
   fc::async([this,capture_this,subscribed_markets_ops](){
	  for(auto item : subscribed_markets_ops)
	  {
		 auto itr = _market_subscriptions.find(item.first);
		 if(itr != _market_subscriptions.end())
			itr->second(fc::variant(item.second));
	  }
   });*/
}

} } // graphene::app
