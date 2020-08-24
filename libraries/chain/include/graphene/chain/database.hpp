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
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/node_property_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/fork_database.hpp>
#include <graphene/chain/block_database.hpp>
#include <graphene/chain/genesis_state.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <graphene/chain/coldhot_transfer_object.hpp>
#include <graphene/db/object_database.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/simple_index.hpp>
#include <fc/signals.hpp>
#include <fc/crypto/elliptic.hpp>
#include <graphene/chain/protocol/protocol.hpp>
#include <graphene/chain/contract_object.hpp>
#include <graphene/db/backup_database.hpp>
#include <fc/log/logger.hpp>
#include <fc/uint128.hpp>

#include <map>

namespace graphene { namespace chain {
   using graphene::db::abstract_object;
   using graphene::db::object;
   class op_evaluator;
   class transaction_evaluation_state;

   struct budget_record;

   /**
    *   @class database
    *   @brief tracks the blockchain state in an extensible manner
    */
   class database : public db::object_database
   {
      public:
         //////////////////// db_management.cpp ////////////////////

         database();
         ~database();

         enum validation_steps
         {
            skip_nothing                = 0,
            skip_miner_signature      = 1 << 0,  ///< used while reindexing
            skip_transaction_signatures = 1 << 1,  ///< used by non-witness nodes
            skip_transaction_dupe_check = 1 << 2,  ///< used while reindexing
            skip_fork_db                = 1 << 3,  ///< used while reindexing
            skip_block_size_check       = 1 << 4,  ///< used when applying locally generated transactions
            skip_tapos_check            = 1 << 5,  ///< used while reindexing -- note this skips expiration check as well
            skip_authority_check        = 1 << 6,  ///< used while reindexing -- disables any checking of authority on transactions
            skip_merkle_check           = 1 << 7,  ///< used while reindexing
            skip_assert_evaluation      = 1 << 8,  ///< used while reindexing
            skip_undo_history_check     = 1 << 9,  ///< used while reindexing
            skip_witness_schedule_check = 1 << 10,  ///< used while reindexing
            skip_validate               = 1 << 11, ///< used prior to checkpoint, skips validate() call on transaction
            check_gas_price             = 1 << 12,
			throw_over_limit            = 1 << 13,
			skip_contract_exec          = 1 << 14,
			skip_contract_db_check      = 1 << 15
         };

         /**
          * @brief Open a database, creating a new one if necessary
          *
          * Opens a database in the specified directory. If no initialized database is found, genesis_loader is called
          * and its return value is used as the genesis state when initializing the new database
          *
          * genesis_loader will not be called if an existing database is found.
          *
          * @param data_dir Path to open or create database in
          * @param genesis_loader A callable object which returns the genesis state to initialize new databases on
          */
          void open(
             const fc::path& data_dir,
             std::function<genesis_state_type()> genesis_loader );

         /**
          * @brief Rebuild object graph from block history and open detabase
          *
          * This method may be called after or instead of @ref database::open, and will rebuild the object graph by
          * replaying blockchain history. When this method exits successfully, the database will be open.
          */
         void reindex(fc::path data_dir, const genesis_state_type& initial_allocation = genesis_state_type());
		 void reindex_part(fc::path data_dir);
         /**
          * @brief wipe Delete database from disk, and potentially the raw chain as well.
          * @param include_blocks If true, delete the raw chain as well as the database.
          *
          * Will close the database before wiping. Database will be closed when this function returns.
          */
         void wipe(const fc::path& data_dir, bool include_blocks);
         void close();
		 void clear();
         //////////////////// db_block.cpp ////////////////////

         /**
          *  @return true if the block is in our fork DB or saved to disk as
          *  part of the official chain, otherwise return false
          */
         bool                       is_known_block( const block_id_type& id )const;
         bool                       is_known_transaction( const transaction_id_type& id )const;
         block_id_type              get_block_id_for_num( uint32_t block_num )const;
         optional<signed_block>     fetch_block_by_id( const block_id_type& id )const;
         optional<signed_block>     fetch_block_by_number( uint32_t num )const;
         const signed_transaction&  get_recent_transaction( const transaction_id_type& trx_id )const;
         std::vector<block_id_type> get_block_ids_on_fork(block_id_type head_of_fork) const;
		 optional<miner_object>     get_citizen_obj(const address& addr) const;
		 vector<miner_object>     get_citizen_objs(const vector<address>& addr) const;
         /**
          *  Calculate the percent of block production slots that were missed in the
          *  past 128 blocks, not including the current block.
          */
         uint32_t miner_participation_rate()const;

