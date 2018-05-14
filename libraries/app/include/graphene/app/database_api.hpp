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

#include <graphene/app/full_account.hpp>

#include <graphene/chain/protocol/types.hpp>

#include <graphene/chain/database.hpp>

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
#include <graphene/market_history/market_history_plugin.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/contract_object.hpp>
#include <graphene/chain/pay_back_object.hpp>
#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace graphene { namespace app {

using namespace graphene::chain;
using namespace graphene::market_history;
using namespace std;

class database_api_impl;

struct order
{
   double                     price;
   double                     quote;
   double                     base;
};

struct order_book
{
  string                      base;
  string                      quote;
  vector< order >             bids;
  vector< order >             asks;
};

struct market_ticker
{
   string                     base;
   string                     quote;
   double                     latest;
   double                     lowest_ask;
   double                     highest_bid;
   double                     percent_change;
   double                     base_volume;
   double                     quote_volume;
};

struct market_volume
{
   string                     base;
   string                     quote;
   double                     base_volume;
   double                     quote_volume;
};

struct market_trade
{
   fc::time_point_sec         date;
   double                     price;
   double                     amount;
   double                     value;
};

/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
class database_api
{
   public:
      database_api(graphene::chain::database& db);
      ~database_api();

      /////////////
      // Objects //
      /////////////

      /**
       * @brief Get the objects corresponding to the provided IDs
       * @param ids IDs of the objects to retrieve
       * @return The objects retrieved, in the order they are mentioned in ids
       *
       * If any of the provided IDs does not map to an object, a null variant is returned in its position.
       */
      fc::variants get_objects(const vector<object_id_type>& ids)const;

      ///////////////////
      // Subscriptions //
      ///////////////////

      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      /**
       * @brief Stop receiving any notifications
       *
       * This unsubscribes from all subscribed markets and objects.
       */
      void cancel_all_subscriptions();

      /////////////////////////////
      // Blocks and transactions //
      /////////////////////////////

      /**
       * @brief Retrieve a block header
       * @param block_num Height of the block whose header should be returned
       * @return header of the referenced block, or null if no matching block was found
       */
      optional<block_header> get_block_header(uint32_t block_num)const;

      /**
      * @brief Retrieve multiple block header by block numbers
      * @param block_num vector containing heights of the block whose header should be returned
      * @return array of headers of the referenced blocks, or null if no matching block was found
      */
      map<uint32_t, optional<block_header>> get_block_header_batch(const vector<uint32_t> block_nums)const;


      /**
       * @brief Retrieve a full, signed block
       * @param block_num Height of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      optional<signed_block> get_block(uint32_t block_num)const;

      /**
       * @brief used to fetch an individual transaction.
       */
      processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;

      /**
       * If the transaction has not expired, this method will return the transaction for the given ID or
       * it will return NULL if it is not known.  Just because it is not known does not mean it wasn't
       * included in the blockchain.
       */
      optional<signed_transaction> get_recent_transaction_by_id( const transaction_id_type& id )const;

      /////////////
      // Globals //
      /////////////

      /**
       * @brief Retrieve the @ref chain_property_object associated with the chain
       */
      chain_property_object get_chain_properties()const;

      /**
       * @brief Retrieve the current @ref global_property_object
       */
      global_property_object get_global_properties()const;

      /**
       * @brief Retrieve compile-time constants
       */
      fc::variant_object get_config()const;

      /**
       * @brief Get the chain ID
       */
      chain_id_type get_chain_id()const;

      /**
       * @brief Retrieve the current @ref dynamic_global_property_object
       */
      dynamic_global_property_object get_dynamic_global_properties()const;
	  void set_guarantee_id(guarantee_object_id_type id);
      //////////
      // Keys //
      //////////

      vector<vector<account_id_type>> get_key_references( vector<public_key_type> key )const;
	  vector <optional<account_binding_object>> get_binding_account(const string& account, const string& symbol) const;
	  vector<optional<multisig_address_object>> get_multisig_address_obj(const string& symbol, const account_id_type& guard) const;
	  vector<optional<multisig_account_pair_object>> get_multisig_account_pair(const string& symbol) const;
     /**
      * Determine whether a textual representation of a public key
      * (in Base-58 format) is *currently* linked
      * to any *registered* (i.e. non-stealth) account on the blockchain
      * @param public_key Public key
      * @return Whether a public key is known
      */
     bool is_public_key_registered(string public_key) const;