         void                              add_checkpoints( const flat_map<uint32_t,block_id_type>& checkpts );
         const flat_map<uint32_t,block_id_type> get_checkpoints()const { return _checkpoints; }
         bool before_last_checkpoint()const;

         bool push_block( const signed_block& b, uint32_t skip = skip_nothing );
         processed_transaction push_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing );
         bool _push_block( const signed_block& b );
         processed_transaction _push_transaction( const signed_transaction& trx );

         ///@throws fc::exception if the proposed transaction fails to apply.
		 void determine_referendum_detailes();
         processed_transaction push_proposal( const proposal_object& proposal );
		 processed_transaction push_referendum(const referendum_object& referendum);
		 void clear_votes();
         signed_block generate_block(
            const fc::time_point_sec when,
            miner_id_type miner_id,
            const fc::ecc::private_key& block_signing_private_key,
            uint32_t skip
            );
         signed_block _generate_block(
            const fc::time_point_sec when,
            miner_id_type miner_id,
            const fc::ecc::private_key& block_signing_private_key
            );

         void pop_block();
         void clear_pending();
		 SecretHashType get_secret(uint32_t block_num,
			 const fc::ecc::private_key& block_signing_private_key);

         /**
          *  This method is used to track appied operations during the evaluation of a block, these
          *  operations should include any operation actually included in a transaction as well
          *  as any implied/virtual operations that resulted, such as filling an order.  The
          *  applied operations is cleared after applying each block and calling the block
          *  observers which may want to index these operations.
          *
          *  @return the op_id which can be used to set the result after it has finished being applied.
          */
         uint32_t  push_applied_operation( const operation& op );
         void      set_applied_operation_result( uint32_t op_id, const operation_result& r );
         const vector<optional< operation_history_object > >& get_applied_operations()const;
         optional<asset_object> get_asset(const string& symbol) const ;
         string to_pretty_string( const asset& a )const;
		 /**
		 *  This signal is emitted after all operations and virtual operation for a
		 *  block have been applied but before the get_applied_operations() are cleared.
		 *
		 *  You may not yield from this callback because the blockchain is holding
		 *  the write lock and may be in an "inconstant state" until after it is
		 *  released.
		 */
		 fc::signal<void(const signed_block&)>           applied_block;
		 fc::signal<void(const signed_block&)>               applied_backup;
		 fc::signal<void(const vector<signed_transaction>&)>  removed_trxs;
		 fc::signal<void(const deque< signed_transaction>&)> broad_trxs;
         /**
          * This signal is emitted any time a new transaction is added to the pending
          * block state.
          */
         fc::signal<void(const signed_transaction&)>     on_pending_transaction;

         /**
          *  Emitted After a block has been applied and committed.  The callback
          *  should not yield and should execute quickly.
          */
         fc::signal<void(const vector<object_id_type>&, const flat_set<account_id_type>&)> new_objects;

         /**
          *  Emitted After a block has been applied and committed.  The callback
          *  should not yield and should execute quickly.
          */
         fc::signal<void(const vector<object_id_type>&, const flat_set<account_id_type>&)> changed_objects;

         /** this signal is emitted any time an object is removed and contains a
          * pointer to the last value of every object that was removed.
          */
         fc::signal<void(const vector<object_id_type>&, const vector<const object*>&, const flat_set<account_id_type>&)>  removed_objects;

         //////////////////// db_witness_schedule.cpp ////////////////////

         /**
          * @brief Get the witness scheduled for block production in a slot.
          *
          * slot_num always corresponds to a time in the future.
          *
          * If slot_num == 1, returns the next scheduled witness.
          * If slot_num == 2, returns the next scheduled witness after
          * 1 block gap.
          *
          * Use the get_slot_time() and get_slot_at_time() functions
          * to convert between slot_num and timestamp.
          *
          * Passing slot_num == 0 returns GRAPHENE_NULL_WITNESS
          */
         miner_id_type get_scheduled_miner(uint32_t slot_num)const;

         /**
          * Get the time at which the given slot occurs.
          *
          * If slot_num == 0, return time_point_sec().
          *
          * If slot_num == N for N > 0, return the Nth next
          * block-interval-aligned time greater than head_block_time().
          */
         fc::time_point_sec get_slot_time(uint32_t slot_num)const;

         /**
          * Get the last slot which occurs AT or BEFORE the given time.
          *
          * The return value is the greatest value N such that
          * get_slot_time( N ) <= when.
          *
          * If no such N exists, return 0.
          */
         uint32_t get_slot_at_time(fc::time_point_sec when)const;

         void update_miner_schedule();

		 void pay_miner(const miner_id_type& miner_id,asset fee);
		 asset get_fee_from_block(const signed_block& b);
		 void update_fee_pool();
		 share_type get_miner_pay_per_block(uint32_t block_num);

		 void reset_current_collected_fee();

		 void modify_current_collected_fee(share_type changed_fee);
		 void modify_current_collected_fee(asset changed_fee);

		 void update_witness_random_seed(const SecretHashType& new_secret);
		 bool is_white(const address& addr, const int& op) const;
         //////////////////// db_getter.cpp ////////////////////

         const chain_id_type&                   get_chain_id()const;
         const asset_object&                    get_core_asset()const;
         const chain_property_object&           get_chain_properties()const;
         const global_property_object&          get_global_properties()const;
         const dynamic_global_property_object&  get_dynamic_global_properties()const;
		 const total_fees_object&               get_total_fees_obj()const;
		 const lockbalance_record_object&        get_lockbalance_records() const;
         const node_property_object&            get_node_properties()const;
         const fee_schedule&                    current_fee_schedule()const;

         time_point_sec   head_block_time()const;
         uint32_t         head_block_num()const;
         block_id_type    head_block_id()const;
         miner_id_type  head_block_witness()const;

         decltype( chain_parameters::block_interval ) block_interval( )const;

         node_property_object& node_properties();


         uint32_t last_non_undoable_block_num() const;
         //////////////////// db_init.cpp ////////////////////

         void initialize_evaluators();
         /// Reset the object graph in-memory
         void initialize_indexes();
         void init_genesis(const genesis_state_type& genesis_state = genesis_state_type());

         template<typename EvaluatorType>
         void register_evaluator()
         {
            _operation_evaluators[
               operation::tag<typename EvaluatorType::operation_type>::value].reset( new op_evaluator_impl<EvaluatorType>() );
         }
		 //////////////////// db_pay_back.cpp/////////////////
		 void adjust_pay_back_balance(address payback_owner, asset payback_asset, miner_id_type miner_id = miner_id_type(0));
	     void adjust_bonus_balance(address bonus_owner, asset bonus);
		 std::map<string, share_type> get_bonus_balance(address owner)const;
		 std::map<string,asset> get_pay_back_balacne(address payback_owner,std::string symbol_type)const;
		 //////////////////// db_lock_balance.cpp/////////////////
		 asset get_lock_balance(account_id_type owner,miner_id_type miner, asset_id_type asset_id)const;
		 vector<lockbalance_object> get_lock_balance(account_id_type owner, asset_id_type asset_id)const;
		 asset get_guard_lock_balance(guard_member_id_type guard, asset_id_type asset_id)const;
		 //asset get_lock_balance(const account_object& owner, const asset_object& asset_obj)const;
		 //asset get_lock_balance(const address& addr, const asset_id_type asset_id) const;
		 void adjust_lock_balance(miner_id_type miner_account, account_id_type lock_account, asset delta);
		 void adjust_guard_lock_balance(guard_member_id_type guard_account,asset delta);
		 ////////db_crosschain_trx.cpp/////////////
		 void create_result_transaction(miner_id_type miner, fc::ecc::private_key pk);
		 void combine_sign_transaction(miner_id_type miner, fc::ecc::private_key pk);
		 void create_acquire_crosschhain_transaction(miner_id_type miner, fc::ecc::private_key pk);
		 void adjust_crosschain_transaction(transaction_id_type relate_transaction_id,
			 transaction_id_type transaction_id,
			 signed_transaction real_transaction,
			 uint64_t op_type,
			 transaction_stata trx_state,
			 vector<transaction_id_type> relate_transaction_ids = vector<transaction_id_type>());
		 void adjust_coldhot_transaction(transaction_id_type relate_trx_id,
			 transaction_id_type current_trx_id,
			 signed_transaction current_trx,
			 uint64_t op_type
			 );
		 void adjust_eths_multi_account_record(transaction_id_type pre_trx_id,
			 transaction_id_type current_trx_id,
			 signed_transaction current_trx,
			 uint64_t op_type);
		 void create_coldhot_transfer_trx(miner_id_type miner, fc::ecc::private_key pk);
		 void combine_coldhot_sign_transaction(miner_id_type miner, fc::ecc::private_key pk);