      //////////////
      // Accounts //
      //////////////

      /**
       * @brief Get a list of accounts by ID
       * @param account_ids IDs of the accounts to retrieve
       * @return The accounts corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;
	  /**
	  * @brief Get a list of accounts by address
	  * @param addresses of the accounts to retrieve
	  * @return The accounts corresponding to the provided addresses
	  *
	  * This function has semantics identical to @ref get_objects
	  */
	  vector<optional<account_object>> get_accounts_addr(const vector<address>& account_ids)const;

      /**
       * @brief Fetch all objects relevant to the specified accounts and subscribe to updates
       * @param callback Function to call with updates
       * @param names_or_ids Each item must be the name or ID of an account to retrieve
       * @return Map of string from @ref names_or_ids to the corresponding account
       *
       * This function fetches all relevant objects for the given accounts, and subscribes to updates to the given
       * accounts. If any of the strings in @ref names_or_ids cannot be tied to an account, that input will be
       * ignored. All other accounts will be retrieved and subscribed.
       *
       */
      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );

      optional<account_object> get_account_by_name( string name )const;

      /**
       *  @return all accounts that referr to the key or account id in their owner or active authorities.
       */
      vector<account_id_type> get_account_references( account_id_type account_id )const;

      /**
       * @brief Get a list of accounts by name
       * @param account_names Names of the accounts to retrieve
       * @return The accounts holding the provided names
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;

	  /**
	  * @brief Get a list of multisigs
	  * @param account_names Names of the accounts to retrieve
	  * @return The accounts holding the provided names
	  *
	  * This function has semantics identical to @ref get_objects
	  */
	  vector<multisig_asset_transfer_object> get_multisigs_trx()const;
	  optional<multisig_asset_transfer_object> lookup_multisig_asset(multisig_asset_transfer_id_type id)const;
      /**
       * @brief Get names and IDs for registered accounts
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of account names to corresponding IDs
       */
      map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;

      //////////////
      // Balances //
      //////////////

      /**
       * @brief Get an account's balances in various assets
       * @param id ID of the account to get balances for
       * @param assets IDs of the assets to get balances of; if empty, get all assets account has a balance in
       * @return Balances of the account
       */
      vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;

      /// Semantically equivalent to @ref get_account_balances, but takes a name instead of an ID.
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;

      /** @return all unclaimed balance objects for a set of addresses */
      vector<balance_object> get_balance_objects( const vector<address>& addrs )const;

      vector<asset> get_vested_balances( const vector<balance_id_type>& objs )const;

      vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;

      /**
       * @brief Get the total number of accounts registered with the blockchain
       */
      uint64_t get_account_count()const;

      ////////////
      // Assets //
      ////////////

      /**
       * @brief Get a list of assets by ID
       * @param asset_ids IDs of the assets to retrieve
       * @return The assets corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;

      /**
       * @brief Get assets alphabetically by symbol name
       * @param lower_bound_symbol Lower bound of symbol names to retrieve
       * @param limit Maximum number of assets to fetch (must not exceed 100)
       * @return The assets found
       */
      vector<asset_object> list_assets(const string& lower_bound_symbol, uint32_t limit)const;

      /**
       * @brief Get a list of assets by symbol
       * @param asset_symbols Symbols or stringified IDs of the assets to retrieve
       * @return The assets corresponding to the provided symbols or IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

      /////////////////////
      // Markets / feeds //
      /////////////////////

      /**
       * @brief Get limit orders in a given market
       * @param a ID of asset being sold
       * @param b ID of asset being purchased
       * @param limit Maximum number of orders to retrieve
       * @return The limit orders, ordered from least price to greatest
       */
      vector<limit_order_object> get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const;

      /**
       * @brief Get call orders in a given asset
       * @param a ID of asset being called
       * @param limit Maximum number of orders to retrieve
       * @return The call orders, ordered from earliest to be called to latest
       */
      vector<call_order_object> get_call_orders(asset_id_type a, uint32_t limit)const;

      /**
       * @brief Get forced settlement orders in a given asset
       * @param a ID of asset being settled
       * @param limit Maximum number of orders to retrieve
       * @return The settle orders, ordered from earliest settlement date to latest
       */
      vector<force_settlement_object> get_settle_orders(asset_id_type a, uint32_t limit)const;

      /**
       *  @return all open margin positions for a given account id.
       */
      vector<call_order_object> get_margin_positions( const account_id_type& id )const;

      /**
       * @brief Request notification when the active orders in the market between two assets changes
       * @param callback Callback method which is called when the market changes
       * @param a First asset ID
       * @param b Second asset ID
       *
       * Callback will be passed a variant containing a vector<pair<operation, operation_result>>. The vector will
       * contain, in order, the operations which changed the market, and their results.
       */
      void subscribe_to_market(std::function<void(const variant&)> callback,
                   asset_id_type a, asset_id_type b);

      /**
       * @brief Unsubscribe from updates to a given market
       * @param a First asset ID
       * @param b Second asset ID
       */
      void unsubscribe_from_market( asset_id_type a, asset_id_type b );

      /**
       * @brief Returns the ticker for the market assetA:assetB
       * @param a String name of the first asset
       * @param b String name of the second asset
       * @return The market ticker for the past 24 hours.
       */
      market_ticker get_ticker( const string& base, const string& quote )const;

      /**
       * @brief Returns the 24 hour volume for the market assetA:assetB
       * @param a String name of the first asset
       * @param b String name of the second asset
       * @return The market volume over the past 24 hours
       */
      market_volume get_24_volume( const string& base, const string& quote )const;

      /**
       * @brief Returns the order book for the market base:quote
       * @param base String name of the first asset
       * @param quote String name of the second asset
       * @param depth of the order book. Up to depth of each asks and bids, capped at 50. Prioritizes most moderate of each
       * @return Order book of the market
       */
      order_book get_order_book( const string& base, const string& quote, unsigned limit = 50 )const;

      /**
       * @brief Returns recent trades for the market assetA:assetB
       * Note: Currentlt, timezone offsets are not supported. The time must be UTC.
       * @param a String name of the first asset
       * @param b String name of the second asset
       * @param stop Stop time as a UNIX timestamp
       * @param limit Number of trasactions to retrieve, capped at 100
       * @param start Start time as a UNIX timestamp
       * @return Recent transactions in the market
       */
      vector<market_trade> get_trade_history( const string& base, const string& quote, fc::time_point_sec start, fc::time_point_sec stop, unsigned limit = 100 )const;



      ///////////////
      // Witnesses //
      ///////////////

      /**
       * @brief Get a list of witnesses by ID
       * @param witness_ids IDs of the witnesses to retrieve
       * @return The witnesses corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<miner_object>> get_miners(const vector<miner_id_type>& witness_ids)const;

      /**
       * @brief Get the witness owned by a given account
       * @param account The ID of the account whose witness should be retrieved
       * @return The witness object, or null if the account does not have a witness
       */
      fc::optional<miner_object> get_miner_by_account(account_id_type account)const;

      /**
       * @brief Get names and IDs for registered witnesses
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of witness names to corresponding IDs
       */
      map<string, miner_id_type> lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const;

      /**
       * @brief Get the total number of witnesses registered with the blockchain
       */
      uint64_t get_miner_count()const;

      ///////////////////////
      // Committee members //
      ///////////////////////

      /**
       * @brief Get a list of committee_members by ID
       * @param committee_member_ids IDs of the committee_members to retrieve
       * @return The committee_members corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<guard_member_object>> get_guard_members(const vector<guard_member_id_type>& committee_member_ids)const;

      /**
       * @brief Get the committee_member owned by a given account
       * @param account The ID of the account whose committee_member should be retrieved
       * @return The committee_member object, or null if the account does not have a committee_member
       */
      fc::optional<guard_member_object> get_guard_member_by_account(account_id_type account)const;

      /**
       * @brief Get names and IDs for registered committee_members
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of committee_member names to corresponding IDs
       */
      map<string, guard_member_id_type> lookup_guard_member_accounts(const string& lower_bound_name, uint32_t limit,bool formal=false)const;


      /// WORKERS

      /**
       * Return the worker objects associated with this account.
       */
      vector<worker_object> get_workers_by_account(account_id_type account)const;


      ///////////
      // Votes //
      ///////////

      /**
       *  @brief Given a set of votes, return the objects they are voting for.
       *
       *  This will be a mixture of guard_member_object, miner_objects, and worker_objects
       *
       *  The results will be in the same order as the votes.  Null will be returned for
       *  any vote ids that are not found.
       */
      vector<variant> lookup_vote_ids( const vector<vote_id_type>& votes )const;

      ////////////////////////////
      // Authority / validation //
      ////////////////////////////

      /// @brief Get a hexdump of the serialized binary form of a transaction
      std::string get_transaction_hex(const signed_transaction& trx)const;

      /**
       *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
       *  and return the minimal subset of public keys that should add signatures to the transaction.
       */
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;

      /**
       *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
       *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
       *  to get the minimum subset.
       */
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      set<address> get_potential_address_signatures( const signed_transaction& trx )const;

      /**
       * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
       */
      bool           verify_authority( const signed_transaction& trx )const;

      /**
       * @return true if the signers have enough authority to authorize an account
       */
      bool           verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

      /**
       *  Validates a transaction against the current state without broadcasting it on the network.
       */
      processed_transaction validate_transaction( const signed_transaction& trx ,bool testing=false)const;

      /**
       *  For each operation calculate the required fee in the specified asset type.  If the asset type does
       *  not have a valid core_exchange_rate
       */
      vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;

      ///////////////////////////
      // Proposed transactions //
      ///////////////////////////

      /**
       *  @return the set of proposed transactions relevant to the specified account id.
       */
      vector<proposal_object> get_proposed_transactions( account_id_type id )const;
	  vector<proposal_object> get_proposer_transactions(account_id_type id)const;
	  vector<proposal_object> get_voter_transactions_waiting(address add)const;  //waiting to be voted
      //////////////////////
      // Blinded balances //
      //////////////////////

      /**
       *  @return the set of blinded balance objects by commitment ID
       */
      vector<blinded_balance_object> get_blinded_balances( const flat_set<commitment_type>& commitments )const;
	  //Lock balance
	  vector<lockbalance_object> get_account_lock_balance(const account_id_type& id)const;
	  vector<lockbalance_object> get_asset_lock_balance(const asset_id_type& asset) const;
	  vector<lockbalance_object> get_miner_lock_balance(const miner_id_type& miner) const;

	  vector<guard_lock_balance_object> get_guard_lock_balance(const guard_member_id_type& id)const;
	  vector<guard_lock_balance_object> get_guard_asset_lock_balance(const asset_id_type& id)const;

	  vector<crosschain_trx_object> get_account_crosschain_transaction(const string& account)const;
	  vector<crosschain_trx_object> get_crosschain_transaction(const transaction_stata& crosschain_trx_state, const transaction_id_type& id)const;
	  vector<coldhot_transfer_object> get_coldhot_transaction(const coldhot_trx_state& coldhot_tx_state, const transaction_id_type& id)const;
	  optional<multisig_account_pair_object> lookup_multisig_account_pair(const multisig_account_pair_id_type& id) const;
	  vector<optional<multisig_address_object>> get_multi_account_guard(const string & multi_address, const string& symbol)const;
	  std::map<std::string, asset> get_pay_back_balances(const address & pay_back_owner)const;
      //contract 
      contract_object get_contract_object(const string& contract_address)const;
	  contract_object get_contract_object_by_name(const string& contract_name)const;

      ContractEntryPrintable get_contract_info(const string& contract_address)const;

      ContractEntryPrintable get_contract_info_by_name(const string& contract_address)const;
      vector<asset> get_contract_balance(const address& contract_address) const;
      vector<address> get_contract_addresses_by_owner(const address&)const ;
      vector<contract_object> get_contracts_by_owner(const address&)const;

	  vector<optional<guarantee_object>> list_guarantee_object(const string& chain_type,bool all=true) const;
	  optional<guarantee_object> get_gurantee_object(const guarantee_object_id_type id) const;
	  vector<optional<guarantee_object>> get_guarantee_orders(const address& addr, bool all) const;
      vector<contract_event_notify_object> get_contract_event_notify(const address& contract_id, const transaction_id_type& trx_id, const string& event_name) const;
      optional<contract_event_notify_object> get_contract_event_notify_by_id(const contract_event_notify_object_id_type& id);

      vector<contract_invoke_result_object> get_contract_invoke_object(const transaction_id_type& trx_id)const ;

      vector<contract_event_notify_object> get_contract_events(const address&)const ;
      vector<contract_resgister_record> get_contract_registered(const uint32_t block_num) const;
   private:
      std::shared_ptr< database_api_impl > my;
};

} }