		 void adjust_deposit_to_link_trx(const hd_trx& handled_trx);
		 void adjust_crosschain_confirm_trx(const hd_trx& handled_trx);
		 //////contract//////
		 StorageDataType get_contract_storage(const address& contract_id, const string& name);
                 std::map<std::string, StorageDataType> get_contract_all_storages(const address& contract_id);
		 optional<contract_storage_object>  get_contract_storage_object(const address& contract_id, const string& name);
		 void set_contract_storage(const address& contract_id, const string& name, const StorageDataType &value);
		 void set_contract_storage_in_contract(const contract_object& contract, const string& name, const StorageDataType& value);
		 void add_contract_storage_change(const transaction_id_type& trx_id, const address& contract_id, const string& name, const StorageDataType &diff);
		 void add_contract_event_notify(const transaction_id_type& trx_id, const address& contract_id, const string& event_name, const string& event_arg, uint64_t block_num, uint64_t
		                                op_num);
         void  store_contract_storage_change_obj(const address& contract,uint32_t block_num);
         vector<contract_blocknum_pair> get_contract_changed(uint32_t block_num, uint32_t duration);
         vector<contract_event_notify_object> get_contract_event_notify(const address& contract_id, const transaction_id_type& trx_id, const string& event_name);
         void store_contract(const contract_object& contract);
		 void update_contract(const contract_object& contract);
         contract_object get_contract(const address& contract_address);
         contract_object get_contract(const contract_id_type& id);
         contract_object get_contract(const string& name_or_id);
		 contract_object get_contract_of_name(const string& contract_name);
         vector<contract_object> get_contract_by_owner(const address& owner);

         vector<address> get_contract_address_by_owner(const address& owner);
		 bool has_contract(const address& contract_address, const string& method="");
		 bool has_contract_of_name(const string& contract_name);
         void store_invoke_result(const transaction_id_type& trx_id,int op_num,const contract_invoke_result& res, const share_type& gas);
		 void store_contract_related_transaction(const transaction_id_type&,const address& contract_id);

		 std::vector<transaction_id_type> get_contract_related_transactions(const address& contract_id,uint64_t start,uint64_t end);
         vector<contract_invoke_result_object> get_contract_invoke_result(const transaction_id_type& trx_id)const ;
		 optional<contract_invoke_result_object> get_contract_invoke_result(const transaction_id_type& trx_id,const uint32_t op_num)const;
		 void remove_contract_invoke_result(const transaction_id_type& trx_id, const uint32_t op_num);
         vector<contract_event_notify_object> get_contract_events_by_contract_ordered(const address &addr) const;
		 vector<contract_event_notify_object> get_contract_events_by_block_and_addr_ordered(const address &addr, uint64_t start, uint64_t range) const;
         vector<contract_object> get_registered_contract_according_block(const uint32_t start_with, const uint32_t num)const ;
         void set_min_gas_price(const share_type min_price);
         share_type get_min_gas_price() const;
         //contract_balance//
		 optional<multisig_account_pair_object> get_current_multisig_account(const string& symbol) const;
		 vector<multisig_address_object>      get_multi_account_senator(const string & multi_address, const string& symbol) const;
		 optional<multisig_account_pair_object> get_multisgi_account(const string& multisig_account,const string& symbol) const;
		 vector<guard_member_object> get_guard_members(bool formal = true) const;
         //get account address by account name
         address get_account_address(const string& name) const;
		 string invoke_contract_offline_indb(const string& caller_pubkey, const string& contract_address_or_name, const string& contract_api, const string& contract_arg);
         //////////////////// db_balance.cpp ////////////////////
		 //get lattest multi_asset_objects
		 vector<multisig_address_object> get_multisig_address_list();

		 // get random relate config
		 SecretHashType get_random_padding(bool is_random) ;
		


         /**
          * @brief Retrieve a particular account's balance in a given asset
          * @param owner Account whose balance should be retrieved
          * @param asset_id ID of the asset to get balance in
          * @return owner's balance in asset
          */
         asset get_balance(account_id_type owner, asset_id_type asset_id)const;
         /// This is an overloaded method.
         asset get_balance(const account_object& owner, const asset_object& asset_obj)const;
		 /// this is another overloaded method
		 asset get_balance(const address& addr, const asset_id_type asset_id) const;
	 std::vector<asset> get_contract_balances(const address& addr) const;
         /**
          * @brief Adjust a particular account's balance in a given asset by a delta
          * @param account ID of account whose balance should be adjusted
          * @param delta Asset ID and amount to adjust balance by
          */
         void adjust_balance(account_id_type account, asset delta);
		 /**
		 * @brief Adjust a particular account's balance in a given asset by a delta
		 * @param account ID of account whose balance should be adjusted
		 * @param delta Asset ID and amount to adjust balance by
		 */
		 void adjust_balance(address addr, asset delta, bool freeze = false);

		 /**
		 * @brief Adjust a particular account's balance in a given asset by a delta
		 * @param account ID of account whose balance should be adjusted
		 * @param delta Asset ID and amount to adjust balance by
		 */
		 void adjust_frozen(address addr, asset delta);
		 /**
		 * @brief Adjust a particular account's balance in a given asset by a delta
		 * @param account ID of account whose balance should be adjusted
		 * @param delta Asset ID and amount to adjust balance by
		 */
		 void cancel_frozen(address addr,asset delta);


		 /**
		 * @brief Adjust a particular account's balance in a given asset by a delta
		 * @param account ID of account whose balance should be adjusted
		 * @param delta Asset ID and amount to adjust balance by
		 */

		 void record_guarantee(const guarantee_object_id_type id, const transaction_id_type& target_asset);

		 void adjust_guarantee(const guarantee_object_id_type id, const asset& target_asset);
         /**
          * @brief Helper to make lazy deposit to CDD VBO.
          *
          * If the given optional VBID is not valid(),
          * or it does not have a CDD vesting policy,
          * or the owner / vesting_seconds of the policy
          * does not match the parameter, then credit amount
          * to newly created VBID and return it.
          *
          * Otherwise, credit amount to ovbid.
          * 
          * @return ID of newly created VBO, but only if VBO was created.
          */
         optional< vesting_balance_id_type > deposit_lazy_vesting(
            const optional< vesting_balance_id_type >& ovbid,
            share_type amount,
            uint32_t req_vesting_seconds,
            account_id_type req_owner,
            bool require_vesting );

         // helper to handle cashback rewards
         void deposit_cashback(const account_object& acct, share_type amount, bool require_vesting = true);
         // helper to handle witness pay
         void deposit_miner_pay(const miner_object& wit, share_type amount);
		 unique_ptr<op_evaluator>& get_evaluator(const operation& op);
         //////////////////// db_debug.cpp ////////////////////
		 std::string get_uuid();
         void debug_dump();
         void apply_debug_updates();
         void debug_update( const fc::variant_object& update );

         //////////////////// db_market.cpp ////////////////////

         /// @{ @group Market Helpers
         void globally_settle_asset( const asset_object& bitasset, const price& settle_price );
         void cancel_order(const force_settlement_object& order, bool create_virtual_op = true);
         void cancel_order(const limit_order_object& order, bool create_virtual_op = true);
		 map<account_id_type, vector<asset>> get_citizen_lockbalance_info(const miner_id_type& id) const;
         