FC_REFLECT( graphene::app::order, (price)(quote)(base) );
FC_REFLECT( graphene::app::order_book, (base)(quote)(bids)(asks) );
FC_REFLECT( graphene::app::market_ticker, (base)(quote)(latest)(lowest_ask)(highest_bid)(percent_change)(base_volume)(quote_volume) );
FC_REFLECT( graphene::app::market_volume, (base)(quote)(base_volume)(quote_volume) );
FC_REFLECT( graphene::app::market_trade, (date)(price)(amount)(value) );

FC_API(graphene::app::database_api,
	// Objects
	(get_objects)

	// Subscriptions
	(set_subscribe_callback)
	(set_pending_transaction_callback)
	(set_block_applied_callback)
	(cancel_all_subscriptions)

	// Blocks and transactions
	(get_block_header)
	(get_block_header_batch)
	(get_block)
	(get_transaction)
	(get_recent_transaction_by_id)

	// Globals
	(get_chain_properties)
	(get_global_properties)
	(get_config)
	(get_chain_id)
	(get_dynamic_global_properties)
	(set_guarantee_id)
	// Keys
	(get_key_references)
	(is_public_key_registered)

	// Accounts
	(get_accounts)
	(get_full_accounts)
	(get_account_by_name)
	(get_account_references)
	(lookup_account_names)
	(lookup_accounts)
	(get_account_count)
	(get_accounts_addr)

	// Balances
	(get_account_balances)
	(get_named_account_balances)
	(get_balance_objects)
	(get_vested_balances)
	(get_vesting_balances)

	// Assets
	(get_assets)
	(list_assets)
	(lookup_asset_symbols)

	// Markets / feeds
	(get_order_book)
	(get_limit_orders)
	(get_call_orders)
	(get_settle_orders)
	(get_margin_positions)
	(subscribe_to_market)
	(unsubscribe_from_market)
	(get_ticker)
	(get_24_volume)
	(get_trade_history)

	// Witnesses
	(get_miners)
	(get_miner_by_account)
	(lookup_miner_accounts)
	(get_miner_count)

	// Committee members
	(get_guard_members)
	(get_guard_member_by_account)
	(lookup_guard_member_accounts)

	// workers
	(get_workers_by_account)
	// Votes
	(lookup_vote_ids)

	// Authority / validation
	(get_transaction_hex)
	(get_required_signatures)
	(get_potential_signatures)
	(get_potential_address_signatures)
	(verify_authority)
	(verify_account_authority)
	(validate_transaction)
	(get_required_fees)

	// Proposed transactions
	(get_proposed_transactions)
	(get_proposer_transactions)
	(get_voter_transactions_waiting)
	// Blinded balances
	(get_blinded_balances)
	// Lock balance
	(get_account_lock_balance)
	(get_asset_lock_balance)
	(get_miner_lock_balance)

	(get_guard_lock_balance)
	(get_guard_asset_lock_balance)
	(get_multisigs_trx)
	(lookup_multisig_asset)
	(get_binding_account)
	(get_crosschain_transaction)
	(get_account_crosschain_transaction)
	(get_coldhot_transaction)
	(get_multi_account_guard)
	(get_multisig_address_obj)
	(get_multisig_account_pair)
	(lookup_multisig_account_pair)
	(get_guarantee_orders)
	(get_pay_back_balances)
    //contract
    (get_contract_object)
	(get_contract_object_by_name)
    (get_contract_info)
    (get_contract_info_by_name)
    (get_contract_balance)
    (get_contract_event_notify)
	(get_gurantee_object)
	(list_guarantee_object)
    (get_contract_event_notify_by_id)
    (get_contract_invoke_object)
    (get_contract_addresses_by_owner)
    (get_contracts_by_owner)
    (get_contract_events)
    (get_contract_registered)
);