         /**
          * @brief Process a new limit order through the markets
          * @param order The new order to process
          * @return true if order was completely filled; false otherwise
          *
          * This function takes a new limit order, and runs the markets attempting to match it with existing orders
          * already on the books.
          */
         bool apply_order(const limit_order_object& new_order_object, bool allow_black_swan = true);

         /**
          * Matches the two orders,
          *
          * @return a bit field indicating which orders were filled (and thus removed)
          *
          * 0 - no orders were matched
          * 1 - bid was filled
          * 2 - ask was filled
          * 3 - both were filled
          */
         ///@{
         template<typename OrderType>
         int match( const limit_order_object& bid, const OrderType& ask, const price& match_price );
         int match( const limit_order_object& bid, const limit_order_object& ask, const price& trade_price );
         /// @return the amount of asset settled
         asset match(const call_order_object& call,
                   const force_settlement_object& settle,
                   const price& match_price,
                   asset max_settlement);
         ///@}

         /**
          * @return true if the order was completely filled and thus freed.
          */
         bool fill_order( const limit_order_object& order, const asset& pays, const asset& receives, bool cull_if_small );
         bool fill_order( const call_order_object& order, const asset& pays, const asset& receives );
         bool fill_order( const force_settlement_object& settle, const asset& pays, const asset& receives );

         bool check_call_orders( const asset_object& mia, bool enable_black_swan = true );

         // helpers to fill_order
         void pay_order( const account_object& receiver, const asset& receives, const asset& pays );

         asset calculate_market_fee(const asset_object& recv_asset, const asset& trade_amount);
         asset pay_market_fees( const asset_object& recv_asset, const asset& receives );


         ///@}
         /**
          *  This method validates transactions without adding it to the pending state.
          *  @return true if the transaction would validate
          */
         processed_transaction validate_transaction( const signed_transaction& trx ,bool testing=false);

		 /*
		 set max limit allowd to produce a block
		 */
		 void set_gas_limit_in_block(const share_type& new_limit);
         /** when popping a block, the transactions that were removed get cached here so they
          * can be reapplied at the proper time */
         std::deque< signed_transaction >       _popped_tx;
		  std::set<transaction_id_type>         _push_transaction_tx_ids;

         /**
          * @}
          */
   protected:
         //Mark pop_undo() as protected -- we do not want outside calling pop_undo(); it should call pop_block() instead
         void pop_undo() { object_database::pop_undo(); }
         void notify_changed_objects();

      private:
         optional<undo_database::session>       _pending_tx_session;
         vector< unique_ptr<op_evaluator> >     _operation_evaluators;
		 
		 //leveldb::DB* l_db = nullptr;;
		 leveldb::Status open_status;
         template<class Index>
         vector<std::reference_wrapper<const typename Index::object_type>> sort_votable_objects(size_t count)const;
		 template<class Index>
		 vector<std::reference_wrapper<const typename Index::object_type>> sort_pledge_objects(fc::uint128_t min_pledge) const;

         //////////////////// db_block.cpp ////////////////////

       public:
         // these were formerly private, but they have a fairly well-defined API, so let's make them public
         void                  apply_block( const signed_block& next_block, uint32_t skip = skip_nothing );
         processed_transaction apply_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing );
         operation_result      apply_operation( transaction_evaluation_state& eval_state, const operation& op );
		 optional<trx_object>   fetch_trx(const transaction_id_type id)const ;
		 void update_all_otc_contract(const uint32_t block_num);
		 Cached_levelDb l_db;
		 leveldb::DB *         get_contract_db()const { return real_l_db; }
		 leveldb::DB *         get_backup_db()const { return backup_l_db; }
		 const Cached_levelDb*          get_cache_contract_db()const { return &l_db; }
		 backup_database                backup_db;
      private:
		  leveldb::DB * real_l_db = nullptr;
		  leveldb::DB * backup_l_db = nullptr;
         void                  _apply_block( const signed_block& next_block );
         processed_transaction _apply_transaction( const signed_transaction& trx ,bool testing=false);
		 void                  _rollback_votes(const proposal_object& proposal);
		 bool                  _need_rollback(const proposal_object& proposal);
         ///Steps involved in applying a new block
         ///@{
		 void                  contract_packed(const signed_transaction& trx, const uint32_t num);

         const miner_object& validate_block_header( uint32_t skip, const signed_block& next_block )const;
         const miner_object& _validate_block_header( const signed_block& next_block )const;
         void create_block_summary(const signed_block& next_block);

         //////////////////// db_update.cpp ////////////////////
         void update_global_dynamic_data( const signed_block& b );
         void update_signing_miner(const miner_object& signing_witness, const signed_block& new_block);
         void update_last_irreversible_block();
         void clear_expired_transactions();
         void clear_expired_proposals();
         void clear_expired_orders();
         void update_expired_feeds();
         void update_maintenance_flag( bool new_maintenance_flag );
         void update_withdraw_permissions();
         bool check_for_blackswan( const asset_object& mia, bool enable_black_swan = true );
		 void update_otc_contract(uint32_t block_num);
		 bool check_contract_type(std::string contract_addr);
		 std::vector<otc_contract_object> get_token_contract_info(std::string contract_addr);
         ///Steps performed only at maintenance intervals
         ///@{
		
         //////////////////// db_maint.cpp ////////////////////

         void initialize_budget_record( fc::time_point_sec now, budget_record& rec )const;
         void process_budget();
		 void process_bonus();
         void pay_workers( share_type& budget );
         void perform_chain_maintenance(const signed_block& next_block, const global_property_object& global_props);
		 void process_name_transfer();
         void update_active_miners();
         void update_active_committee_members();
         void update_worker_votes();
         template<class... Types>
         void perform_account_maintenance(std::tuple<Types...> helpers);
         ///@}
         ///@}

         vector< processed_transaction >        _pending_tx;
         fork_database                          _fork_db;

         /**
          *  Note: we can probably store blocks by block num rather than
          *  block id because after the undo window is past the block ID
          *  is no longer relevant and its number is irreversible.
          *
          *  During the "fork window" we can cache blocks in memory
          *  until the fork is resolved.  This should make maintaining
          *  the fork tree relatively simple.
          */
         block_database   _block_id_to_block;

         /**
          * Contains the set of ops that are in the process of being applied from
          * the current block.  It contains real and virtual operations in the
          * order they occur and is cleared after the applied_block signal is
          * emited.
          */
         vector<optional<operation_history_object> >  _applied_ops;

         uint32_t                          _current_block_num    = 0;
         uint16_t                          _current_trx_in_block = 0;
         uint16_t                          _current_op_in_trx    = 0;
         uint16_t                          _current_virtual_op   = 0;
		 uint16_t						   _current_contract_call_num = 0;
		 SecretHashType                    _current_secret_key   = SecretHashType();

         vector<uint64_t>                  _vote_tally_buffer;
         vector<uint64_t>                  _witness_count_histogram_buffer;
         vector<uint64_t>                  _guard_count_histogram_buffer;
         uint64_t                          _total_voting_stake;
		 share_type						   _total_collected_fee;
		 map<asset_id_type, share_type>     _total_collected_fees;
		 map<asset_id_type, share_type>     _total_fees_pool;
         flat_map<uint32_t,block_id_type>  _checkpoints;

         node_property_object              _node_property_object;

         //gas_price check
		 share_type                        _min_gas_price = 1;
		 share_type						   _gas_limit_in_in_block = 2000000;
		 share_type						   _current_gas_in_block= 0;	 
	public:
		bool ontestnet = false;
		volatile bool stop_process = false;
		bool rewind_on_close = false;
		bool rebuild_mode = false;
		std::mutex                         db_lock;
		bool								sync_mode = false;
		bool								sync_otc_mode = false;
		fc::variant_object				   _network_get_info_data;
   };

   namespace detail
   {
       template<int... Is>
       struct seq { };

       template<int N, int... Is>
       struct gen_seq : gen_seq<N - 1, N - 1, Is...> { };

       template<int... Is>
       struct gen_seq<0, Is...> : seq<Is...> { };

       template<typename T, int... Is>
       void for_each(T&& t, const account_object& a, seq<Is...>)
       {
           auto l = { (std::get<Is>(t)(a), 0)... };
           (void)l;
       }
   }

} }
