#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <list>
#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <fc/git_revision.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <graphene/app/api.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/words.hpp>
#include <graphene/wallet/wallet.hpp>
#include <graphene/wallet/api_documentation.hpp>
#include <graphene/wallet/reflect_util.hpp>
#include <graphene/debug_witness/debug_api.hpp>
#include <fc/smart_ref_impl.hpp>
#include <graphene/crosschain/crosschain.hpp>
#include <graphene/crosschain/crosschain_interface_emu.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/native_contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_object.hpp>
#include <graphene/chain/contract_evaluate.hpp>
#include <graphene/crosschain_privatekey_management/private_key.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/time.hpp>
#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
#endif
#include "graphene/chain/contract_entry.hpp"
#include "boost/filesystem/operations.hpp"
#define BRAIN_KEY_WORD_COUNT 16

namespace graphene { namespace wallet {

namespace detail {

struct operation_result_printer
{
public:
   operation_result_printer( const wallet_api_impl& w )
      : _wallet(w) {}
   const wallet_api_impl& _wallet;
   typedef std::string result_type;

   std::string operator()(const void_result& x) const;
   std::string operator()(const object_id_type& oid);
   std::string operator()(const asset& a);
   std::string operator()(const string& a);
};

// BLOCK  TRX  OP  VOP
struct operation_printer
{
private:
   ostream& out;
   const wallet_api_impl& wallet;
   operation_result result;

   std::string fee(const asset& a) const;

public:
   operation_printer( ostream& out, const wallet_api_impl& wallet, const operation_result& r = operation_result() )
      : out(out),
        wallet(wallet),
        result(r)
   {}
   typedef std::string result_type;

   template<typename T>
   std::string operator()(const T& op)const;

   std::string operator()(const transfer_operation& op)const;
   std::string operator()(const transfer_from_blind_operation& op)const;
   std::string operator()(const transfer_to_blind_operation& op)const;
   std::string operator()(const account_create_operation& op)const;
   std::string operator()(const account_update_operation& op)const;
   std::string operator()(const asset_create_operation& op)const;
};



string address_to_shorthash( const address& addr )
{
   uint32_t x = addr.addr._hash[0];
   static const char hd[] = "0123456789abcdef";
   string result;

   result += hd[(x >> 0x1c) & 0x0f];
   result += hd[(x >> 0x18) & 0x0f];
   result += hd[(x >> 0x14) & 0x0f];
   result += hd[(x >> 0x10) & 0x0f];
   result += hd[(x >> 0x0c) & 0x0f];
   result += hd[(x >> 0x08) & 0x0f];
   result += hd[(x >> 0x04) & 0x0f];
   result += hd[(x        ) & 0x0f];

   return result;
}

fc::ecc::private_key derive_private_key( const std::string& prefix_string,
                                         int sequence_number )
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
   fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
   return derived_key;
}

string normalize_brain_key( string s )
{
   size_t i = 0, n = s.length();
   std::string result;
   char c;
   result.reserve( n );

   bool preceded_by_whitespace = false;
   bool non_empty = false;
   while( i < n )
   {
      c = s[i++];
      switch( c )
      {
      case ' ':  case '\t': case '\r': case '\n': case '\v': case '\f':
         preceded_by_whitespace = true;
         continue;

      case 'a': c = 'A'; break;
      case 'b': c = 'B'; break;
      case 'c': c = 'C'; break;
      case 'd': c = 'D'; break;
      case 'e': c = 'E'; break;
      case 'f': c = 'F'; break;
      case 'g': c = 'G'; break;
      case 'h': c = 'H'; break;
      case 'i': c = 'I'; break;
      case 'j': c = 'J'; break;
      case 'k': c = 'K'; break;
      case 'l': c = 'L'; break;
      case 'm': c = 'M'; break;
      case 'n': c = 'N'; break;
      case 'o': c = 'O'; break;
      case 'p': c = 'P'; break;
      case 'q': c = 'Q'; break;
      case 'r': c = 'R'; break;
      case 's': c = 'S'; break;
      case 't': c = 'T'; break;
      case 'u': c = 'U'; break;
      case 'v': c = 'V'; break;
      case 'w': c = 'W'; break;
      case 'x': c = 'X'; break;
      case 'y': c = 'Y'; break;
      case 'z': c = 'Z'; break;

      default:
         break;
      }
      if( preceded_by_whitespace && non_empty )
         result.push_back(' ');
      result.push_back(c);
      preceded_by_whitespace = false;
      non_empty = true;
   }
   return result;
}

struct op_prototype_visitor
{
   typedef void result_type;

   int t = 0;
   flat_map< std::string, operation >& name2op;

   op_prototype_visitor(
      int _t,
      flat_map< std::string, operation >& _prototype_ops
      ):t(_t), name2op(_prototype_ops) {}

   template<typename Type>
   result_type operator()( const Type& op )const
   {
      string name = fc::get_typename<Type>::name();
      size_t p = name.rfind(':');
      if( p != string::npos )
         name = name.substr( p+1 );
      name2op[ name ] = Type();
   }
};

class wallet_api_impl
{
public:
   api_documentation method_documentation;
private:
   void claim_registered_account(const account_object& account)
   {
      auto it = _wallet.pending_account_registrations.find( account.name );
      FC_ASSERT( it != _wallet.pending_account_registrations.end() );
	  auto acc = get_account(it->first);
	  if (acc.addr == account.addr)
	  {
		  _wallet.update_account(account);
	  }

         //if( !_wallet.update_account( account.name, wif_key ) )
         //{
            // somebody else beat our pending registration, there is
            //    nothing we can do except log it and move on
            //elog( "account ${name} registered by someone else first!",
            //      ("name", account.name) );
            // might as well remove it from pending regs,
            //    because there is now no way this registration
            //    can become valid (even in the extremely rare
            //    possibility of migrating to a fork where the
            //    name is available, the user can always
            //    manually re-register)
         //}
      _wallet.pending_account_registrations.erase( it );
   }
   void claim_account_update(const account_object& obj)
   {
		   _wallet.update_account(obj);
   }
   // after a witness registration succeeds, this saves the private key in the wallet permanently
   //
   void claim_registered_miner(const std::string& witness_name)
   {
      auto iter = _wallet.pending_miner_registrations.find(witness_name);
      FC_ASSERT(iter != _wallet.pending_miner_registrations.end());
      std::string wif_key = iter->second;

      // get the list key id this key is registered with in the chain
      fc::optional<fc::ecc::private_key> witness_private_key = wif_to_key(wif_key);
      FC_ASSERT(witness_private_key);

      auto pub_key = witness_private_key->get_public_key();
      //_keys[pub_key] = wif_key;
	  FC_ASSERT(_keys.count(pub_key));
      _wallet.pending_miner_registrations.erase(iter);
   }

   fc::mutex _resync_mutex;
   void resync()
   {
      fc::scoped_lock<fc::mutex> lock(_resync_mutex);
      // this method is used to update wallet_data annotations
      //   e.g. wallet has been restarted and was not notified
      //   of events while it was down
      //
      // everything that is done "incremental style" when a push
      //   notification is received, should also be done here
      //   "batch style" by querying the blockchain

      if( !_wallet.pending_account_registrations.empty() )
      {
         // make a vector of the account names pending registration
         std::vector<string> pending_account_names = boost::copy_range<std::vector<string> >(boost::adaptors::keys(_wallet.pending_account_registrations));

         // look those up on the blockchain
         std::vector<fc::optional<graphene::chain::account_object >>
               pending_account_objects = _remote_db->lookup_account_names( pending_account_names );

         // if any of them exist, claim them
         for( const fc::optional<graphene::chain::account_object>& optional_account : pending_account_objects )
            if( optional_account )
               claim_registered_account(*optional_account);
      }

      if (!_wallet.pending_miner_registrations.empty())
      {
         // make a vector of the owner accounts for witnesses pending registration
         std::vector<string> pending_witness_names = boost::copy_range<std::vector<string> >(boost::adaptors::keys(_wallet.pending_miner_registrations));

         // look up the owners on the blockchain
         std::vector<fc::optional<graphene::chain::account_object>> owner_account_objects = _remote_db->lookup_account_names(pending_witness_names);

         // if any of them have registered witnechaindatabase_apisses, claim them
         for( const fc::optional<graphene::chain::account_object>& optional_account : owner_account_objects )
            if (optional_account)
            {
               fc::optional<miner_object> witness_obj = _remote_db->get_miner_by_account(optional_account->id);
               if (witness_obj)
				   claim_registered_miner(optional_account->name);
            }
      }

	  if (!_wallet.pending_account_updation.empty())
	  {
		  // make a vector of the owner accounts for update pending registration

		  std::vector<transaction_id_type> pending_trx_ids = boost::copy_range<std::vector<transaction_id_type> >(boost::adaptors::keys(_wallet.pending_account_updation));

		  for (auto id : pending_trx_ids)
		  {
			  //std::cout << "Try get trx" << fc::json::to_string(id)<<std::endl;
			  try {
				  auto trx = _remote_db->get_transaction_by_id(id);
			 
			  if (trx.valid())
			  {
				  auto acct=_remote_db->get_account(_wallet.pending_account_updation[id]);
				  claim_account_update(acct);
				  _wallet.pending_account_updation.erase(id);
			  }
			  }catch(...)
			  {}
		  }
	  }
      if (_wallet.event_handlers.size() != 0)
      {
          auto& idx = _wallet.event_handlers.get<by_id>();
          for (auto& it : idx)
          {
              optional<script_object> script_obj=_wallet.get_script_by_hash(it.script_hash);
              if(!script_obj.valid())
                  continue;
              ::blockchain::contract_engine::ContractEngineBuilder builder;
              if (!uvm::lua::api::global_uvm_chain_api)
                  uvm::lua::api::global_uvm_chain_api = new UvmChainApi();
              auto engine = builder.build();
              int exception_code = 0;
              auto code_stream = engine->get_bytestream_from_code(script_obj->script);
              if (!code_stream)
                  continue;
              vector<pair<object_id_type, contract_event_notify_object>> new_handled;
              vector<pair<object_id_type, contract_event_notify_object>> undoed;
              auto last_handled=it.handled.rbegin();
              bool undo_failed = false;
              while (last_handled != it.handled.rend())
              {
                  if (_remote_db->get_contract_event_notify_by_id(last_handled->first).valid())
                  {
                      break;
                  }
                  try {
                      undoed.push_back(std::make_pair(last_handled->first, last_handled->second));
                      engine->clear_exceptions();
                      engine->add_global_string_variable("event_type", last_handled->second.event_name.c_str());
                      engine->add_global_string_variable("param", last_handled->second.event_arg.c_str());
                      engine->add_global_string_variable("contract_id", last_handled->second.contract_address.operator fc::string());
                      engine->add_global_bool_variable("undo", true);
                      engine->add_global_int_variable("block_num", last_handled->second.block_num);
                      engine->load_and_run_stream(code_stream.get());
                  }
                  catch (...)
                  {
                      undo_failed = true;
                      break;
                  }
                  
                  last_handled++;
              }
              _wallet.update_handler(it.id, undoed, false);
              if(undo_failed)
                  continue;
              vector<contract_event_notify_object> events=_remote_db->get_contract_event_notify(it.contract_id,transaction_id_type(),it.event_name);
              for (auto ev : events)
              {
                  if (ev.id > last_handled->first)
                  {
                      try {
                          //todo: execute
                          engine->clear_exceptions();
                          engine->add_global_string_variable("event_type", ev.event_name.c_str());
                          engine->add_global_string_variable("param", ev.event_arg.c_str());
                          engine->add_global_string_variable("contract_id", ev.contract_address.operator fc::string());
                          engine->add_global_bool_variable("undo", false);

                          engine->load_and_run_stream(code_stream.get());

                          new_handled.push_back(std::make_pair(ev.id, ev));
                      }
                      catch (...)
                      {
                          break;
                      }

                  }
              }
              _wallet.update_handler(it.id, new_handled, true);

          }
          save_wallet_file();
      }
   }
   void enable_umask_protection()
   {
#ifdef __unix__
      _old_umask = umask( S_IRWXG | S_IRWXO );
#endif
   }

   void disable_umask_protection()
   {
#ifdef __unix__
      umask( _old_umask );
#endif
   }

   void init_prototype_ops()
   {
      operation op;
      for( int t=0; t<op.count(); t++ )
      {
         op.set_which( t );
         op.visit( op_prototype_visitor(t, _prototype_ops) );
      }
      return;
   }

   map<transaction_handle_type, signed_transaction> _builder_transactions;

   // if the user executes the same command twice in quick succession,
   // we might generate the same transaction id, and cause the second
   // transaction to be rejected.  This can be avoided by altering the
   // second transaction slightly (bumping up the expiration time by
   // a second).  Keep track of recent transaction ids we've generated
   // so we can know if we need to do this
   struct recently_generated_transaction_record
   {
      fc::time_point_sec generation_time;
      graphene::chain::transaction_id_type transaction_id;
   };
   struct timestamp_index{};
   typedef boost::multi_index_container<recently_generated_transaction_record,
                                        boost::multi_index::indexed_by<boost::multi_index::hashed_unique<boost::multi_index::member<recently_generated_transaction_record,
                                                                                                                                    graphene::chain::transaction_id_type,
                                                                                                                                    &recently_generated_transaction_record::transaction_id>,
                                                                                                         std::hash<graphene::chain::transaction_id_type> >,
                                                                       boost::multi_index::ordered_non_unique<boost::multi_index::tag<timestamp_index>,
                                                                                                              boost::multi_index::member<recently_generated_transaction_record, fc::time_point_sec, &recently_generated_transaction_record::generation_time> > > > recently_generated_transaction_set_type;
   recently_generated_transaction_set_type _recently_generated_transactions;

public:
   wallet_api& self;
   wallet_api_impl( wallet_api& s, const wallet_data& initial_data, fc::api<login_api> rapi )
      : self(s),
        _chain_id(initial_data.chain_id),
        _remote_api(rapi),
        _remote_db(rapi->database()),
        _remote_net_broadcast(rapi->network_broadcast()),
        _remote_hist(rapi->history()),
	   _crosschain_manager(rapi->crosschain_config()),
	   _guarantee_id(optional<guarantee_object_id_type>()),
	   _remote_trx(rapi->transaction()),
	   _remote_local_node(rapi->localnode())
   {
      chain_id_type remote_chain_id = _remote_db->get_chain_id();
      if( remote_chain_id != _chain_id )
      {
         FC_THROW( "Remote server gave us an unexpected chain_id",
            ("remote_chain_id", remote_chain_id)
            ("chain_id", _chain_id) );
      }
      init_prototype_ops();

     /* _remote_db->set_block_applied_callback( [this](const variant& block_id )
      {
         on_block_applied( block_id );
      } );*/

      _wallet.chain_id = _chain_id;
      _wallet.ws_server = initial_data.ws_server;
      _wallet.ws_user = initial_data.ws_user;
      _wallet.ws_password = initial_data.ws_password;
	  schedule_loop();
   }
   virtual ~wallet_api_impl()
   {
      try
      {
         _remote_db->cancel_all_subscriptions();
      }
      catch (const fc::exception& e)
      {
         // Right now the wallet_api has no way of knowing if the connection to the
         // witness has already disconnected (via the witness node exiting first).
         // If it has exited, cancel_all_subscriptsions() will throw and there's
         // nothing we can do about it.
         // dlog("Caught exception ${e} while canceling database subscriptions", ("e", e));
      }
   }

   void schedule_loop()
   {
	   //std::cout << "schedule_loop" << std::endl;
	   fc::time_point next_wakeup(fc::time_point::now() + fc::seconds(5));
	   try {
		   static uint32_t block_num = 0;
		   auto block_interval = _remote_db->get_global_properties().parameters.block_interval;
		   fc::time_point now = fc::time_point::now();
		   next_wakeup=fc::time_point(now + fc::seconds(block_interval));
		   auto height = _remote_db->get_dynamic_global_properties().head_block_number;
		   if (height != block_num)
		   {
			   block_num = height;
			   resync();
		   }
	   }
	   catch (const fc::exception& e)
	   {
		   //std::cout << e.to_detail_string()<< std::endl;
	   }
	   fc::schedule([this] {schedule_loop(); }, next_wakeup, "Resync From The Node", fc::priority::max());

	   //std::cout << "schedule_loop  end" << std::endl;
   }
   void encrypt_keys()
   {
      if( !is_locked() )
      {
         plain_keys data;
         data.keys = _keys;
		 data.crosschain_keys = _crosschain_keys;
         data.checksum = _checksum;
		 if (_current_brain_key.valid())
			 _wallet.cipher_keys_extend = fc::aes_encrypt(_checksum,fc::raw::pack(*_current_brain_key));
         auto plain_txt = fc::raw::pack(data);
         _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
      }
   }

   void on_block_applied( const variant& block_id )
   {
      fc::async([this]{resync();}, "Resync after block");
   }

   bool copy_wallet_file( string destination_filename )
   {
      fc::path src_path = get_wallet_filename();
      if( !fc::exists( src_path ) )
         return false;
      fc::path dest_path = destination_filename + _wallet_filename_extension;
      int suffix = 0;
      while( fc::exists(dest_path) )
      {
         ++suffix;
         dest_path = destination_filename + "-" + to_string( suffix ) + _wallet_filename_extension;
      }
      wlog( "backing up wallet ${src} to ${dest}",
            ("src", src_path)
            ("dest", dest_path) );

      fc::path dest_parent = fc::absolute(dest_path).parent_path();
      try
      {
         enable_umask_protection();
         if( !fc::exists( dest_parent ) )
            fc::create_directories( dest_parent );
         fc::copy( src_path, dest_path );
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
      return true;
   }

   bool is_locked()const
   {
      return _checksum == fc::sha512();
   }

   template<typename T>
   T get_object(object_id<T::space_id, T::type_id, T> id)const
   {
      auto ob = _remote_db->get_objects({id}).front();
      return ob.template as<T>();
   }

   void set_operation_fees( signed_transaction& tx, const fee_schedule& s  )
   {
      for( auto& op : tx.operations )
         s.set_fee(op);
   }

   variant info() const
   {
      auto chain_props = get_chain_properties();
      auto global_props = get_global_properties();
      auto dynamic_props = get_dynamic_global_properties();
      fc::mutable_variant_object result;
      result["head_block_num"] = dynamic_props.head_block_number;
      result["head_block_id"] = dynamic_props.head_block_id;
      result["head_block_age"] = fc::get_approximate_relative_time_string(dynamic_props.time,
                                                                          time_point_sec(time_point::now()),
                                                                          " old");
	  result["version"] = "1.2.8";
      result["next_maintenance_time"] = fc::get_approximate_relative_time_string(dynamic_props.next_maintenance_time);
      result["chain_id"] = chain_props.chain_id;
	  //result["data_dir"] = (*_remote_local_node)->get_data_dir();
	  
      result["participation"] = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
	  result["round_participation"] =100.0 * dynamic_props.round_produced_miners.size() / (GRAPHENE_PRODUCT_PER_ROUND *1.0);
	  auto scheduled_citizens = _remote_db->list_scheduled_citizens();
	  result["scheduled_citizens"] = scheduled_citizens;
      //result["active_guard_members"] = global_props.active_committee_members;
      return result;
   }

   variant_object about() const
   {
      string client_version( graphene::utilities::git_revision_description );
      const size_t pos = client_version.find( '/' );
      if( pos != string::npos && client_version.size() > pos )
         client_version = client_version.substr( pos + 1 );

      fc::mutable_variant_object result;
      //result["blockchain_name"]        = BLOCKCHAIN_NAME;
      //result["blockchain_description"] = LNK_BLOCKCHAIN_DESCRIPTION;
      result["client_version"]           = client_version;
      result["graphene_revision"]        = graphene::utilities::git_revision_sha;
      result["graphene_revision_age"]    = fc::get_approximate_relative_time_string( fc::time_point_sec( graphene::utilities::git_revision_unix_timestamp ) );
      result["fc_revision"]              = fc::git_revision_sha;
      result["fc_revision_age"]          = fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) );
      result["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
      result["boost_version"]            = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
      result["openssl_version"]          = OPENSSL_VERSION_TEXT;

      std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
      std::string os = "osx";
#elif defined(__linux__)
      std::string os = "linux";
#elif defined(_MSC_VER)
      std::string os = "win32";
#else
      std::string os = "other";
#endif
      result["build"] = os + " " + bitness;

      return result;
   }

   chain_property_object get_chain_properties() const
   {
      return _remote_db->get_chain_properties();
   }
   global_property_object get_global_properties() const
   {
      return _remote_db->get_global_properties();
   }
   dynamic_global_property_object get_dynamic_global_properties() const
   {
      return _remote_db->get_dynamic_global_properties();
   }
   account_object get_account(account_id_type id) const
   {
      if( _wallet.my_accounts.get<by_id>().count(id) )
         return *_wallet.my_accounts.get<by_id>().find(id);
      auto rec = _remote_db->get_accounts({id}).front();
      FC_ASSERT(rec);
      return *rec;
   }

   account_object get_account(address addr) const
   {
	   if (_wallet.my_accounts.get<by_address>().count(addr))
		   return *_wallet.my_accounts.get<by_address>().find(addr);
	   auto rec = _remote_db->get_accounts_addr({addr}).front();
	   FC_ASSERT(rec);
	   return *rec;
   }
   optional<account_object> get_account_by_addr(const address& addr) const 
   {
	   auto rec = _remote_db->get_accounts_addr({ addr });
	   if (rec.size())
	   {
		   auto acc = rec.front();
		   if (acc.valid())
			   return *acc;
	   }
	   return optional<account_object>();
   }


   account_object change_account_name(const string& oldname, const string& newname) 
   {
	   FC_ASSERT(is_valid_account_name(newname),"not a correct account name.");
	   int local_account_count = _wallet.my_accounts.get<by_name>().count(oldname);
	   FC_ASSERT((local_account_count != 0), "This account dosen`t belong to local wallet");
	   auto local_account = *_wallet.my_accounts.get<by_name>().find(oldname);
	   auto blockchain_account = _remote_db->lookup_account_names({ oldname }).front();
	   if (blockchain_account)
	   {
		   FC_ASSERT((local_account.addr != blockchain_account->addr), "This account has been registed");
	   }
	   int local_new_account_count = _wallet.my_accounts.get<by_name>().count(newname);
	   FC_ASSERT((local_new_account_count == 0), "New name has been used in local wallet");
	   auto blockchain_new_account = _remote_db->lookup_account_names({ newname }).front();
	   FC_ASSERT((!blockchain_new_account),"New name has been used in blockchain");
	   local_account.name = newname;
	   _wallet.updata_account_name(local_account,oldname);
	   return *_wallet.my_accounts.get<by_name>().find(newname);
   }
   void remove_local_account(const string& account_name) {
	   int local_account_count = _wallet.my_accounts.get<by_name>().count(account_name);
	   FC_ASSERT((local_account_count != 0), "This account dosen`t belong to local wallet");
	   auto& iter_db = _wallet.my_accounts.get<by_name>();
	   auto iter_remove = iter_db.find(account_name);
	   if (iter_remove != iter_db.end()) {
		   iter_db.erase(iter_remove);
	   }
	   save_wallet_file();
   }
   account_object get_account(string account_name_or_id) const
   {
      FC_ASSERT( account_name_or_id.size() > 0 );

      if( auto id = maybe_id<account_id_type>(account_name_or_id) )
      {
         // It's an ID
         return get_account(*id);
      } else {
         // It's a name
         if( _wallet.my_accounts.get<by_name>().count(account_name_or_id) )
         {
            auto local_account = *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
            auto blockchain_account = _remote_db->lookup_account_names({account_name_or_id}).front();
			if (blockchain_account)
			{
				if (local_account.id != blockchain_account->id)
					elog("my account id ${id} different from blockchain id ${id2}", ("id", local_account.id)("id2", blockchain_account->id));
				if (local_account.name != blockchain_account->name)
					elog("my account name ${id} different from blockchain name ${id2}", ("id", local_account.name)("id2", blockchain_account->name));
			}
            return *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
         }
         auto rec = _remote_db->lookup_account_names({account_name_or_id}).front();
         if( rec && rec->name == account_name_or_id )
			 return *rec;
      }
	  return account_object();
   }

   vector<optional<guarantee_object>> get_my_guarantee_order(const string& account, bool all)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   auto& iter = _wallet.my_accounts.get<by_name>();
		   FC_ASSERT(iter.find(account) != iter.end(), "Could not find account name ${account}", ("account", account));
		   return _remote_db->get_guarantee_orders(iter.find(account)->addr, all);

	   }FC_CAPTURE_AND_RETHROW((account)(all))
   }

   vector<optional<guarantee_object>> list_guarantee_order(const string& chain_type,bool all)
   {
	   try {
		   auto asset_obj = get_asset(chain_type);
		   return _remote_db->list_guarantee_object(chain_type,all);

	   }FC_CAPTURE_AND_RETHROW((chain_type))
   }

   fc::variant get_transaction(transaction_id_type id) const
   {
	   try {
		   auto trx = _remote_trx->get_transaction(id);
		   if (!trx.valid())
			   return fc::variant();
		   fc::variant trx_var(*trx);
		   fc::mutable_variant_object obj = trx_var.as<fc::mutable_variant_object>();
		   int index = 0;
		   for (auto& op : trx->operations)
		   {
			   auto op_obj = obj["operations"].get_array()[index].get_array()[1].as<fc::mutable_variant_object>();
			   if (op.which() == operation::tag<transfer_operation>().value)
			   {
				   std::stringstream ss;
				   auto memo = op.visit(detail::operation_printer(ss,*this));
				   /*
				   auto& transfer_op = op.get<transfer_operation>();
				   if (memo.size() == 0)
					   break;
				   transfer_op.memo->message.clear();
				   transfer_op.memo->message.assign(memo.begin(),memo.end());
				   */
				   if (memo.size() == 0)
					   continue;
				   auto temp = op_obj["memo"].as<fc::mutable_variant_object>().set("message",memo);
				   auto op_temp =op_obj.set("memo",temp);
				   obj["operations"].get_array()[index].get_array()[1] = op_temp;
			   }
			   index++;
		   }
		   return obj;
	   }FC_CAPTURE_AND_RETHROW((id))
   }

   vector<transaction_id_type> list_transactions(uint32_t blocknum , uint32_t nums ) const
   {
	   try {

		   auto result = _remote_trx->list_transactions(blocknum,nums);
		   return result;

	   }FC_CAPTURE_AND_RETHROW()
   }

   optional<guarantee_object> get_guarantee_order(const guarantee_object_id_type id)
   {
	   return _remote_db->get_gurantee_object(id);
   }

   void set_guarantee_id(const guarantee_object_id_type id)
   {
	   try {
		   _guarantee_id = id;
	   }FC_CAPTURE_AND_RETHROW((id))
   }
   void remove_guarantee_id()
   {
	   _guarantee_id = optional<guarantee_object_id_type>();
   }
   optional<guarantee_object_id_type> get_guarantee_id()
   {
	   auto id = _guarantee_id;
	   _guarantee_id = optional<guarantee_object_id_type>();
	   return id;
   }
   string derive_wif_key(string brain_key, int index, const string& symbol="HX")
   {

	   auto bk = graphene::wallet::detail::normalize_brain_key(brain_key);
	   fc::ecc::private_key priv_key = graphene::wallet::detail::derive_private_key(bk, index);
	   if (symbol == "HX")
	   {
		   return  key_to_wif(priv_key);
	   }
	   auto pos = symbol.find("|etguard");
	   auto real_symbol = symbol;
	   if (pos != symbol.npos) {
		   real_symbol = symbol.substr(0, pos);
	   }
	   string config = (*_crosschain_manager)->get_config();
	   FC_ASSERT((*_crosschain_manager)->contain_symbol(real_symbol), "no this plugin");
	   auto prk = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(real_symbol);
	   prk->generate(priv_key);
	   return prk->get_wif_key();

   }
   crosschain_prkeys create_crosschain_symbol(const string& symbol, bool cold=false,bool use_brain_key=false)
   {
	   try {
		   auto pos = symbol.find("|etguard");
		   auto real_symbol = symbol;
		   bool bGuard = false;
		   if (pos != symbol.npos) {
			   real_symbol = symbol.substr(0, pos);
			   bGuard = true;
		   }
		   FC_ASSERT(!self.is_locked());
		   string config = (*_crosschain_manager)->get_config();
		   FC_ASSERT((*_crosschain_manager)->contain_symbol(real_symbol), "no this plugin");
		   auto& instance = graphene::crosschain::crosschain_manager::get_instance();
		   auto fd = instance.get_crosschain_handle(real_symbol);
		   fd->initialize_config(fc::json::from_string(config).get_object());
		   std::string wif_key;
		   fc::scoped_lock<fc::mutex> lock();
		   optional<fc::ecc::private_key> key= optional<fc::ecc::private_key>();
		   if (use_brain_key)
		   {
			   FC_ASSERT(this->_current_brain_key.valid(), "brain_key not set");
			   auto brain_key = graphene::wallet::detail::normalize_brain_key(_current_brain_key->key);
			   fc::ecc::private_key priv_key = graphene::wallet::detail::derive_private_key(brain_key, _current_brain_key->next);
			   key = priv_key;
		   }
		   if (bGuard){
			   wif_key = fd->create_normal_account("guard",key);
		   }
		   else {
			   wif_key = fd->create_normal_account("",key);
		   }
		   FC_ASSERT(wif_key != "");
		   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(real_symbol);
		   auto pk = prk_ptr->import_private_key(wif_key);
		   FC_ASSERT(pk.valid());
		   prk_ptr->set_key(*pk);

		   auto addr = prk_ptr->get_address();
		   auto pubkey = prk_ptr->get_public_key();
		   crosschain_prkeys keys;
		   keys.addr = addr;
		   keys.pubkey = pubkey;
		   keys.wif_key = wif_key;
		   if (!cold)
		   {
			   _crosschain_keys[addr] = keys;
		   }
		   if (use_brain_key)
		   {
			   _current_brain_key->used_indexes[addr]=_current_brain_key->next++;
		   }
		   save_wallet_file();
		   return keys;
	   }FC_CAPTURE_AND_RETHROW((symbol))

   }

   crosschain_prkeys wallet_create_crosschain_symbol(const string& symbol,bool use_brain_key=false)
   {
	   try {
		   FC_ASSERT(!self.is_locked());
		   fc::scoped_lock<fc::mutex> lock(brain_key_index_lock);
		   auto ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(symbol);
		   FC_ASSERT(ptr != nullptr ,"plugin doesnt exist.");
		   if(!use_brain_key)
			   ptr->generate();
		   else
		   {
			   FC_ASSERT(this->_current_brain_key.valid(), "brain_key not set");
			   auto brain_key = graphene::wallet::detail::normalize_brain_key(_current_brain_key->key);
			   fc::ecc::private_key priv_key = graphene::wallet::detail::derive_private_key(brain_key, _current_brain_key->next);
			   ptr->generate(priv_key);
			   _current_brain_key->next++;
		   }
		   auto addr = ptr->get_address();
		   if (use_brain_key)
		   {
			   _current_brain_key->used_indexes[addr] = _current_brain_key->next - 1;
		   }
		   auto pubkey = ptr->get_public_key();
		   crosschain_prkeys keys;
		   keys.addr = addr;
		   keys.pubkey = pubkey;
		   keys.wif_key = ptr->get_wif_key();
		   _crosschain_keys[addr] = keys;
		   save_wallet_file();
		   return keys;
	   }FC_CAPTURE_AND_RETHROW((symbol))
	   
   }

   account_id_type get_account_id(string account_name_or_id) const
   {
      return get_account(account_name_or_id).get_id();
   }
   address get_account_addr(string account_name) const
   {
	   auto result = get_account(account_name).addr;
	   FC_ASSERT(result != address(),"wallet doesnt have this account!");
	   return result;
   }

   optional<asset_object> find_asset(asset_id_type id)const
   {
      auto rec = _remote_db->get_assets({id}).front();
      if( rec )
         _asset_cache[id] = *rec;
      return rec;
   }
   optional<asset_object> find_asset(string asset_symbol_or_id)const
   {
      FC_ASSERT( asset_symbol_or_id.size() > 0 );

      if( auto id = maybe_id<asset_id_type>(asset_symbol_or_id) )
      {
         // It's an ID
         return find_asset(*id);
      } else {
         // It's a symbol
         auto rec = _remote_db->lookup_asset_symbols({asset_symbol_or_id}).front();
         if( rec )
         {
            if( rec->symbol != asset_symbol_or_id )
               return optional<asset_object>();

            _asset_cache[rec->get_id()] = *rec;
         }
         return rec;
      }
   }
   asset_object get_asset(asset_id_type id)const
   {
      auto opt = find_asset(id);
      FC_ASSERT(opt);
      return *opt;
   }
   asset_object get_asset(string asset_symbol_or_id)const
   {
      auto opt = find_asset(asset_symbol_or_id);
      FC_ASSERT(opt);
      return *opt;
   }

   asset_id_type get_asset_id(string asset_symbol_or_id) const
   {
      FC_ASSERT( asset_symbol_or_id.size() > 0 );
      vector<optional<asset_object>> opt_asset;
      if( std::isdigit( asset_symbol_or_id.front() ) )
         return fc::variant(asset_symbol_or_id).as<asset_id_type>();
      opt_asset = _remote_db->lookup_asset_symbols( {asset_symbol_or_id} );
      FC_ASSERT( (opt_asset.size() > 0) && (opt_asset[0].valid()) );
      return opt_asset[0]->id;
   }

   string                            get_wallet_filename() const
   {
      return _wallet_filename;
   }

   fc::ecc::private_key              get_private_key(const public_key_type& id)const
   {
      auto it = _keys.find(id);
      FC_ASSERT( it != _keys.end() );

      fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
      FC_ASSERT( privkey );
      return *privkey;
   }

   fc::ecc::private_key get_private_key_for_account(const account_object& account)const
   {
      vector<public_key_type> active_keys = account.active.get_keys();
      if (active_keys.size() != 1)
         FC_THROW("Expecting a simple authority with one active key");
      return get_private_key(active_keys.front());
   }

   // imports the private key into the wallet, and associate it in some way (?) with the
   // given account name.
   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is stored either way)
   bool import_key(string account_name_or_id, string wif_key)
   {
      fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
      if (!optional_private_key)
         FC_THROW("Invalid private key");
      graphene::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();
	  account_object account;
      account_object temp = get_account( account_name_or_id );
	  if (temp.addr != address())
	  {
		  if (temp.addr != address(wif_pub_key))
		  {
			  if (_wallet.my_accounts.get<by_address>().count(account.addr) > 0)
				  FC_THROW("there is same account in this wallet.");
		  }
		  else
		  {
			  account = temp;
		  }
	  }
		  
	  string addr = account.addr.operator fc::string();
      flat_set<public_key_type> all_keys_for_account;
      std::vector<public_key_type> active_keys = account.active.get_keys();
      std::vector<public_key_type> owner_keys = account.owner.get_keys();
      std::copy(active_keys.begin(), active_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
      std::copy(owner_keys.begin(), owner_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
	 

      _keys[wif_pub_key] = wif_key;
	  account.addr = address(wif_pub_key);
	  account.name = account_name_or_id;
	  vector<address> vec;
	  vec.push_back(account.addr);
	  auto vec_acc = _remote_db->get_accounts_addr(vec);
	  for (auto op : vec_acc)
	  {
		  if (op.valid())
		  {
			  if (fc::json::to_string(account) != fc::json::to_string(*op))
			  {
				  wlog("Account ${id} : \"${name}\" updated on chain", ("id", account.get_id())("name", account.name));
			  }
			  account = *op;
		  } 
	  }
      _wallet.update_account(account);
      _wallet.extra_keys[account.id].insert(wif_pub_key);
	  if (account.options.memo_key == public_key_type())
		  return true;
	  all_keys_for_account.insert(account.options.memo_key);
      return all_keys_for_account.find(wif_pub_key) != all_keys_for_account.end();
   }

   bool import_crosschain_key(string wif_key, string symbol)
   {
	   auto ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(symbol);
	   FC_ASSERT(ptr != nullptr, "plugin doesnt exist.");
	   ptr->import_private_key(wif_key);
	   if (ptr->get_private_key() == fc::ecc::private_key())
		   return false;
	   crosschain_prkeys key;
	   auto addr = ptr->get_address();
	   key.addr = addr;
	   key.pubkey = ptr->get_public_key();
	   key.wif_key = ptr->get_wif_key();
	   _crosschain_keys[addr] = key;
	   return true;
   }

   string sign_multisig_trx(const address& addr, const signed_transaction& trx)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   auto itr = _keys.find(addr);
		   FC_ASSERT(itr != _keys.end());
		   auto privkey = wif_to_key(itr->second);
		   FC_ASSERT(privkey.valid());
		   signed_transaction tx = trx;
		   tx.sign(*privkey, _chain_id);
		   auto json_str = fc::json::to_string(tx);
		   auto base_str = fc::to_base58(json_str.c_str(), json_str.size());
		   return base_str;
	   }FC_CAPTURE_AND_RETHROW((addr)(trx))
   }

   vector< signed_transaction > import_balance( string name_or_id, const vector<string>& wif_keys, bool broadcast );

   bool load_wallet_file(string wallet_filename = "")
   {
	   // TODO:  Merge imported wallet with existing wallet,
	   //        instead of replacing it
	   if (wallet_filename == "")
		   wallet_filename = _wallet_filename;

	   if (!fc::exists(wallet_filename))
		   return false;

	   _wallet = fc::json::from_file(wallet_filename).as< wallet_data >();
	   if (_wallet.chain_id != _chain_id)
		   FC_THROW("Wallet chain ID does not match",
		   ("wallet.chain_id", _wallet.chain_id)
			   ("chain_id", _chain_id));

	   size_t account_pagination = 100;
	   vector< address > account_address_to_send;
	   size_t n = _wallet.my_accounts.size();
	   account_address_to_send.reserve(std::min(account_pagination, n));
	   auto it = _wallet.my_accounts.begin();

	   for (size_t start = 0; start < n; start += account_pagination)
	   {
		   size_t end = std::min(start + account_pagination, n);
		   assert(end > start);
		   account_address_to_send.clear();
		   std::vector< account_object > old_accounts;
		   for (size_t i = start; i < end; i++)
		   {
			   assert(it != _wallet.my_accounts.end());
			   old_accounts.push_back(*it);
			   account_address_to_send.push_back(old_accounts.back().addr);
			   ++it;
		   }
		   std::vector< optional< account_object > > accounts = _remote_db->get_accounts_addr(account_address_to_send);
		   // server response should be same length as request
		   FC_ASSERT(accounts.size() == account_address_to_send.size());
		   size_t i = 0;
		   for (optional< account_object >& acct : accounts)
		   {
			   account_object& old_acct = old_accounts[i];
			   if (!acct.valid())
			   {
				   wlog("Could not find account ${id} : \"${name}\" does not exist on the chain!", ("id", old_acct.id)("name", old_acct.name));
				   i++;
				   continue;
			   }
			   // this check makes sure the server didn't send results
			   // in a different order, or accounts we didn't request
			   //FC_ASSERT( acct->id == old_acct.id );
		   /*	acct->addr = old_acct.addr;
			   if (acct->id == old_acct.id  || acct->addr == old_acct.addr)
			   {
				   acct->addr = old_acct.addr;
			   }*/
			   if (fc::json::to_string(*acct) != fc::json::to_string(old_acct))
			   {
				   wlog("Account ${id} : \"${name}\" updated on chain", ("id", acct->id)("name", acct->name));
			   }
			   _wallet.update_account(*acct);
			   i++;
		   }
	   }

	   return true;
   }
   bool check_keys_modified(string wallet_filename = "")
   {
	   // TODO:  Merge imported wallet with existing wallet,
	   //        instead of replacing it
	   if (wallet_filename == "")
		   wallet_filename = _wallet_filename;

	   if (!fc::exists(wallet_filename))
		   return false;

	   wallet_data tmp = fc::json::from_file(wallet_filename).as< wallet_data >();
	   if (tmp.chain_id != _chain_id)
		   false;
	   if (tmp.cipher_keys != _wallet.cipher_keys)
	   {
		   std::cout << fc::json::to_string(tmp.cipher_keys) << std::endl<< fc::json::to_string(_wallet.cipher_keys) <<std::endl;
		   return true;
	   }
		
	   //if (tmp.my_accounts != _wallet.my_accounts)
	//	   return true;
	   return false;
   }
   fc::mutex save_mutex;
   void save_wallet_file(string wallet_filename = "")
   {
       fc::scoped_lock<fc::mutex> lock(save_mutex);

      //
      // Serialize in memory, then save to disk
      //
      // This approach lessens the risk of a partially written wallet
      // if exceptions are thrown in serialization
      //

      

      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;
	  if (check_keys_modified(wallet_filename))
	  {
		  boost::filesystem::copy_file(wallet_filename, wallet_filename+"_backupAccordingKeychk@"+fc::json::to_string(fc::time_point::now().sec_since_epoch()));
	  }
	  encrypt_keys();
      wlog( "saving wallet to file ${fn}", ("fn", wallet_filename) );

      string data = fc::json::to_pretty_string( _wallet );
      try
      {
         enable_umask_protection();
         //
         // Parentheses on the following declaration fails to compile,
         // due to the Most Vexing Parse.  Thanks, C++
         //
         // http://en.wikipedia.org/wiki/Most_vexing_parse
         //
         fc::ofstream outfile{ fc::path( wallet_filename ) };
         outfile.write( data.c_str(), data.length() );
         outfile.flush();
         outfile.close();
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
   }

   transaction_handle_type begin_builder_transaction()
   {
      int trx_handle = _builder_transactions.empty()? 0
                                                    : (--_builder_transactions.end())->first + 1;
      _builder_transactions[trx_handle];
      return trx_handle;
   }
   void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
   {
      FC_ASSERT(_builder_transactions.count(transaction_handle));
      _builder_transactions[transaction_handle].operations.emplace_back(op);
   }
   void replace_operation_in_builder_transaction(transaction_handle_type handle,
                                                 uint32_t operation_index,
                                                 const operation& new_op)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      signed_transaction& trx = _builder_transactions[handle];
      FC_ASSERT( operation_index < trx.operations.size());
      trx.operations[operation_index] = new_op;
   }
   asset set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset = GRAPHENE_SYMBOL)
   {
      FC_ASSERT(_builder_transactions.count(handle));

      auto fee_asset_obj = get_asset(fee_asset);
      asset total_fee = fee_asset_obj.amount(0);

      auto gprops = _remote_db->get_global_properties().parameters;
      if( fee_asset_obj.get_id() != asset_id_type() )
      {
         for( auto& op : _builder_transactions[handle].operations )
            total_fee += gprops.current_fees->set_fee( op, fee_asset_obj.options.core_exchange_rate );

         FC_ASSERT((total_fee * fee_asset_obj.options.core_exchange_rate).amount <=
                   get_object<asset_dynamic_data_object>(fee_asset_obj.dynamic_asset_data_id).fee_pool,
                   "Cannot pay fees in ${asset}, as this asset's fee pool is insufficiently funded.",
                   ("asset", fee_asset_obj.symbol));
      } else {
         for( auto& op : _builder_transactions[handle].operations )
            total_fee += gprops.current_fees->set_fee( op );
      }

      return total_fee;
   }
   
   transaction preview_builder_transaction(transaction_handle_type handle)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      return _builder_transactions[handle];
   }
   full_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(transaction_handle));

      return _builder_transactions[transaction_handle] = sign_transaction(_builder_transactions[transaction_handle], broadcast);
   }
   full_transaction propose_builder_transaction(
      transaction_handle_type handle,
      time_point_sec expiration = time_point::now() + fc::minutes(1),
      uint32_t review_period_seconds = 0, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      proposal_create_operation op;
      op.expiration_time = expiration;
      signed_transaction& trx = _builder_transactions[handle];
      std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
                     [](const operation& op) -> op_wrapper { return op; });
      if( review_period_seconds )
         op.review_period_seconds = review_period_seconds;
      trx.operations = {op};
      _remote_db->get_global_properties().parameters.current_fees->set_fee( trx.operations.front() );

      return trx = sign_transaction(trx, broadcast);
   }

   full_transaction propose_builder_transaction2(
      transaction_handle_type handle,
      string account_name_or_id,
      time_point_sec expiration = time_point::now() + fc::minutes(1),
      uint32_t review_period_seconds = 0, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      proposal_create_operation op;
      op.fee_paying_account = get_account(account_name_or_id).addr;
      op.expiration_time = expiration;
      signed_transaction& trx = _builder_transactions[handle];
      std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
                     [](const operation& op) -> op_wrapper { return op; });
      if( review_period_seconds )
         op.review_period_seconds = review_period_seconds;
      trx.operations = {op};
      _remote_db->get_global_properties().parameters.current_fees->set_fee( trx.operations.front() );

      return trx = sign_transaction(trx, broadcast);
   }
   std::map<std::string,asset> get_address_pay_back_balance(const address& owner_addr, std::string asset_symbol = "") const {
	   try {
		   return _remote_db->get_address_pay_back_balance(owner_addr, asset_symbol);
	   }
	   catch (...) {
		   return std::map<std::string, asset>();
	   }
   }
   full_transaction obtain_pay_back_balance(const string& pay_back_owner, std::map<std::string, asset> nums, bool broadcast = true) {
	   try {
		   FC_ASSERT(!self.is_locked());
		   pay_back_operation trx_op;
		   for (auto iter = nums.begin(); iter != nums.end(); ++iter)
		   {
			   fc::optional<asset_object> asset_obj = get_asset(iter->second.asset_id);
			   FC_ASSERT(asset_obj.valid(), "Could not find asset matching ${asset_id}", ("asset_id", iter->second.asset_id));
		   }
		   trx_op.pay_back_owner = address(pay_back_owner);
		   trx_op.pay_back_balance = nums;
		   trx_op.guarantee_id = get_guarantee_id();
		   signed_transaction tx;

		   tx.operations.push_back(trx_op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   } FC_CAPTURE_AND_RETHROW((pay_back_owner)(nums)(broadcast))
   }

   full_transaction obtain_bonus_balance(const string& bonus_owner, std::map<std::string, share_type> nums, bool broadcast = true)
   {
	   try {
		   FC_ASSERT(!self.is_locked());
		   bonus_operation op;
		   for (auto& iter : nums)
		   {
			   fc::optional<asset_object> asset_obj = get_asset(iter.first);
			   FC_ASSERT(asset_obj.valid(), "Could not find asset matching ${asset_id}", ("asset_id", iter.first));
		   }
		   op.bonus_owner = address(bonus_owner);
		   op.bonus_balance = nums;
		   op.guarantee_id = get_guarantee_id();
		   signed_transaction tx;
		   tx.operations.push_back(op);
		   set_operation_fees(tx,_remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();
		   return sign_transaction(tx,broadcast);

	   } FC_CAPTURE_AND_RETHROW((bonus_owner)(nums)(broadcast))
   }

   void remove_builder_transaction(transaction_handle_type handle)
   {
      _builder_transactions.erase(handle);
   }

   full_transaction register_account(string name, bool broadcast)
   {
	   try {
		   FC_ASSERT(!self.is_locked());
		   FC_ASSERT(is_valid_account_name(name));

		   account_create_operation account_create_op;

		   //juge if the name has been registered in the chain
		   auto acc_register = get_account(name);
		   FC_ASSERT(acc_register.addr != address(), "wallet hasnt this account.");
		   FC_ASSERT(_keys.count(acc_register.addr), "this name has existed in the chain.");
		   auto privkey = *wif_to_key(_keys[acc_register.addr]);
		   auto owner = privkey.get_public_key();
		   auto active = owner;
		   account_create_op.name = name;
		   account_create_op.owner = authority(1, graphene::chain::public_key_type(owner), 1);
		   account_create_op.active = authority(1, graphene::chain::public_key_type(active), 1);
		   account_create_op.payer = acc_register.addr;
		   account_create_op.options.memo_key = active;
		   account_create_op.guarantee_id = get_guarantee_id();
		   signed_transaction tx;
		   tx.operations.push_back(account_create_op);
		   auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
		   set_operation_fees(tx, current_fees);

		   auto dyn_props = get_dynamic_global_properties();
		   tx.set_reference_block(dyn_props.head_block_id);
		   tx.set_expiration(dyn_props.time + fc::seconds(30));
		   tx.validate();

		   ///sign

		   tx.sign(privkey, _chain_id);
		   _wallet.pending_account_registrations[name] = owner;
		   if (broadcast)
			   _remote_net_broadcast->broadcast_transaction(tx);
		   return tx;
	   }FC_CAPTURE_AND_RETHROW((name)(broadcast))
   }

   full_transaction register_contract(const string& caller_account_name, const string& gas_price, const string& gas_limit, const string& contract_filepath)
   {
	   try {
		   FC_ASSERT(!self.is_locked());
		   FC_ASSERT(is_valid_account_name(caller_account_name));

		   contract_register_operation contract_register_op;

		   //juge if the name has been registered in the chain
		   auto acc_caller = get_account(caller_account_name);
		   FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
		   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
		   auto privkey = *wif_to_key(_keys[acc_caller.addr]);
		   auto owner_pubkey = privkey.get_public_key();
		   
           fc::optional<asset_object> default_asset = get_asset(GRAPHENE_SYMBOL);
           contract_register_op.gas_price = default_asset->amount_from_string(gas_price).amount.value;

		   contract_register_op.init_cost = std::stoll(gas_limit);
		   contract_register_op.owner_addr = acc_caller.addr;
		   contract_register_op.owner_pubkey = owner_pubkey;
		   std::ifstream in(contract_filepath, std::ios::in | std::ios::binary);
		   FC_ASSERT(in.is_open());
		   std::vector<unsigned char> contract_filedata((std::istreambuf_iterator<char>(in)),
			   (std::istreambuf_iterator<char>()));
		   in.close();
		   auto contract_code = ContractHelper::load_contract_from_file(contract_filepath);
		   contract_register_op.contract_code = contract_code;
		   contract_register_op.contract_code.code_hash = contract_register_op.contract_code.GetHash();
		   contract_register_op.register_time = fc::time_point::now()+fc::seconds(1); 
		   contract_register_op.contract_id = contract_register_op.calculate_contract_id();
		   contract_register_op.fee.amount = 0;
		   contract_register_op.fee.asset_id = asset_id_type(0);
		   contract_register_op.guarantee_id = get_guarantee_id();
		   signed_transaction tx;
		   tx.operations.push_back(contract_register_op);
		   auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
		   set_operation_fees(tx, current_fees);

		   auto dyn_props = get_dynamic_global_properties();
		   tx.set_reference_block(dyn_props.head_block_id);
		   tx.set_expiration(dyn_props.time + fc::seconds(30));
		   tx.validate();

		   bool broadcast = true;
           full_transaction res = sign_transaction(tx, broadcast);
           res.contract_id= contract_register_op.contract_id.operator fc::string();
           return res;
	   }FC_CAPTURE_AND_RETHROW((caller_account_name)(gas_price)(gas_limit)(contract_filepath))
   }

   full_transaction register_contract_like(const string & caller_account_name, const string & gas_price, const string & gas_limit, const string & base)
   {
       try {
           FC_ASSERT(!self.is_locked());
           FC_ASSERT(is_valid_account_name(caller_account_name));

           contract_register_operation contract_register_op;

           //juge if the name has been registered in the chain
           auto acc_caller = get_account(caller_account_name);
           FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
		   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
           auto privkey = *wif_to_key(_keys[acc_caller.addr]);
           auto owner_pubkey = privkey.get_public_key();

           fc::optional<asset_object> default_asset = get_asset(GRAPHENE_SYMBOL);
           contract_register_op.gas_price = default_asset->amount_from_string(gas_price).amount.value;

           contract_register_op.init_cost = std::stoll(gas_limit);
           contract_register_op.owner_addr = acc_caller.addr;
           contract_register_op.owner_pubkey = owner_pubkey;


           contract_register_op.inherit_from = address(base);
           contract_register_op.register_time = fc::time_point::now() + fc::seconds(1);
           contract_register_op.contract_id = contract_register_op.calculate_contract_id();
           contract_register_op.fee.amount = 0;
           contract_register_op.fee.asset_id = asset_id_type(0);
		   contract_register_op.guarantee_id = get_guarantee_id();
           signed_transaction tx;
           tx.operations.push_back(contract_register_op);
           auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
           set_operation_fees(tx, current_fees);

           auto dyn_props = get_dynamic_global_properties();
           tx.set_reference_block(dyn_props.head_block_id);
           tx.set_expiration(dyn_props.time + fc::seconds(30));
           tx.validate();

           bool broadcast = true;
           full_transaction trx= sign_transaction(tx, broadcast);
           trx.contract_id= contract_register_op.contract_id.operator fc::string();
           return trx;
       }FC_CAPTURE_AND_RETHROW((caller_account_name)(gas_price)(gas_limit)(base))
   }
   std::pair<asset, share_type> register_contract_testing(const string& caller_account_name, const string& contract_filepath)
   {
       try {
           FC_ASSERT(!self.is_locked());
           FC_ASSERT(is_valid_account_name(caller_account_name));

           contract_register_operation contract_register_op;

           //juge if the name has been registered in the chain
           auto acc_caller = get_account(caller_account_name);
           FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
		   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
           auto privkey = *wif_to_key(_keys[acc_caller.addr]);
           auto owner_pubkey = privkey.get_public_key();

           contract_register_op.gas_price = 0 ;
           contract_register_op.init_cost = (GRAPHENE_CONTRACT_TESTING_GAS);
           contract_register_op.owner_addr = acc_caller.addr;
           contract_register_op.owner_pubkey = owner_pubkey;

           std::ifstream in(contract_filepath, std::ios::in | std::ios::binary);
           FC_ASSERT(in.is_open());
           std::vector<unsigned char> contract_filedata((std::istreambuf_iterator<char>(in)),
               (std::istreambuf_iterator<char>()));
           in.close();
           auto contract_code = ContractHelper::load_contract_from_file(contract_filepath);
           contract_register_op.contract_code = contract_code;
           contract_register_op.contract_code.code_hash = contract_register_op.contract_code.GetHash();
           contract_register_op.register_time = fc::time_point::now() + fc::seconds(1);
           contract_register_op.contract_id = contract_register_op.calculate_contract_id();
           contract_register_op.fee.amount = 0;
           contract_register_op.fee.asset_id = asset_id_type(0);

           signed_transaction tx;
           tx.operations.push_back(contract_register_op);
           auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
           set_operation_fees(tx, current_fees);

           auto dyn_props = get_dynamic_global_properties();
           tx.set_reference_block(dyn_props.head_block_id);
           tx.set_expiration(dyn_props.time + fc::seconds(30));
           tx.validate();

           auto signed_tx = sign_transaction(tx, false);
           auto trx_res=_remote_db->validate_transaction(signed_tx,true);
           share_type gas_count = 0;
           for (auto op_res : trx_res.operation_results)
           {
               try { gas_count += op_res.get<contract_operation_result_info>().gas_count; }
               catch (...)
               {

               }
           }
           asset res_data_fee= signed_tx.operations[0].get<contract_register_operation>().fee ;
           std::pair<asset, share_type> res=make_pair(res_data_fee,gas_count);
           return res;
       }FC_CAPTURE_AND_RETHROW((caller_account_name)(contract_filepath))
   }

   string register_native_contract(const string& caller_account_name, const string& gas_price, const string& gas_limit, const string& native_contract_key)
   {
	   try {
		   FC_ASSERT(!self.is_locked());
		   FC_ASSERT(is_valid_account_name(caller_account_name));

		   native_contract_register_operation n_contract_register_op;

		   //juge if the name has been registered in the chain
		   auto acc_caller = get_account(caller_account_name);
		   FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
		   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
		   auto privkey = *wif_to_key(_keys[acc_caller.addr]);
		   auto owner_pubkey = privkey.get_public_key();

           fc::optional<asset_object> default_asset = get_asset(GRAPHENE_SYMBOL);
           n_contract_register_op.gas_price = default_asset->amount_from_string(gas_price).amount.value;

		   n_contract_register_op.init_cost = std::stoll(gas_limit);
		   n_contract_register_op.owner_addr = acc_caller.addr;
		   n_contract_register_op.owner_pubkey = owner_pubkey;

		   FC_ASSERT(native_contract_finder::has_native_contract_with_key(native_contract_key));
		   n_contract_register_op.native_contract_key = native_contract_key;
		   n_contract_register_op.register_time = fc::time_point::now() + fc::seconds(1);
		   n_contract_register_op.contract_id = n_contract_register_op.calculate_contract_id();
		   n_contract_register_op.fee.amount = 0;
		   n_contract_register_op.fee.asset_id = asset_id_type(0);
		   n_contract_register_op.guarantee_id = get_guarantee_id();
		   signed_transaction tx;
		   tx.operations.push_back(n_contract_register_op);
		   auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
		   set_operation_fees(tx, current_fees);

		   auto dyn_props = get_dynamic_global_properties();
		   tx.set_reference_block(dyn_props.head_block_id);
		   tx.set_expiration(dyn_props.time + fc::seconds(30));
		   tx.validate();

		   bool broadcast = true;
		   auto signed_tx = sign_transaction(tx, broadcast);
		   return n_contract_register_op.contract_id.operator fc::string();
	   }FC_CAPTURE_AND_RETHROW((caller_account_name)(gas_price)(gas_limit)(native_contract_key))
   }

   std::pair<asset, share_type> register_native_contract_testing(const string & caller_account_name, const string & native_contract_key)
   {
       try {
           FC_ASSERT(!self.is_locked());
           FC_ASSERT(is_valid_account_name(caller_account_name));

           native_contract_register_operation n_contract_register_op;

           //juge if the name has been registered in the chain
           auto acc_caller = get_account(caller_account_name);
           FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
		   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
           auto privkey = *wif_to_key(_keys[acc_caller.addr]);
           auto owner_pubkey = privkey.get_public_key();

           n_contract_register_op.gas_price =  0;
           n_contract_register_op.init_cost = GRAPHENE_CONTRACT_TESTING_GAS;
           n_contract_register_op.owner_addr = acc_caller.addr;
           n_contract_register_op.owner_pubkey = owner_pubkey;

           FC_ASSERT(native_contract_finder::has_native_contract_with_key(native_contract_key));
           n_contract_register_op.native_contract_key = native_contract_key;
           n_contract_register_op.register_time = fc::time_point::now() + fc::seconds(1);
           n_contract_register_op.contract_id = n_contract_register_op.calculate_contract_id();
           n_contract_register_op.fee.amount = 0;
           n_contract_register_op.fee.asset_id = asset_id_type(0);

           signed_transaction tx;
           tx.operations.push_back(n_contract_register_op);
           auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
           set_operation_fees(tx, current_fees);

           auto dyn_props = get_dynamic_global_properties();
           tx.set_reference_block(dyn_props.head_block_id);
           tx.set_expiration(dyn_props.time + fc::seconds(30));
           tx.validate();

           auto signed_tx = sign_transaction(tx, false);
           
           auto trx_res = _remote_db->validate_transaction(signed_tx,true);
           share_type gas_count = 0;
           for (auto op_res : trx_res.operation_results)
           {
               try { gas_count += op_res.get<contract_operation_result_info>().gas_count; }
               catch (...)
               {

               }
           }
           asset res_data_fee = signed_tx.operations[0].get<native_contract_register_operation>().fee;
           std::pair<asset, share_type> res = make_pair(res_data_fee, gas_count);
           return res;
       }FC_CAPTURE_AND_RETHROW((caller_account_name)(native_contract_key))
   }
   std::pair<asset, share_type> invoke_contract_testing(const string & caller_account_name, const string & contract_address_or_name, const string & contract_api, const string & contract_arg)
   {
       try {
           FC_ASSERT(!self.is_locked());
           FC_ASSERT(is_valid_account_name(caller_account_name));

           contract_invoke_operation contract_invoke_op;

           //juge if the name has been registered in the chain
           auto acc_caller = get_account(caller_account_name);
           FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
		   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
           auto privkey = *wif_to_key(_keys[acc_caller.addr]);
           auto caller_pubkey = privkey.get_public_key();


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
			   cont = _remote_db->get_contract_object_by_name(contract_address_or_name);
			   contract_address = string(cont.contract_address);
		   }
		   else
		   {
			   cont = _remote_db->get_contract_object(contract_address_or_name);
			   contract_address = string(cont.contract_address);
		   }
           contract_invoke_op.gas_price =  0;
           contract_invoke_op.invoke_cost = GRAPHENE_CONTRACT_TESTING_GAS;
           contract_invoke_op.caller_addr = acc_caller.addr;
           contract_invoke_op.caller_pubkey = caller_pubkey;
           contract_invoke_op.contract_id = address(contract_address);
           contract_invoke_op.contract_api = contract_api;
           contract_invoke_op.contract_arg = contract_arg;
           contract_invoke_op.fee.amount = 0;
           contract_invoke_op.fee.asset_id = asset_id_type(0);

           signed_transaction tx;
           tx.operations.push_back(contract_invoke_op);
           auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
           set_operation_fees(tx, current_fees);

           auto dyn_props = get_dynamic_global_properties();
           tx.set_reference_block(dyn_props.head_block_id);
           tx.set_expiration(dyn_props.time + fc::seconds(30));
           tx.validate();

           auto signed_tx = sign_transaction(tx, false);
           auto trx_res=_remote_db->validate_transaction(signed_tx,true);
           share_type gas_count = 0;
           for (auto op_res:trx_res.operation_results)
           {
               try { gas_count += op_res.get<contract_operation_result_info>().gas_count; }
               catch (...)
               {

               }
           }
           asset res_data_fee = signed_tx.operations[0].get<contract_invoke_operation>().fee;
           std::pair<asset, share_type> res = make_pair(res_data_fee, gas_count);
           return res;;
       }FC_CAPTURE_AND_RETHROW((caller_account_name)(contract_address_or_name)(contract_api)(contract_arg))

   }
   full_transaction invoke_contract(const string& caller_account_name, const string& gas_price, const string& gas_limit, const string& contract_address_or_name, const string& contract_api, const string& contract_arg)
   {
	   try {
		   FC_ASSERT(!self.is_locked());
		   FC_ASSERT(is_valid_account_name(caller_account_name));

		   contract_invoke_operation contract_invoke_op;

		   //juge if the name has been registered in the chain
		   auto acc_caller = get_account(caller_account_name);
		   FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
		   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
		   auto privkey = *wif_to_key(_keys[acc_caller.addr]);
		   auto caller_pubkey = privkey.get_public_key();

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
			   cont = _remote_db->get_contract_object_by_name(contract_address_or_name);
			   contract_address = string(cont.contract_address);
		   }
		   else
		   {
			   cont = _remote_db->get_contract_object(contract_address_or_name);
			   contract_address = string(cont.contract_address);
		   }
		   auto& abi = cont.code.abi;
		   if (abi.find(contract_api) == abi.end())
			   FC_CAPTURE_AND_THROW(blockchain::contract_engine::contract_api_not_found);
           fc::optional<asset_object> default_asset = get_asset(GRAPHENE_SYMBOL);
           contract_invoke_op.gas_price = default_asset->amount_from_string(gas_price).amount.value;

		   contract_invoke_op.invoke_cost = std::stoll(gas_limit);
		   contract_invoke_op.caller_addr = acc_caller.addr;
		   contract_invoke_op.caller_pubkey = caller_pubkey;
		   contract_invoke_op.contract_id = address(contract_address);
		   contract_invoke_op.contract_api = contract_api;
		   contract_invoke_op.contract_arg = contract_arg;
		   contract_invoke_op.fee.amount = 0;
		   contract_invoke_op.fee.asset_id = asset_id_type(0);
		   contract_invoke_op.guarantee_id = get_guarantee_id();
		   signed_transaction tx;
		   tx.operations.push_back(contract_invoke_op);
		   auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
		   set_operation_fees(tx, current_fees);

		   auto dyn_props = get_dynamic_global_properties();
		   tx.set_reference_block(dyn_props.head_block_id);
		   tx.set_expiration(dyn_props.time + fc::seconds(30));
		   tx.validate();

		   bool broadcast = true;
		   auto signed_tx = sign_transaction(tx, broadcast);
		   return signed_tx;
	   }FC_CAPTURE_AND_RETHROW((caller_account_name)(gas_price)(gas_limit)(contract_address_or_name)(contract_api)(contract_arg))
   }

   string invoke_contract_offline(const string& caller_account_name, const string& contract_address_or_name, const string& contract_api, const string& contract_arg)
   {
	   try {
		   FC_ASSERT(!self.is_locked());
		   FC_ASSERT(is_valid_account_name(caller_account_name));

		   contract_invoke_operation contract_invoke_op;

		   //juge if the name has been registered in the chain
		   auto acc_caller = get_account(caller_account_name);
		   FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
		   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
		   auto privkey = *wif_to_key(_keys[acc_caller.addr]);
		   auto caller_pubkey = privkey.get_public_key();
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
			   cont = _remote_db->get_contract_object_by_name(contract_address_or_name);
			   contract_address = string(cont.contract_address);
		   }
		   else
		   {
				cont = _remote_db->get_contract_object(contract_address_or_name);
				contract_address = string(cont.contract_address);
		   }
		   auto& abi = cont.code.offline_abi;
		   if (abi.find(contract_api) == abi.end())
			   FC_CAPTURE_AND_THROW(blockchain::contract_engine::contract_api_not_found);

		   contract_invoke_op.gas_price = 0;
		   contract_invoke_op.invoke_cost = GRAPHENE_CONTRACT_TESTING_GAS;
		   contract_invoke_op.caller_addr = acc_caller.addr;
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
		   auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
		   set_operation_fees(tx, current_fees);

		   auto dyn_props = get_dynamic_global_properties();
		   tx.set_reference_block(dyn_props.head_block_id);
		   tx.set_expiration(dyn_props.time + fc::seconds(30));
		   tx.validate();
		   auto signed_tx = sign_transaction(tx, false, true);
		   auto trx_res = _remote_db->validate_transaction(signed_tx, true);
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
	   }FC_CAPTURE_AND_RETHROW((caller_account_name)(contract_address_or_name)(contract_api)(contract_arg))
   }

   full_transaction upgrade_contract(const string& caller_account_name, const string& gas_price, const string& gas_limit, const string& contract_address, const string& contract_name, const string& contract_desc)
   {
	   try {
		   FC_ASSERT(!self.is_locked());
		   FC_ASSERT(is_valid_account_name(caller_account_name));

		   contract_upgrade_operation contract_upgrade_op;

		   //juge if the name has been registered in the chain
		   auto acc_caller = get_account(caller_account_name);
		   FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
		   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
		   auto privkey = *wif_to_key(_keys[acc_caller.addr]);
		   auto caller_pubkey = privkey.get_public_key();

           fc::optional<asset_object> default_asset = get_asset(GRAPHENE_SYMBOL);
           contract_upgrade_op.gas_price = default_asset->amount_from_string(gas_price).amount.value;

		   contract_upgrade_op.invoke_cost = std::stoll(gas_limit);
		   contract_upgrade_op.caller_addr = acc_caller.addr;
		   contract_upgrade_op.caller_pubkey = caller_pubkey;
		   contract_upgrade_op.contract_id = address(contract_address);
           contract_upgrade_operation::contract_name_check(contract_name);
		   contract_upgrade_op.contract_name = contract_name;
		   contract_upgrade_op.contract_desc = contract_desc;
		   contract_upgrade_op.fee.amount = 0;
		   contract_upgrade_op.fee.asset_id = asset_id_type(0);
		   contract_upgrade_op.guarantee_id = get_guarantee_id();
		   signed_transaction tx;
		   tx.operations.push_back(contract_upgrade_op);
		   auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
		   set_operation_fees(tx, current_fees);

		   auto dyn_props = get_dynamic_global_properties();
		   tx.set_reference_block(dyn_props.head_block_id);
		   tx.set_expiration(dyn_props.time + fc::seconds(30));
		   tx.validate();

		   bool broadcast = true;
		   auto signed_tx = sign_transaction(tx, broadcast);
		   return signed_tx;
	   }FC_CAPTURE_AND_RETHROW((caller_account_name)(gas_price)(gas_limit)(contract_address)(contract_name)(contract_desc))
   }

   variant_object decoderawtransaction(const string& raw_trx, const string& symbol)
   {
	   try {
		   auto cross_mgr = graphene::privatekey_management::crosschain_management::get_instance();
		   return cross_mgr.decoderawtransaction(raw_trx,symbol);
		  
	   }FC_CAPTURE_AND_RETHROW((raw_trx)(symbol))
   }
   variant_object createrawtransaction(const string& from, const string& to, const string& amount, const string& symbol)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   string config = (*_crosschain_manager)->get_config();
		   FC_ASSERT((*_crosschain_manager)->contain_symbol(symbol), "no this plugin");
		   auto crosschain = crosschain::crosschain_manager::get_instance().get_crosschain_handle(symbol);
		   crosschain->initialize_config(fc::json::from_string(config).get_object());
		   FC_ASSERT(crosschain->validate_address(from));
		   FC_ASSERT(crosschain->validate_address(to));
		   auto gas_price_pos = amount.find('|');
		   if (gas_price_pos != amount.npos)
		   {
			   FC_ASSERT(((symbol == "ETH") || (symbol.find("ERC") != symbol.npos)),"only eth or erc asset need GasPrice");
		   }
		   map<string, string> dest;
		   dest[to] = amount;
		   if ((symbol == "ETH") || (symbol.find("ERC") != symbol.npos)) {
			   std::string real_amount = amount;
			   string gas_price = fc::to_string(5) + "000000000";
			   if (gas_price_pos != amount.npos) {
				   auto temp_gas_price = amount.substr(gas_price_pos + 1);
				   auto int_gas_price = fc::to_uint64(temp_gas_price);
				   gas_price = fc::to_string(int_gas_price) + "000000000";
				   real_amount = amount.substr(0, gas_price_pos);
			   }
			   fc::optional<asset_object> asset_obj = get_asset(symbol);

			   std::string from_acount = from;
			   std::string to_account = to+'|'+gas_price;
			   std::string amount_to_trans = real_amount;
			   std::string _symbol = symbol;
			   std::string memo = asset_obj->options.description;
			   return crosschain->create_multisig_transaction(from_acount, to_account,amount_to_trans,_symbol, memo,false);
		   }
		   else {
		   return crosschain->create_multisig_transaction(from,dest,symbol,"");
		   }
		  
	   }FC_CAPTURE_AND_RETHROW((from)(to)(amount)(symbol))
   }

   string signrawtransaction(const string& from,const string& symbol, const fc::variant_object& trx, bool broadcast = true)
   {
	   try {
		   FC_ASSERT(!is_locked());
		  
		   auto iter = _crosschain_keys.find(from);
		   FC_ASSERT(iter != _crosschain_keys.end(),"there is no private key in this wallet.");
		   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(symbol);
		   auto pk = prk_ptr->import_private_key(iter->second.wif_key);
		   FC_ASSERT(pk.valid());
		   std::string raw;
		   if ((symbol == "ETH") || (symbol.find("ERC") != symbol.npos)){
			   raw = trx["without_sign"].as_string();
			   raw = prk_ptr->sign_trx(raw, 0);
		   }
		   else {
		   auto vins = trx["trx"].get_object()["vin"].get_array();
				raw = trx["hex"].as_string();
		   for (auto index=0;index < vins.size(); index++)
		   {
			   raw=prk_ptr->sign_trx(raw,index);
		   }
		   }
		   
		   if (broadcast)
		   {
			   string config = (*_crosschain_manager)->get_config();
			   FC_ASSERT((*_crosschain_manager)->contain_symbol(symbol), "no this plugin");
			   auto& instance = graphene::crosschain::crosschain_manager::get_instance();
			   auto fd = instance.get_crosschain_handle(symbol);
			   fd->initialize_config(fc::json::from_string(config).get_object());
			   if ((symbol == "ETH") || (symbol.find("ERC") != symbol.npos)) {
				   fc::variant_object new_trx("trx", "0x"+raw);
				   fd->broadcast_transaction(new_trx);
			   }
			   else {
			   fc::variant_object new_trx("hex", raw);
			   fd->broadcast_transaction(new_trx);
		   }
		   }
		   return raw;
	   }FC_CAPTURE_AND_RETHROW((from)(trx)(broadcast))
   }

   std::pair<asset, share_type> upgrade_contract_testing(const string & caller_account_name, const string & contract_address, const string & contract_name, const string & contract_desc)
   {
       try {
           FC_ASSERT(!self.is_locked());
           FC_ASSERT(is_valid_account_name(caller_account_name));

           contract_upgrade_operation contract_upgrade_op;

           //juge if the name has been registered in the chain
           auto acc_caller = get_account(caller_account_name);
           FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
		   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
           auto privkey = *wif_to_key(_keys[acc_caller.addr]);
           auto caller_pubkey = privkey.get_public_key();

           contract_upgrade_op.gas_price = 0;
           contract_upgrade_op.invoke_cost = GRAPHENE_CONTRACT_TESTING_GAS;
           contract_upgrade_op.caller_addr = acc_caller.addr;
           contract_upgrade_op.caller_pubkey = caller_pubkey;
           contract_upgrade_op.contract_id = address(contract_address);
           contract_upgrade_operation::contract_name_check(contract_name);
           contract_upgrade_op.contract_name = contract_name;
           contract_upgrade_op.contract_desc = contract_desc;
           contract_upgrade_op.fee.amount = 0;
           contract_upgrade_op.fee.asset_id = asset_id_type(0);

           signed_transaction tx;
           tx.operations.push_back(contract_upgrade_op);
           auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
           set_operation_fees(tx, current_fees);

           auto dyn_props = get_dynamic_global_properties();
           tx.set_reference_block(dyn_props.head_block_id);
           tx.set_expiration(dyn_props.time + fc::seconds(600));
           tx.validate();

           auto signed_tx = sign_transaction(tx, false);
           auto trx_res = _remote_db->validate_transaction(signed_tx,true);
           share_type gas_count = 0;
           for (auto op_res : trx_res.operation_results)
           {
               try { gas_count += op_res.get<contract_operation_result_info>().gas_count; }
               catch (...)
               {

               }
           }           
           asset res_data_fee = signed_tx.operations[0].get<contract_upgrade_operation>().fee;
           std::pair<asset, share_type> res = make_pair(res_data_fee, gas_count);
           return res;
       }FC_CAPTURE_AND_RETHROW((caller_account_name)(contract_address)(contract_name)(contract_desc))

   }

   full_transaction transfer_to_contract(string from,
       string to,
       string amount,
       string asset_symbol,
       const string& param,
       const string& gas_price,
       const string& gas_limit,
       bool broadcast = false)
   {
       FC_ASSERT(!self.is_locked());
       FC_ASSERT(is_valid_account_name(from));

       transfer_contract_operation transfer_to_contract_op;
       fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
       FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
       asset transfer_asset=asset_obj->amount_from_string(amount);
       //juge if the name has been registered in the chain
       auto acc_caller = get_account(from);
       FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
	   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
       auto privkey = *wif_to_key(_keys[acc_caller.addr]);
       auto caller_pubkey = privkey.get_public_key();

       fc::optional<asset_object> default_asset = get_asset(GRAPHENE_SYMBOL);
       transfer_to_contract_op.gas_price = default_asset->amount_from_string(gas_price).amount.value;

       transfer_to_contract_op.invoke_cost = std::stoll(gas_limit);
       transfer_to_contract_op.caller_addr = acc_caller.addr;
       transfer_to_contract_op.caller_pubkey = caller_pubkey;
       transfer_to_contract_op.contract_id = address(to);
       transfer_to_contract_op.fee.amount = 0;
       transfer_to_contract_op.fee.asset_id = asset_id_type(0);
       transfer_to_contract_op.amount = transfer_asset;
       transfer_to_contract_op.param = param;
	   transfer_to_contract_op.guarantee_id = get_guarantee_id();
       signed_transaction tx;
       tx.operations.push_back(transfer_to_contract_op);
       auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
       set_operation_fees(tx, current_fees);

       auto dyn_props = get_dynamic_global_properties();
       tx.set_reference_block(dyn_props.head_block_id);
       tx.set_expiration(dyn_props.time + fc::seconds(30));
       tx.validate();

       auto signed_tx = sign_transaction(tx, broadcast);
       return signed_tx;
   }
   std::pair<asset, share_type> transfer_to_contract_testing(string from, string to, string amount, string asset_symbol,const string& param)
   {
       FC_ASSERT(!self.is_locked());
       FC_ASSERT(is_valid_account_name(from));

       transfer_contract_operation transfer_to_contract_op;
       fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
       FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
       asset transfer_asset = asset_obj->amount_from_string(amount);
       //juge if the name has been registered in the chain
       auto acc_caller = get_account(from);
       FC_ASSERT(acc_caller.addr != address(), "contract owner can't be empty.");
	   FC_ASSERT(_keys.count(acc_caller.addr), "this name has not existed in the wallet.");
       auto privkey = *wif_to_key(_keys[acc_caller.addr]);
       auto caller_pubkey = privkey.get_public_key();

       transfer_to_contract_op.gas_price = 0;
       transfer_to_contract_op.invoke_cost = GRAPHENE_CONTRACT_TESTING_GAS;
       transfer_to_contract_op.caller_addr = acc_caller.addr;
       transfer_to_contract_op.caller_pubkey = caller_pubkey;
       transfer_to_contract_op.contract_id = address(to);
       transfer_to_contract_op.fee.amount = 0;
       transfer_to_contract_op.fee.asset_id = asset_id_type(0);
       transfer_to_contract_op.amount = transfer_asset;
       transfer_to_contract_op.param = param;

       signed_transaction tx;
       tx.operations.push_back(transfer_to_contract_op);
       auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
       set_operation_fees(tx, current_fees);

       auto dyn_props = get_dynamic_global_properties();
       tx.set_reference_block(dyn_props.head_block_id);
       tx.set_expiration(dyn_props.time + fc::seconds(30));
       tx.validate();

       auto signed_tx = sign_transaction(tx, false);
       auto trx_res = _remote_db->validate_transaction(signed_tx,true);
       share_type gas_count = 0;
       for (auto op_res : trx_res.operation_results)
       {
           try { gas_count += op_res.get<contract_operation_result_info>().gas_count; }
           catch (...)
           {

           }
       }
       asset res_data_fee = signed_tx.operations[0].get<transfer_contract_operation>().fee;
       std::pair<asset, share_type> res = make_pair(res_data_fee, gas_count);
       return res;
   }
   full_transaction create_contract_transfer_fee_proposal(const string& proposer, share_type fee_rate, int64_t expiration_time, bool broadcast = false)
   {
	   try
	   {
		   FC_ASSERT(!is_locked());
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(proposer).get_id();
		   prop_op.fee_paying_account = get_account(proposer).addr;
		   contract_transfer_fee_proposal_operation op;
		   op.fee_rate = fee_rate;

		   auto guard_obj = get_guard_member(proposer);
		   auto guard_id = guard_obj.guard_member_account;
		   auto guard_account_obj = get_account(guard_id);
		   op.guard = guard_account_obj.addr;
		   op.guard_id = guard_obj.id;


		   const chain_parameters& current_params = get_global_properties().parameters;
		   prop_op.proposed_ops.emplace_back(op);
		   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
		   signed_transaction trx;
		   trx.operations.emplace_back(prop_op);
		   set_operation_fees(trx, current_params.current_fees);
		   trx.validate();

		   return sign_transaction(trx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((proposer)(fee_rate)(expiration_time)(broadcast))

   }
   full_transaction register_account(string name,
                                       public_key_type owner,
                                       public_key_type active,
                                       string  registrar_account,
                                       string  referrer_account,
                                       uint32_t referrer_percent,
                                       bool broadcast = false)
   { try {
      FC_ASSERT( !self.is_locked() );
      FC_ASSERT( is_valid_name(name) );
      account_create_operation account_create_op;

      // #449 referrer_percent is on 0-100 scale, if user has larger
      // number it means their script is using GRAPHENE_100_PERCENT scale
      // instead of 0-100 scale.
      FC_ASSERT( referrer_percent <= 100 );
      // TODO:  process when pay_from_account is ID

      account_object registrar_account_object =
            this->get_account( registrar_account );
      FC_ASSERT( registrar_account_object.is_lifetime_member() );

      account_id_type registrar_account_id = registrar_account_object.id;

      account_object referrer_account_object =
            this->get_account( referrer_account );
      account_create_op.referrer = referrer_account_object.id;
      account_create_op.referrer_percent = uint16_t( referrer_percent * GRAPHENE_1_PERCENT );

      account_create_op.registrar = registrar_account_id;
      account_create_op.name = name;
      account_create_op.owner = authority(1, owner, 1);
      account_create_op.active = authority(1, active, 1);
      account_create_op.options.memo_key = active;

      signed_transaction tx;

      tx.operations.push_back( account_create_op );

      auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
      set_operation_fees( tx, current_fees );

      vector<public_key_type> paying_keys = registrar_account_object.active.get_keys();

      auto dyn_props = get_dynamic_global_properties();
      tx.set_reference_block( dyn_props.head_block_id );
      tx.set_expiration( dyn_props.time + fc::seconds(30) );
      tx.validate();

      for( public_key_type& key : paying_keys )
      {
         auto it = _keys.find(key);
         if( it != _keys.end() )
         {
            fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
            if( !privkey.valid() )
            {
               FC_ASSERT( false, "Malformed private key in _keys" );
            }
            tx.sign( *privkey, _chain_id );
         }
      }
	  
      if( broadcast )
         _remote_net_broadcast->broadcast_transaction( tx );
      return tx;
   } FC_CAPTURE_AND_RETHROW( (name)(owner)(active)(registrar_account)(referrer_account)(referrer_percent)(broadcast) ) }

   full_transaction upgrade_account(string name, bool broadcast)
   { try {
      FC_ASSERT( !self.is_locked() );
      account_object account_obj = get_account(name);
      FC_ASSERT( !account_obj.is_lifetime_member() );

      signed_transaction tx;
      account_upgrade_operation op;
      op.account_to_upgrade = account_obj.get_id();
      op.upgrade_to_lifetime_member = true;
      tx.operations = {op};
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (name) ) }

   full_transaction create_guarantee_order(const string& account, const string& asset_orign, const string& asset_target, const string& symbol, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   auto acc = get_account(account);
		   auto asset_obj = get_asset(symbol);
		   gurantee_create_operation op;
		   op.owner_addr = acc.addr;
		   auto sys_asset = get_asset(GRAPHENE_SYMBOL);
		   op.asset_origin = asset(sys_asset.amount_from_string(asset_orign).amount, sys_asset.get_id());
		   auto target_asset = get_asset(symbol);
		   op.asset_target = asset(target_asset.amount_from_string(asset_target).amount, target_asset.get_id());
		   op.time = string(fc::time_point::now());
		   op.symbol = symbol;

		   signed_transaction trx;
		   trx.operations.push_back(op);
		   set_operation_fees(trx, _remote_db->get_global_properties().parameters.current_fees);
		   trx.validate();
		   return sign_transaction(trx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(asset_orign)(asset_target)(symbol)(broadcast))
   }
   full_transaction cancel_guarantee_order(const guarantee_object_id_type id, bool broadcast)
   {
	   try {
		   auto guarantee_obj = _remote_db->get_gurantee_object(id);
		   FC_ASSERT(guarantee_obj.valid());

		   gurantee_cancel_operation op;
		   op.owner_addr = guarantee_obj->owner_addr;
		   op.cancel_guarantee_id = guarantee_obj->id;

		   signed_transaction trx;
		   trx.operations.push_back(op);
		   set_operation_fees(trx, _remote_db->get_global_properties().parameters.current_fees);
		   trx.validate();
		   return sign_transaction(trx, broadcast);

	   }FC_CAPTURE_AND_RETHROW((id)(broadcast))
   }


   // This function generates derived keys starting with index 0 and keeps incrementing
   // the index until it finds a key that isn't registered in the block chain.  To be
   // safer, it continues checking for a few more keys to make sure there wasn't a short gap
   // caused by a failed registration or the like.
   int find_first_unused_derived_key_index(const fc::ecc::private_key& parent_key)
   {
      int first_unused_index = 0;
      int number_of_consecutive_unused_keys = 0;
      for (int key_index = 0; ; ++key_index)
      {
         fc::ecc::private_key derived_private_key = derive_private_key(key_to_wif(parent_key), key_index);
         graphene::chain::public_key_type derived_public_key = derived_private_key.get_public_key();
         if( _keys.find(derived_public_key) == _keys.end() )
         {
            if (number_of_consecutive_unused_keys)
            {
               ++number_of_consecutive_unused_keys;
               if (number_of_consecutive_unused_keys > 5)
                  return first_unused_index;
            }
            else
            {
               first_unused_index = key_index;
               number_of_consecutive_unused_keys = 1;
            }
         }
         else
         {
            // key_index is used
            first_unused_index = 0;
            number_of_consecutive_unused_keys = 0;
         }
      }
   }
   
   full_transaction create_account_with_private_key(fc::ecc::private_key owner_privkey,
                                                      string account_name,
                                                      string registrar_account,
                                                      string referrer_account,
                                                      bool broadcast = false,
                                                      bool save_wallet = true)
   { try {
         int active_key_index = find_first_unused_derived_key_index(owner_privkey);
         fc::ecc::private_key active_privkey = derive_private_key( key_to_wif(owner_privkey), active_key_index);

         int memo_key_index = find_first_unused_derived_key_index(active_privkey);
         fc::ecc::private_key memo_privkey = derive_private_key( key_to_wif(active_privkey), memo_key_index);

         graphene::chain::public_key_type owner_pubkey = owner_privkey.get_public_key();
         graphene::chain::public_key_type active_pubkey = active_privkey.get_public_key();
         graphene::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();

         account_create_operation account_create_op;

         // TODO:  process when pay_from_account is ID

         account_object registrar_account_object = get_account( registrar_account );

         account_id_type registrar_account_id = registrar_account_object.id;

         account_object referrer_account_object = get_account( referrer_account );
         account_create_op.referrer = referrer_account_object.id;
         account_create_op.referrer_percent = referrer_account_object.referrer_rewards_percentage;

         account_create_op.registrar = registrar_account_id;
         account_create_op.name = account_name;
         account_create_op.owner = authority(1, owner_pubkey, 1);
         account_create_op.active = authority(1, active_pubkey, 1);
         account_create_op.options.memo_key = memo_pubkey;

         // current_fee_schedule()
         // find_account(pay_from_account)

         // account_create_op.fee = account_create_op.calculate_fee(db.current_fee_schedule());

         signed_transaction tx;

         tx.operations.push_back( account_create_op );

         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);

         vector<public_key_type> paying_keys = registrar_account_object.active.get_keys();

         auto dyn_props = get_dynamic_global_properties();
         tx.set_reference_block( dyn_props.head_block_id );
         tx.set_expiration( dyn_props.time + fc::seconds(30) );
         tx.validate();

         for( public_key_type& key : paying_keys )
         {
            auto it = _keys.find(key);
            if( it != _keys.end() )
            {
               fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
               FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
               tx.sign( *privkey, _chain_id );
            }
         }

         // we do not insert owner_privkey here because
         //    it is intended to only be used for key recovery
         //_wallet.pending_account_registrations[account_name].push_back(key_to_wif( active_privkey ));
         //_wallet.pending_account_registrations[account_name].push_back(key_to_wif( memo_privkey ));
         if( save_wallet )
            save_wallet_file();
         if( broadcast )
            _remote_net_broadcast->broadcast_transaction( tx );
         return tx;
   } FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account)(referrer_account)(broadcast) ) }

   full_transaction create_account_with_brain_key(string brain_key,
                                                    string account_name,
                                                    string registrar_account,
                                                    string referrer_account,
                                                    bool broadcast = false,
                                                    bool save_wallet = true)
   { try {
      FC_ASSERT( !self.is_locked() );
      string normalized_brain_key = normalize_brain_key( brain_key );
      // TODO:  scan blockchain for accounts that exist with same brain key
      fc::ecc::private_key owner_privkey = derive_private_key( normalized_brain_key, 0 );
      return create_account_with_private_key(owner_privkey, account_name, registrar_account, referrer_account, broadcast, save_wallet);
   } FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account)(referrer_account) ) }

   bool is_valid_account_name(const string& name)
   {
	   try {
		   if (name.size() < GRAPHENE_MIN_ACCOUNT_NAME_LENGTH) return false;
		   if (name.size() > GRAPHENE_MAX_ACCOUNT_NAME_LENGTH) return false;
		   if (!isalpha(name[0])) return false;
		   if (!isalnum(name[name.size() - 1]) || isupper(name[name.size() - 1])) return false;

		   string subname(name);
		   string supername;
		   int dot = name.find('.');
		   if (dot != string::npos)
		   {
			   subname = name.substr(0, dot);
			   //There is definitely a remainder; we checked above that the last character is not a dot
			   supername = name.substr(dot + 1);
		   }

		   if (!isalnum(subname[subname.size() - 1]) || isupper(subname[subname.size() - 1])) return false;
		   for (const auto& c : subname)
		   {
			   if (isalnum(c) && !isupper(c)) continue;
			   else if (c == '-') continue;
			   else return false;
		   }

		   if (supername.empty())
			   return true;
		   return is_valid_account_name(supername);
	   } FC_CAPTURE_AND_RETHROW((name))
   }
   void change_acquire_plugin_num(const string&symbol, const uint32_t& blocknum) {
	  _remote_db->set_acquire_block_num(symbol, blocknum);
   }
   fc::mutex brain_key_index_lock;
   address create_account(string account_name,bool from_master_key=false)
   {
	   try{
		   FC_ASSERT(!self.is_locked());
		   if (!is_valid_account_name(account_name))
			   FC_THROW("Invalid account name!");
		   auto& itr= _wallet.my_accounts.get<by_name>();
		   FC_ASSERT(itr.count(account_name) == 0, "Account with that name has exist!");
		   auto get_private_key = [account_name,this]()->address
		   {
			   auto prk = fc::ecc::private_key::generate();
			   auto exprk = fc::ecc::extended_private_key(prk, fc::sha256()).derive_child(0);
			   
			   auto result = fc::ecc::private_key::regenerate(fc::sha256::hash(exprk.to_base58()));
			   auto addr = address(result.get_public_key());
			   auto str_prk = key_to_wif(result);
			   _keys[addr] = str_prk;
			   
			   account_object acc;
			   acc.addr = addr;
			   acc.name = account_name;
			   _wallet.update_account(acc);
			 
			   save_wallet_file();
			   return address(result.get_public_key());
		   };
		   auto get_private_key_from_brain_key = [account_name, this]()->address
		   {
			   fc::scoped_lock<fc::mutex> lock(brain_key_index_lock);
			   FC_ASSERT(this->_current_brain_key.valid(), "brain_key not set");
			   auto brain_key = graphene::wallet::detail::normalize_brain_key(_current_brain_key->key);
			   fc::ecc::private_key priv_key = graphene::wallet::detail::derive_private_key(brain_key, _current_brain_key->next);
			   auto addr = address(priv_key.get_public_key());
			   auto str_prk = key_to_wif(priv_key);
			   _keys[addr] = str_prk;
			   _current_brain_key->used_indexes[addr.address_to_string()] = _current_brain_key->next;
			   account_object acc;
			   acc.addr = addr;
			   acc.name = account_name;
			   _wallet.update_account(acc);
			   _current_brain_key->next += 1;
			   save_wallet_file();
			   return address(priv_key.get_public_key());
		   };
		   auto addr = from_master_key? get_private_key_from_brain_key ():get_private_key();
		   _remote_trx->set_tracked_addr(addr);
		   return addr;

		   
	   }FC_CAPTURE_AND_RETHROW((account_name))
   }
   
   full_transaction create_asset(string issuer,
                                   string symbol,
                                   uint8_t precision,
                                   asset_options common,
                                   fc::optional<bitasset_options> bitasset_opts,
                                   bool broadcast = false)
   { try {
      account_object issuer_account = get_account( issuer );
      FC_ASSERT(!find_asset(symbol).valid(), "Asset with that symbol already exists!");

      asset_create_operation create_op;
      create_op.issuer = issuer_account.id;
      create_op.symbol = symbol;
      create_op.precision = precision;
      create_op.common_options = common;
      create_op.bitasset_opts = bitasset_opts;

      signed_transaction tx;
      tx.operations.push_back( create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (issuer)(symbol)(precision)(common)(bitasset_opts)(broadcast) ) }

   full_transaction wallet_create_asset(string issuer,
	   string symbol,
	   uint8_t precision,
	   share_type max_supply,
	   share_type core_fee_paid,
	   bool broadcast = false)
   {
	   try {
		   FC_ASSERT(!self.is_locked());
		   FC_ASSERT(!find_asset(symbol).valid(), "Asset with that symbol already exists!");

		   //need a create asset op create a new asset
		   asset_real_create_operation op;
		   auto issuer_account = get_guard_member(issuer);
		   op.issuer = issuer_account.guard_member_account;
		   op.issuer_addr = get_account(op.issuer).addr;
		   op.precision = precision;
		   op.max_supply = max_supply;
		   op.symbol = symbol;
		   op.core_fee_paid = core_fee_paid;
		   signed_transaction tx;
		   tx.operations.push_back(op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();
		   
		   return sign_transaction(tx, broadcast);
	   } FC_CAPTURE_AND_RETHROW((issuer)(symbol)(precision)(max_supply)(broadcast))
   }
   full_transaction wallet_create_erc_asset(string issuer,
	   string symbol,
	   uint8_t precision,
	   share_type max_supply,
	   share_type core_fee_paid,
	   std::string erc_address,
	   bool broadcast = false) {
	   try {
		   FC_ASSERT(!self.is_locked());
		   FC_ASSERT(!find_asset(symbol).valid(), "Asset with that symbol already exists!");
		   FC_ASSERT(symbol.find("ERC") != symbol.npos);
		   FC_ASSERT(erc_address != "");
		   auto precison_pos = erc_address.find('|');
		   FC_ASSERT(precison_pos != erc_address.npos);
		   auto erc_real_address = erc_address.substr(0, precison_pos);
		   auto erc_precision = erc_address.substr(precison_pos+1);
		   //need a create asset op create a new asset
		    /*string config = (*_crosschain_manager)->get_config();
			auto fd = crosschain::crosschain_manager::get_instance().get_crosschain_handle(symbol);
			fd->initialize_config(fc::json::from_string(config).get_object());
			fd->create_wallet(symbol, erc_address);*/
		   asset_eth_create_operation op;
		   auto issuer_account = get_guard_member(issuer);
		   op.issuer = issuer_account.guard_member_account;
		   op.issuer_addr = get_account(op.issuer).addr;
		   op.precision = precision;
		   op.max_supply = max_supply;
		   op.erc_real_precision = erc_precision;
		   op.symbol = symbol;
		   op.core_fee_paid = core_fee_paid;
		   op.erc_address = erc_real_address;
		   signed_transaction tx;
		   tx.operations.push_back(op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();

		   return sign_transaction(tx, broadcast);
	   } FC_CAPTURE_AND_RETHROW((issuer)(symbol)(precision)(max_supply)(broadcast)(erc_address))
   }

   full_transaction update_asset(const string& account, const std::string& symbol,
	                             const std::string& description,
	                             bool broadcast = false)
   { try {
      optional<asset_object> asset_to_update = find_asset(symbol);
      if (!asset_to_update)
        FC_THROW("No asset with that symbol exists!");
	  const auto& addr = get_account_addr(account);
	  const auto& acc_id = get_account_id(account);
	  FC_ASSERT(acc_id == asset_to_update->issuer, "only issuer can be modified.");
      asset_update_operation update_op;
      update_op.issuer = addr;
      update_op.asset_to_update = asset_to_update->id;
	  update_op.description = description;

      signed_transaction tx;
      tx.operations.push_back( update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account)(symbol)(description)(broadcast) ) }

   signed_transaction update_bitasset(string symbol,
                                      bitasset_options new_options,
                                      bool broadcast /* = false */)
   { try {
      optional<asset_object> asset_to_update = find_asset(symbol);
      if (!asset_to_update)
        FC_THROW("No asset with that symbol exists!");

      asset_update_bitasset_operation update_op;
      update_op.issuer = asset_to_update->issuer;
      update_op.asset_to_update = asset_to_update->id;
      update_op.new_options = new_options;

      full_transaction tx;
      tx.operations.push_back( update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (symbol)(new_options)(broadcast) ) }

   full_transaction update_asset_feed_producers(string symbol,
                                                  flat_set<string> new_feed_producers,
                                                  bool broadcast /* = false */)
   { try {
      optional<asset_object> asset_to_update = find_asset(symbol);
      if (!asset_to_update)
        FC_THROW("No asset with that symbol exists!");

      asset_update_feed_producers_operation update_op;
      update_op.issuer = asset_to_update->issuer;
      update_op.asset_to_update = asset_to_update->id;
      update_op.new_feed_producers.reserve(new_feed_producers.size());
      std::transform(new_feed_producers.begin(), new_feed_producers.end(),
                     std::inserter(update_op.new_feed_producers, update_op.new_feed_producers.end()),
                     [this](const std::string& account_name_or_id){ return get_account_id(account_name_or_id); });

      signed_transaction tx;
      tx.operations.push_back( update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (symbol)(new_feed_producers)(broadcast) ) }

   full_transaction publish_asset_feed(string publishing_account,
                                         string symbol,
                                         price_feed feed,
                                         bool broadcast /* = false */)
   { try {
      optional<asset_object> asset_to_update = find_asset(symbol);
      if (!asset_to_update)
        FC_THROW("No asset with that symbol exists!");

      asset_publish_feed_operation publish_op;
      publish_op.publisher = get_account_id(publishing_account);
      publish_op.asset_id = asset_to_update->id;
      publish_op.feed = feed;

      signed_transaction tx;
      tx.operations.push_back( publish_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (publishing_account)(symbol)(feed)(broadcast) ) }


   signed_transaction publish_normal_asset_feed(string publishing_account,
	   string symbol,
	   price_feed feed,
	   bool broadcast /* = false */)
   {
	   try {
		   optional<asset_object> asset_to_update = find_asset(symbol);
		   if (!asset_to_update)
			   FC_THROW("No asset with that symbol exists!");

		   normal_asset_publish_feed_operation publish_op;
		   publish_op.publisher = get_account_id(publishing_account);
		   publish_op.publisher_addr = get_account_addr(publishing_account);
		   publish_op.asset_id = asset_to_update->id;
		   publish_op.feed = feed;

		   signed_transaction tx;
		   tx.operations.push_back(publish_op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();

		   return sign_transaction(tx, broadcast);
	   } FC_CAPTURE_AND_RETHROW((publishing_account)(symbol)(feed)(broadcast))
   }

   signed_transaction fund_asset_fee_pool(string from,
                                          string symbol,
                                          string amount,
                                          bool broadcast /* = false */)
   { try {
      account_object from_account = get_account(from);
      optional<asset_object> asset_to_fund = find_asset(symbol);
      if (!asset_to_fund)
        FC_THROW("No asset with that symbol exists!");
      asset_object core_asset = get_asset(asset_id_type());

      asset_fund_fee_pool_operation fund_op;
      fund_op.from_account = from_account.id;
      fund_op.asset_id = asset_to_fund->id;
      fund_op.amount = core_asset.amount_from_string(amount).amount;

      signed_transaction tx;
      tx.operations.push_back( fund_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (from)(symbol)(amount)(broadcast) ) }

   full_transaction reserve_asset(string from,
                                 string amount,
                                 string symbol,
                                 bool broadcast /* = false */)
   { try {
      account_object from_account = get_account(from);
      optional<asset_object> asset_to_reserve = find_asset(symbol);
      if (!asset_to_reserve)
        FC_THROW("No asset with that symbol exists!");

      asset_reserve_operation reserve_op;
      reserve_op.payer = from_account.id;
      reserve_op.amount_to_reserve = asset_to_reserve->amount_from_string(amount);

      signed_transaction tx;
      tx.operations.push_back( reserve_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (from)(amount)(symbol)(broadcast) ) }

   full_transaction global_settle_asset(string symbol,
                                          price settle_price,
                                          bool broadcast /* = false */)
   { try {
      optional<asset_object> asset_to_settle = find_asset(symbol);
      if (!asset_to_settle)
        FC_THROW("No asset with that symbol exists!");

      asset_global_settle_operation settle_op;
      settle_op.issuer = asset_to_settle->issuer;
      settle_op.asset_to_settle = asset_to_settle->id;
      settle_op.settle_price = settle_price;

      signed_transaction tx;
      tx.operations.push_back( settle_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (symbol)(settle_price)(broadcast) ) }

   full_transaction settle_asset(string account_to_settle,
                                   string amount_to_settle,
                                   string symbol,
                                   bool broadcast /* = false */)
   { try {
      optional<asset_object> asset_to_settle = find_asset(symbol);
      if (!asset_to_settle)
        FC_THROW("No asset with that symbol exists!");

      asset_settle_operation settle_op;
      settle_op.account = get_account_id(account_to_settle);
      settle_op.amount = asset_to_settle->amount_from_string(amount_to_settle);

      signed_transaction tx;
      tx.operations.push_back( settle_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account_to_settle)(amount_to_settle)(symbol)(broadcast) ) }

   full_transaction whitelist_account(string authorizing_account,
                                        string account_to_list,
                                        account_whitelist_operation::account_listing new_listing_status,
                                        bool broadcast /* = false */)
   { try {
      account_whitelist_operation whitelist_op;
      whitelist_op.authorizing_account = get_account_id(authorizing_account);
      whitelist_op.account_to_list = get_account_id(account_to_list);
      whitelist_op.new_listing = new_listing_status;

      signed_transaction tx;
      tx.operations.push_back( whitelist_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (authorizing_account)(account_to_list)(new_listing_status)(broadcast) ) }

   vector<optional<account_binding_object>> get_binding_account(const string& account,const string& symbol)const 
   {
	   try {

		   if (address::is_valid(account))
		   {
			   return _remote_db->get_binding_account(account, symbol);
		   }
		   auto acct = get_account(account);
		   FC_ASSERT(acct.addr != address());
		   return _remote_db->get_binding_account(acct.addr.address_to_string(), symbol);
	   }FC_CAPTURE_AND_RETHROW((account)(symbol))
   }


   full_transaction create_guard_member(string account, bool broadcast /* = false */)
   { try {
	  //account should be register in the blockchian
	  FC_ASSERT(!is_locked());
      guard_member_create_operation guard_member_create_op;
	  auto guard_member_account = get_account(account);
	  FC_ASSERT(account_object().get_id() != guard_member_account.get_id(),"account is not registered to the chain.");
	  guard_member_create_op.guard_member_account = guard_member_account.get_id();
	  guard_member_create_op.fee_pay_address = guard_member_account.addr;
	  guard_member_create_op.guarantee_id = get_guarantee_id();

	  signed_transaction tx;
	  tx.operations.push_back(guard_member_create_op);
      set_operation_fees( tx, get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account)(broadcast) ) }

   full_transaction resign_guard_member(string proposing_account, string account, int64_t expiration_time, bool broadcast)
   {
       try {
           //account should be register in the blockchian
           FC_ASSERT(!is_locked());
           guard_member_resign_operation op;
           auto guard_member_account = get_account_id(account);
           FC_ASSERT(account_object().get_id() != guard_member_account, "account is not registered to the chain.");
           op.guard_member_account = guard_member_account;

           const chain_parameters& current_params = get_global_properties().parameters;
           auto guard_create_op = operation(op);
           current_params.current_fees->set_fee(guard_create_op);

           signed_transaction tx;
           proposal_create_operation prop_op;
           prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
           prop_op.proposer = get_account(proposing_account).get_id();
           prop_op.fee_paying_account = get_account(proposing_account).addr;
           prop_op.proposed_ops.emplace_back(guard_create_op);
           //prop_op.review_period_seconds = 100;
           tx.operations.push_back(prop_op);
           set_operation_fees(tx, current_params.current_fees);
           tx.validate();

           return sign_transaction(tx, broadcast);
       } FC_CAPTURE_AND_RETHROW((proposing_account)(account)(expiration_time)(broadcast))
   }

   full_transaction update_guard_formal(string proposing_account, map<account_id_type, account_id_type> replace_queue, int64_t expiration_time,
	   bool broadcast /* = false */)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   guard_member_update_operation op;
		   auto guard_member_account = get_guard_member(proposing_account);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   op.replace_queue = replace_queue;
		   auto guard_update_op = operation(op);
		   current_params.current_fees->set_fee(guard_update_op);

		   signed_transaction tx;
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(proposing_account).get_id();
		   prop_op.fee_paying_account = get_account(proposing_account).addr;
		   prop_op.proposed_ops.emplace_back(guard_update_op);
		   //prop_op.type = vote_id_type::committee;
		   //prop_op.review_period_seconds = 0;
		   tx.operations.push_back(prop_op);
		   set_operation_fees(tx, current_params.current_fees);
		   tx.validate();

		   return sign_transaction(tx, broadcast);
		   

	   } FC_CAPTURE_AND_RETHROW((proposing_account)(expiration_time)(broadcast))
   }

   full_transaction referendum_accelerate_pledge(const referendum_id_type referendum_id, const string& amount, bool broadcast = true)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   referendum_accelerate_pledge_operation op;
		   auto obj = _remote_db->get_referendum_object(referendum_id);
		   FC_ASSERT(obj.valid(),"there is no this referendum.");
		   auto acc_obj = get_account(obj->proposer);
		   op.fee_paying_account = acc_obj.addr;
		   op.fee = get_asset(asset_id_type()).amount_from_string(amount);
		   op.referendum_id = referendum_id;
		   op.guarantee_id = get_guarantee_id();
		   signed_transaction tx;
		   tx.operations.push_back(op);
		   tx.validate();

		   return sign_transaction(tx,broadcast);
	   }FC_CAPTURE_AND_RETHROW((referendum_id)(amount)(broadcast))
   }
   full_transaction set_citizen_pledge_pay_back_rate(const string& citizen, int pledge_pay_back_rate,bool broadcast)
   {
	   try {

		   FC_ASSERT(!is_locked());
		   FC_ASSERT(pledge_pay_back_rate >= 0 && pledge_pay_back_rate <= 20, "pledge_pay_back_rate must between 0 20");
		   auto ctz = get_miner(citizen);
		   auto acct=get_account(citizen);
		   FC_ASSERT(acct.id == ctz.miner_account, "Invalid citizen:" + citizen);
		   acct.options.miner_pledge_pay_back = pledge_pay_back_rate;
		   account_update_operation op;
		   op.addr = acct.addr;
		   op.account = acct.id;
		   op.new_options = acct.options;
		   signed_transaction tx;
		   tx.operations.push_back(op);
		   set_operation_fees(tx, get_global_properties().parameters.current_fees);
		   tx.validate();

		   full_transaction res= sign_transaction(tx, broadcast);
		   _wallet.pending_account_updation[res.trxid] = acct.name;

		   //std::cout <<"new trx"<<fc::json::to_pretty_string(res.trxid) << std::endl;
		   //std::cout << fc::json::to_pretty_string(_wallet.pending_account_updation) << std::endl;
		   return res;

	   }FC_CAPTURE_AND_RETHROW((citizen)(pledge_pay_back_rate))
   }
   full_transaction citizen_referendum_for_senator(const string& citizen,const string& amount ,const map<account_id_type,account_id_type>& replacement,bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   get_miner(citizen);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   citizen_referendum_senator_operation op;
		   for (const auto& iter : replacement)
		   {
			  get_account(iter.first);
			  get_account(iter.second);
		   }
	
		   op.replace_queue = replacement;
		   signed_transaction tx;
		   referendum_create_operation prop_op;
		   prop_op.proposer = get_account(citizen).get_id();
		   prop_op.fee_paying_account = get_account(citizen).addr;
		   prop_op.proposed_ops.emplace_back(op);
		   current_params.current_fees->set_fee(prop_op.proposed_ops.front().op);
		   //prop_op.review_period_seconds = 0;
		   prop_op.fee = get_asset(asset_id_type()).amount_from_string(amount);
		   prop_op.guarantee_id = get_guarantee_id();
		   tx.operations.push_back(prop_op);
		   //set_operation_fees(tx, current_params.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((citizen)(replacement)(broadcast))
   }

   full_transaction guard_appointed_publisher(const string& account, const account_id_type publisher, const string& symbol, int64_t expiration_time,bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   publisher_appointed_operation op;
		   auto guard_member_account = get_guard_member(account);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   auto publisher_addr = get_account_addr(fc::variant(publisher).as_string());
		   op.publisher = publisher_addr;
		   op.asset_symbol = symbol;

		   auto publisher_appointed_op = operation(op);
		   current_params.current_fees->set_fee(publisher_appointed_op);

		   signed_transaction tx;
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(account).get_id();
		   prop_op.fee_paying_account = get_account(account).addr;
		   prop_op.proposed_ops.emplace_back(publisher_appointed_op);
		   tx.operations.push_back(prop_op);
		   set_operation_fees(tx, current_params.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(publisher)(symbol)(expiration_time)(broadcast))
   }
   full_transaction guard_cancel_publisher(const string& account, const account_id_type publisher, const string& symbol, int64_t expiration_time, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   publisher_canceled_operation op;
		   auto guard_member_account = get_guard_member(account);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   auto publisher_addr = get_account_addr(fc::variant(publisher).as_string());
		   op.publisher = publisher_addr;
		   op.asset_symbol = symbol;

		   auto publisher_appointed_op = operation(op);
		   current_params.current_fees->set_fee(publisher_appointed_op);

		   signed_transaction tx;
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(account).get_id();
		   prop_op.fee_paying_account = get_account(account).addr;
		   prop_op.proposed_ops.emplace_back(publisher_appointed_op);
		   tx.operations.push_back(prop_op);
		   set_operation_fees(tx, current_params.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(publisher)(symbol)(expiration_time)(broadcast))
   }

	full_transaction senator_change_eth_gas_price(const string& account, const string& gas_price, const string& symbol, int64_t expiration_time, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   senator_change_eth_gas_price_operation op;
		   auto guard_member_account = get_guard_member(account);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   op.new_gas_price = gas_price;
		   op.symbol = symbol;

		   auto publisher_appointed_op = operation(op);
		   current_params.current_fees->set_fee(publisher_appointed_op);

		   signed_transaction tx;
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(account).get_id();
		   prop_op.fee_paying_account = get_account(account).addr;
		   prop_op.proposed_ops.emplace_back(publisher_appointed_op);
		   //prop_op.type = vote_id_type::witness;
		   tx.operations.push_back(prop_op);
		   set_operation_fees(tx, current_params.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(gas_price)(symbol)(expiration_time)(broadcast))
   }
   full_transaction senator_appointed_crosschain_fee(const string& account, const share_type fee, const string& symbol, int64_t expiration_time, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   asset_fee_modification_operation op;
		   auto guard_member_account = get_guard_member(account);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   op.crosschain_fee = fee;
		   op.asset_symbol = symbol;

		   auto publisher_appointed_op = operation(op);
		   current_params.current_fees->set_fee(publisher_appointed_op);

		   signed_transaction tx;
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(account).get_id();
		   prop_op.fee_paying_account = get_account(account).addr;
		   prop_op.proposed_ops.emplace_back(publisher_appointed_op);
		   //prop_op.type = vote_id_type::witness;
		   tx.operations.push_back(prop_op);
		   set_operation_fees(tx, current_params.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(fee)(symbol)(expiration_time)(broadcast))
   }

   full_transaction senator_appointed_lockbalance_senator(const string& account, const std::map<string, asset>& lockbalance, int64_t expiration_time, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   set_guard_lockbalance_operation op;
		   auto guard_member_account = get_guard_member(account);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   op.lockbalance = lockbalance;

		   auto publisher_appointed_op = operation(op);
		   current_params.current_fees->set_fee(publisher_appointed_op);

		   signed_transaction tx;
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(account).get_id();
		   prop_op.fee_paying_account = get_account(account).addr;
		   prop_op.proposed_ops.emplace_back(publisher_appointed_op);
		   //prop_op.type = vote_id_type::witness;
		   tx.operations.push_back(prop_op);
		   set_operation_fees(tx, current_params.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(lockbalance)(expiration_time)(broadcast))
   }
   full_transaction senator_determine_block_payment(const string& account, const std::map<uint32_t, uint32_t>& blocks_pays, int64_t expiration_time, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   senator_determine_block_payment_operation op;
		   auto guard_member_account = get_guard_member(account);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   op.blocks_pairs = blocks_pays;
		   auto determine_op = operation(op);
		   current_params.current_fees->set_fee(determine_op);

		   signed_transaction tx;
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(account).get_id();
		   prop_op.fee_paying_account = get_account(account).addr;
		   prop_op.proposed_ops.emplace_back(determine_op);
		   tx.operations.push_back(prop_op);
		   set_operation_fees(tx, current_params.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(blocks_pays)(expiration_time)(broadcast))
   }
   full_transaction proposal_block_address(const string& account, const fc::flat_set<address>& block_addr, int64_t expiration_time, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   block_address_operation op;
		   auto guard_member_account = get_guard_member(account);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   op.blocked_address = block_addr;
		
		   signed_transaction tx;
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(account).get_id();
		   prop_op.fee_paying_account = get_account(account).addr;
		   prop_op.proposed_ops.emplace_back(op);
		   prop_op.type = vote_id_type::cancel_commit;
		   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
		   tx.operations.push_back(prop_op);
		   set_operation_fees(tx, current_params.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(block_addr)(expiration_time)(broadcast))
   }

   full_transaction proposal_cancel_block_address(const string& account, const fc::flat_set<address>& block_addr, int64_t expiration_time, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   cancel_address_block_operation op;
		   auto guard_member_account = get_guard_member(account);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   op.cancel_blocked_address = block_addr;

		   signed_transaction tx;
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(account).get_id();
		   prop_op.fee_paying_account = get_account(account).addr;
		   prop_op.proposed_ops.emplace_back(op);
		   prop_op.type = vote_id_type::cancel_commit;
		   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
		   tx.operations.push_back(prop_op);
		   set_operation_fees(tx, current_params.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(block_addr)(expiration_time)(broadcast))
   }

   full_transaction senator_determine_withdraw_deposit(const string& account, bool can,const string& symbol, int64_t expiration_time, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   senator_determine_withdraw_deposit_operation op;
		   auto guard_member_account = get_guard_member(account);
		   const chain_parameters& current_params = get_global_properties().parameters;
		   op.can = can;
		   op.symbol = symbol;
		   auto publisher_appointed_op = operation(op);
		   current_params.current_fees->set_fee(publisher_appointed_op);

		   signed_transaction tx;
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(account).get_id();
		   prop_op.fee_paying_account = get_account(account).addr;
		   prop_op.proposed_ops.emplace_back(publisher_appointed_op);
		   //prop_op.type = vote_id_type::witness;
		   tx.operations.push_back(prop_op);
		   set_operation_fees(tx, current_params.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(can)(expiration_time)(broadcast))
   }

   address create_multisignature_address(const string& account, const fc::flat_set<public_key_type>& pubs, int required, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   account_create_multisignature_address_operation op;
		   auto acc = get_account(account);
		   op.addr = acc.addr;
		   op.pubs = pubs;
		   op.required = required;
		   std::string temp="";
		   auto pubkey = fc::ecc::public_key();
		   FC_ASSERT(pubs.size() > 1, "there should be more than 2 pubkeys");
		   FC_ASSERT(pubs.size() <= 15, "more than 15 pubkeys.");
		   FC_ASSERT(required <= pubs.size() && required > 0 ,"required should be less than pubs size, but larger than 0.");
		   for (auto iter : pubs)
		   {
			   auto temp = iter.operator fc::ecc::public_key();
			   if (!pubkey.valid())
			   {
				   pubkey = temp;
			   }
			  pubkey = pubkey.add(fc::sha256::hash(temp));
		   }
		   pubkey=pubkey.add(fc::sha256::hash(required));
		   op.multisignature = address(pubkey,addressVersion::MULTISIG);
		   op.guarantee_id = get_guarantee_id();
		   signed_transaction tx;
		   tx.operations.push_back(op);
		   set_operation_fees(tx, get_global_properties().parameters.current_fees);
		   tx.validate();
		   sign_transaction(tx,broadcast);
		   return  op.multisignature;

	   }FC_CAPTURE_AND_RETHROW((account)(pubs)(required)(broadcast))
   }

   map<account_id_type, vector<asset>> get_citizen_lockbalance_info(const string& account)
   {
	   auto obj = get_miner(account);
	   return _remote_db->get_citizen_lockbalance_info(obj.id);
   }
   vector<optional< eth_multi_account_trx_object>> get_eth_multi_account_trx(const int & mul_acc_tx_state) {
	   eth_multi_account_trx_state temp = eth_multi_account_trx_state(mul_acc_tx_state);
	   return _remote_db->get_eths_multi_create_account_trx(temp,transaction_id_type());
   }
   miner_object get_miner(string owner_account)
   {
      try
      {
		 miner_object obj;
         fc::optional<miner_id_type> miner_id = maybe_id<miner_id_type>(owner_account);
         if (miner_id)
         {
            std::vector<miner_id_type> ids_to_get;
            ids_to_get.push_back(*miner_id);
            std::vector<fc::optional<miner_object>> miner_objects = _remote_db->get_miners(ids_to_get);
            if (miner_objects.front())
				obj= *miner_objects.front();
			else
			{
				FC_THROW("No witness is registered for id ${id}", ("id", owner_account));
			}
            
         }
         else
         {
            // then maybe it's the owner account
            try
            {
               account_id_type owner_account_id = get_account_id(owner_account);
               fc::optional<miner_object> witness = _remote_db->get_miner_by_account(owner_account_id);
               if (witness)
				   obj= *witness;
               else
                  FC_THROW("No witness is registered for account ${account}", ("account", owner_account));
            }
            catch (const fc::exception&)
            {
               FC_THROW("No account or witness named ${account}", ("account", owner_account));
            }
         }
		 return obj;
      }
      FC_CAPTURE_AND_RETHROW( (owner_account) )
   }

   guard_member_object get_guard_member(string owner_account)
   {
      try
      {
         fc::optional<guard_member_id_type> committee_member_id = maybe_id<guard_member_id_type>(owner_account);
         if (committee_member_id)
         {
            std::vector<guard_member_id_type> ids_to_get;
            ids_to_get.push_back(*committee_member_id);
            std::vector<fc::optional<guard_member_object>> guard_member_objects = _remote_db->get_guard_members(ids_to_get);
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
               fc::optional<guard_member_object> committee_member = _remote_db->get_guard_member_by_account(owner_account_id);
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
      FC_CAPTURE_AND_RETHROW( (owner_account) )
   }
   flat_set<miner_id_type> list_active_citizens()
   {
	   auto pro = get_global_properties();
	   return pro.active_witnesses;
   }


   full_transaction create_miner(string owner_account,
                                     string url,
                                     bool broadcast /* = false */)
   { try {
	   FC_ASSERT(!is_locked());
      account_object miner_account = get_account(owner_account);
      fc::ecc::private_key active_private_key = get_private_key_for_account(miner_account);
      //int witness_key_index = find_first_unused_derived_key_index(active_private_key);
      //fc::ecc::private_key witness_private_key = derive_private_key(key_to_wif(active_private_key), witness_key_index);
      graphene::chain::public_key_type miner_public_key = active_private_key.get_public_key();

      miner_create_operation miner_create_op;
	  miner_create_op.miner_account = miner_account.id;
	  miner_create_op.miner_address = miner_account.addr;
	  miner_create_op.block_signing_key = miner_public_key;
	  miner_create_op.url = url;
	  miner_create_op.guarantee_id = get_guarantee_id();
      if (_remote_db->get_miner_by_account(miner_create_op.miner_account))
         FC_THROW("Account ${owner_account} is already a miner", ("owner_account", owner_account));

      signed_transaction tx;
      tx.operations.push_back(miner_create_op);
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      _wallet.pending_miner_registrations[owner_account] = key_to_wif(active_private_key);

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (owner_account)(broadcast) ) }

   full_transaction update_witness(string witness_name,
                                     string url,
                                     string block_signing_key,
                                     bool broadcast /* = false */)
   { try {
      miner_object witness = get_miner(witness_name);
      account_object witness_account = get_account( witness.miner_account );
      fc::ecc::private_key active_private_key = get_private_key_for_account(witness_account);

      witness_update_operation witness_update_op;
      witness_update_op.witness = witness.id;
      witness_update_op.witness_account = witness_account.id;
      if( url != "" )
         witness_update_op.new_url = url;
      if( block_signing_key != "" )
         witness_update_op.new_signing_key = public_key_type( block_signing_key );

      signed_transaction tx;
      tx.operations.push_back( witness_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (witness_name)(url)(block_signing_key)(broadcast) ) }

   template<typename WorkerInit>
   static WorkerInit _create_worker_initializer( const variant& worker_settings )
   {
      WorkerInit result;
      from_variant( worker_settings, result );
      return result;
   }

   full_transaction create_worker(
      string owner_account,
      time_point_sec work_begin_date,
      time_point_sec work_end_date,
      share_type daily_pay,
      string name,
      string url,
      variant worker_settings,
      bool broadcast
      )
   {
      worker_initializer init;
      std::string wtype = worker_settings["type"].get_string();

      // TODO:  Use introspection to do this dispatch
      if( wtype == "burn" )
         init = _create_worker_initializer< burn_worker_initializer >( worker_settings );
      else if( wtype == "refund" )
         init = _create_worker_initializer< refund_worker_initializer >( worker_settings );
      else if( wtype == "vesting" )
         init = _create_worker_initializer< vesting_balance_worker_initializer >( worker_settings );
      else
      {
         FC_ASSERT( false, "unknown worker[\"type\"] value" );
      }

      worker_create_operation op;
      op.owner = get_account( owner_account ).id;
      op.work_begin_date = work_begin_date;
      op.work_end_date = work_end_date;
      op.daily_pay = daily_pay;
      op.name = name;
      op.url = url;
      op.initializer = init;

      signed_transaction tx;
      tx.operations.push_back( op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   }

   full_transaction update_worker_votes(
      string account,
      worker_vote_delta delta,
      bool broadcast
      )
   {
      account_object acct = get_account( account );
      account_update_operation op;

      // you could probably use a faster algorithm for this, but flat_set is fast enough :)
      flat_set< worker_id_type > merged;
      merged.reserve( delta.vote_for.size() + delta.vote_against.size() + delta.vote_abstain.size() );
      for( const worker_id_type& wid : delta.vote_for )
      {
         bool inserted = merged.insert( wid ).second;
         FC_ASSERT( inserted, "worker ${wid} specified multiple times", ("wid", wid) );
      }
      for( const worker_id_type& wid : delta.vote_against )
      {
         bool inserted = merged.insert( wid ).second;
         FC_ASSERT( inserted, "worker ${wid} specified multiple times", ("wid", wid) );
      }
      for( const worker_id_type& wid : delta.vote_abstain )
      {
         bool inserted = merged.insert( wid ).second;
         FC_ASSERT( inserted, "worker ${wid} specified multiple times", ("wid", wid) );
      }

      // should be enforced by FC_ASSERT's above
      assert( merged.size() == delta.vote_for.size() + delta.vote_against.size() + delta.vote_abstain.size() );

      vector< object_id_type > query_ids;
      for( const worker_id_type& wid : merged )
         query_ids.push_back( wid );

      flat_set<vote_id_type> new_votes( acct.options.votes );

      fc::variants objects = _remote_db->get_objects( query_ids );
      for( const variant& obj : objects )
      {
         worker_object wo;
         from_variant( obj, wo );
         new_votes.erase( wo.vote_for );
         new_votes.erase( wo.vote_against );
         if( delta.vote_for.find( wo.id ) != delta.vote_for.end() )
            new_votes.insert( wo.vote_for );
         else if( delta.vote_against.find( wo.id ) != delta.vote_against.end() )
            new_votes.insert( wo.vote_against );
         else
            assert( delta.vote_abstain.find( wo.id ) != delta.vote_abstain.end() );
      }

      account_update_operation update_op;
      update_op.account = acct.id;
      update_op.new_options = acct.options;
      update_op.new_options->votes = new_votes;

      signed_transaction tx;
      tx.operations.push_back( update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   }

   vector< vesting_balance_object_with_info > get_vesting_balances( string account_name )
   { try {
      fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>( account_name );
      std::vector<vesting_balance_object_with_info> result;
      fc::time_point_sec now = _remote_db->get_dynamic_global_properties().time;

      if( vbid )
      {
         result.emplace_back( get_object<vesting_balance_object>(*vbid), now );
         return result;
      }

      // try casting to avoid a round-trip if we were given an account ID
      fc::optional<account_id_type> acct_id = maybe_id<account_id_type>( account_name );
      if( !acct_id )
         acct_id = get_account( account_name ).id;

      vector< vesting_balance_object > vbos = _remote_db->get_vesting_balances( *acct_id );
      if( vbos.size() == 0 )
         return result;

      for( const vesting_balance_object& vbo : vbos )
         result.emplace_back( vbo, now );

      return result;
   } FC_CAPTURE_AND_RETHROW( (account_name) )
   }

   full_transaction withdraw_vesting(
      string witness_name,
      string amount,
      string asset_symbol,
      bool broadcast = false )
   { try {
      asset_object asset_obj = get_asset( asset_symbol );
      fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>(witness_name);
      if( !vbid )
      {
         miner_object wit = get_miner( witness_name );
         FC_ASSERT( wit.pay_vb );
         vbid = wit.pay_vb;
      }

      vesting_balance_object vbo = get_object< vesting_balance_object >( *vbid );
      vesting_balance_withdraw_operation vesting_balance_withdraw_op;

      vesting_balance_withdraw_op.vesting_balance = *vbid;
      vesting_balance_withdraw_op.owner = vbo.owner;
      vesting_balance_withdraw_op.amount = asset_obj.amount_from_string(amount);

      signed_transaction tx;
      tx.operations.push_back( vesting_balance_withdraw_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (witness_name)(amount) )
   }

   full_transaction cancel_cold_hot_uncreate_transaction(const string& proposer, const string& trxid, const int64_t& exception_time, bool broadcast) {
	   try {
		   FC_ASSERT(!is_locked());
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(exception_time);
		   prop_op.proposer = get_account(proposer).get_id();
		   prop_op.fee_paying_account = get_account(proposer).addr;
		   coldhot_cancel_transafer_transaction_operation coldhot_canecl_transfer_op;
		   coldhot_canecl_transfer_op.trx_id = transaction_id_type(trxid);

		   auto guard_obj = get_guard_member(proposer);
		   auto guard_id = guard_obj.guard_member_account;
		   auto guard_account_obj = get_account(guard_id);
		   coldhot_canecl_transfer_op.guard = guard_account_obj.addr;
		   coldhot_canecl_transfer_op.guard_id = guard_obj.id;
		   const chain_parameters& current_params = get_global_properties().parameters;
		   prop_op.proposed_ops.emplace_back(coldhot_canecl_transfer_op);
		   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
		   signed_transaction trx;
		   trx.operations.emplace_back(prop_op);
		   set_operation_fees(trx, current_params.current_fees);
		   trx.validate();
		   return sign_transaction(trx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((proposer)(trxid)(exception_time)(broadcast))

   }
   full_transaction refund_request(const string& refund_account, const string& txid, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   auto acc = get_account(refund_account);
		   auto addr = acc.addr;
		   FC_ASSERT(addr != address(), "wallet has no this account.");
		   guard_refund_balance_operation op;
		   op.refund_addr = addr;
		   op.txid = txid;

		   signed_transaction trx;
		   trx.operations.push_back(op);
		   set_operation_fees(trx, _remote_db->get_global_properties().parameters.current_fees);
		   trx.validate();

		   return sign_transaction(trx,broadcast);


          }FC_CAPTURE_AND_RETHROW((refund_account)(txid)(broadcast))
   }
   full_transaction refund_uncombined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	   FC_ASSERT(!is_locked());
	   FC_ASSERT(transaction_id_type(txid) != transaction_id_type(), "txid is not legal");
	   auto without_sign_trx = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_without_sign_trx_create, transaction_id_type(txid));
	   FC_ASSERT(without_sign_trx.size() == 1, "Transaction find error");
	   auto account = get_account(guard);
	   auto guard_obj = get_guard_member(guard);
	   auto addr = account.addr;
	   FC_ASSERT(addr != address(), "wallet doesnt has this account.");
	   FC_ASSERT(guard_obj.guard_member_account == account.id, "guard account error");
	   proposal_create_operation prop_op;
	   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
	   prop_op.proposer = get_account(guard).get_id();
	   prop_op.fee_paying_account = addr;
	   prop_op.type = vote_id_type::vote_type::cancel_commit;
	   guard_refund_crosschain_trx_operation guard_refund_op;
	   guard_refund_op.not_enough_sign_trx_id = transaction_id_type(txid);
	   guard_refund_op.guard_address = addr;
	   guard_refund_op.guard_id = guard_obj.id;
	   const chain_parameters& current_params = get_global_properties().parameters;
	   prop_op.proposed_ops.emplace_back(guard_refund_op);
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	   signed_transaction trx;
	   trx.operations.emplace_back(prop_op);
	   set_operation_fees(trx, current_params.current_fees);
	   trx.validate();

	   return sign_transaction(trx, broadcast);
   }
   
	full_transaction senator_pass_combined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	   FC_ASSERT(!is_locked());
	   FC_ASSERT(transaction_id_type(txid) != transaction_id_type(), "txid is not legal");
	   auto without_sign_trx = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_combine_trx_create, transaction_id_type(txid));
	   FC_ASSERT(without_sign_trx.size() == 1, "Transaction find error");
	   auto account = get_account(guard);
	   auto guard_obj = get_guard_member(guard);
	   FC_ASSERT(guard_obj.senator_type == PERMANENT,"Only Permanent senator can use this proposal");
	   auto addr = account.addr;
	   FC_ASSERT(addr != address(), "wallet doesnt has this account.");
	   FC_ASSERT(guard_obj.guard_member_account == account.id, "guard account error");
	   proposal_create_operation prop_op;
	   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
	   prop_op.proposer = get_account(guard).get_id();
	   prop_op.fee_paying_account = addr;
	   prop_op.type = vote_id_type::vote_type::cancel_commit;
	   senator_pass_success_trx_operation guard_refund_op;
	   guard_refund_op.pass_transaction_id = transaction_id_type(txid);
	   guard_refund_op.guard_address = addr;
	   guard_refund_op.guard_id = guard_obj.id;
	   const chain_parameters& current_params = get_global_properties().parameters;
	   prop_op.proposed_ops.emplace_back(guard_refund_op);
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	   signed_transaction trx;
	   trx.operations.emplace_back(prop_op);
	   set_operation_fees(trx, current_params.current_fees);
	   trx.validate();

	   return sign_transaction(trx, broadcast);
   }
	full_transaction cancel_eth_sign_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
		FC_ASSERT(!is_locked());
		FC_ASSERT(transaction_id_type(txid) != transaction_id_type(), "txid is not legal");
		auto without_sign_trx = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_eth_guard_need_sign, transaction_id_type(txid));
		FC_ASSERT(without_sign_trx.size() == 1, "Transaction find error");
		auto account = get_account(guard);
		auto guard_obj = get_guard_member(guard);
		auto addr = account.addr;
		FC_ASSERT(addr != address(), "wallet doesnt has this account.");
		FC_ASSERT(guard_obj.guard_member_account == account.id, "guard account error");
		proposal_create_operation prop_op;
		prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		prop_op.proposer = get_account(guard).get_id();
		prop_op.fee_paying_account = addr;
		prop_op.type = vote_id_type::vote_type::cancel_commit;
		eths_cancel_unsigned_transaction_operation guard_refund_op;
		guard_refund_op.cancel_trx_id = transaction_id_type(txid);
		guard_refund_op.guard_address = addr;
		guard_refund_op.guard_id = guard_obj.id;
		const chain_parameters& current_params = get_global_properties().parameters;
		prop_op.proposed_ops.emplace_back(guard_refund_op);
		current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
		signed_transaction trx;
		trx.operations.emplace_back(prop_op);
		set_operation_fees(trx, current_params.current_fees);
		trx.validate();

		return sign_transaction(trx, broadcast);
	}
   full_transaction refund_combined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	   FC_ASSERT(!is_locked());
	   FC_ASSERT(transaction_id_type(txid) != transaction_id_type(), "txid is not legal");
	   auto without_sign_trx = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_combine_trx_create, transaction_id_type(txid));
	   FC_ASSERT(without_sign_trx.size() == 1, "Transaction find error");
	   auto account = get_account(guard);
	   auto guard_obj = get_guard_member(guard);
	   auto addr = account.addr;
	   FC_ASSERT(addr != address(), "wallet doesnt has this account.");
	   FC_ASSERT(guard_obj.guard_member_account == account.id, "guard account error");
	   proposal_create_operation prop_op;
	   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
	   prop_op.proposer = get_account(guard).get_id();
	   prop_op.fee_paying_account = addr;
	   prop_op.type = vote_id_type::vote_type::cancel_commit;
	   guard_cancel_combine_trx_operation guard_refund_op;
	   guard_refund_op.fail_trx_id = transaction_id_type(txid);
	   guard_refund_op.guard_address = addr;
	   guard_refund_op.guard_id = guard_obj.id;
	   const chain_parameters& current_params = get_global_properties().parameters;
	   prop_op.proposed_ops.emplace_back(guard_refund_op);
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	   signed_transaction trx;
	   trx.operations.emplace_back(prop_op);
	   set_operation_fees(trx, current_params.current_fees);
	   trx.validate();

	   return sign_transaction(trx, broadcast);
   }

   full_transaction eth_cancel_fail_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	   FC_ASSERT(!is_locked());
	   FC_ASSERT(transaction_id_type(txid) != transaction_id_type(), "txid is not legal");
	   auto without_sign_trx = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_eth_guard_sign, transaction_id_type(txid));
	   FC_ASSERT(without_sign_trx.size() == 1, "Transaction find error");
	   auto account = get_account(guard);
	   auto guard_obj = get_guard_member(guard);
	   auto addr = account.addr;
	   FC_ASSERT(addr != address(), "wallet doesnt has this account.");
	   FC_ASSERT(guard_obj.guard_member_account == account.id, "guard account error");
	   proposal_create_operation prop_op;
	   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
	   prop_op.proposer = get_account(guard).get_id();
	   prop_op.fee_paying_account = addr;
	   prop_op.type = vote_id_type::vote_type::cancel_commit;
	   eth_cancel_fail_crosschain_trx_operation eth_cancel_op;
	   eth_cancel_op.fail_transaction_id = transaction_id_type(txid);
	   eth_cancel_op.guard_address = addr;
	   eth_cancel_op.guard_id = guard_obj.id;
	   const chain_parameters& current_params = get_global_properties().parameters;
	   prop_op.proposed_ops.emplace_back(eth_cancel_op);
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	   signed_transaction trx;
	   trx.operations.emplace_back(prop_op);
	   set_operation_fees(trx, current_params.current_fees);
	   trx.validate();

	   return sign_transaction(trx, broadcast);
   }
   full_transaction cancel_coldhot_eth_fail_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast = false) {
	   FC_ASSERT(!is_locked());
	   FC_ASSERT(transaction_id_type(txid) != transaction_id_type(), "txid is not legal");
	   auto without_sign_trx = _remote_db->get_coldhot_transaction(coldhot_trx_state::coldhot_eth_guard_sign, transaction_id_type(txid));
	   FC_ASSERT(without_sign_trx.size() == 1, "Transaction find error");
	   auto account = get_account(guard);
	   auto guard_obj = get_guard_member(guard);
	   auto addr = account.addr;
	   FC_ASSERT(addr != address(), "wallet doesnt has this account.");
	   FC_ASSERT(guard_obj.guard_member_account == account.id, "guard account error");
	   proposal_create_operation prop_op;
	   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
	   prop_op.proposer = get_account(guard).get_id();
	   prop_op.fee_paying_account = addr;
	   prop_op.type = vote_id_type::vote_type::cancel_commit;
	   eth_cancel_coldhot_fail_trx_operaion cancel_op;
	   cancel_op.fail_trx_id = transaction_id_type(txid);
	   cancel_op.guard = addr;
	   cancel_op.guard_id = guard_obj.id;
	   const chain_parameters& current_params = get_global_properties().parameters;
	   prop_op.proposed_ops.emplace_back(cancel_op);
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	   signed_transaction trx;
	   trx.operations.emplace_back(prop_op);
	   set_operation_fees(trx, current_params.current_fees);
	   trx.validate();

	   return sign_transaction(trx, broadcast);
   }
   full_transaction cancel_coldhot_uncombined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast = false) {
	   FC_ASSERT(!is_locked());
	   FC_ASSERT(transaction_id_type(txid) != transaction_id_type(), "txid is not legal");
	   auto without_sign_trx = _remote_db->get_coldhot_transaction(coldhot_trx_state::coldhot_without_sign_trx_create, transaction_id_type(txid));
	   FC_ASSERT(without_sign_trx.size() == 1, "Transaction find error");
	   auto account = get_account(guard);
	   auto guard_obj = get_guard_member(guard);
	   auto addr = account.addr;
	   FC_ASSERT(addr != address(), "wallet doesnt has this account.");
	   FC_ASSERT(guard_obj.guard_member_account == account.id, "guard account error");
	   proposal_create_operation prop_op;
	   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
	   prop_op.proposer = get_account(guard).get_id();
	   prop_op.fee_paying_account = addr;
	   coldhot_cancel_uncombined_trx_operaion cancel_op;
	   cancel_op.trx_id = transaction_id_type(txid);
	   cancel_op.guard = addr;
	   cancel_op.guard_id = guard_obj.id;
	   const chain_parameters& current_params = get_global_properties().parameters;
	   prop_op.proposed_ops.emplace_back(cancel_op);
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	   signed_transaction trx;
	   trx.operations.emplace_back(prop_op);
	   set_operation_fees(trx, current_params.current_fees);
	   trx.validate();

	   return sign_transaction(trx, broadcast);
   }
	full_transaction senator_pass_coldhot_combined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast = false) {
	   FC_ASSERT(!is_locked());
	   FC_ASSERT(transaction_id_type(txid) != transaction_id_type(), "txid is not legal");
	   auto without_sign_trx = _remote_db->get_coldhot_transaction(coldhot_trx_state::coldhot_combine_trx_create, transaction_id_type(txid));
	   FC_ASSERT(without_sign_trx.size() == 1, "Transaction find error");
	   auto account = get_account(guard);
	   auto guard_obj = get_guard_member(guard);
	   FC_ASSERT(guard_obj.senator_type == PERMANENT,"Only Permanent senator can use this proposal");
	   auto addr = account.addr;
	   FC_ASSERT(addr != address(), "wallet doesnt has this account.");
	   FC_ASSERT(guard_obj.guard_member_account == account.id, "guard account error");
	   proposal_create_operation prop_op;
	   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
	   prop_op.proposer = get_account(guard).get_id();
	   prop_op.fee_paying_account = addr;
	   prop_op.type = vote_id_type::vote_type::cancel_commit;
	   coldhot_pass_combine_trx_operation cancel_op;
	   cancel_op.pass_transaction_id = transaction_id_type(txid);
	   cancel_op.guard = addr;
	   cancel_op.guard_id = guard_obj.id;
	   const chain_parameters& current_params = get_global_properties().parameters;
	   prop_op.proposed_ops.emplace_back(cancel_op);
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	   signed_transaction trx;
	   trx.operations.emplace_back(prop_op);
	   set_operation_fees(trx, current_params.current_fees);
	   trx.validate();
	   return sign_transaction(trx, broadcast);
   }
   full_transaction cancel_coldhot_combined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast = false) {
	   FC_ASSERT(!is_locked());
	   FC_ASSERT(transaction_id_type(txid) != transaction_id_type(), "txid is not legal");
	   auto without_sign_trx = _remote_db->get_coldhot_transaction(coldhot_trx_state::coldhot_combine_trx_create, transaction_id_type(txid));
	   FC_ASSERT(without_sign_trx.size() == 1, "Transaction find error");
	   auto account = get_account(guard);
	   auto guard_obj = get_guard_member(guard);
	   auto addr = account.addr;
	   FC_ASSERT(addr != address(), "wallet doesnt has this account.");
	   FC_ASSERT(guard_obj.guard_member_account == account.id, "guard account error");
	   proposal_create_operation prop_op;
	   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
	   prop_op.proposer = get_account(guard).get_id();
	   prop_op.fee_paying_account = addr;
	   prop_op.type = vote_id_type::vote_type::cancel_commit;
	   coldhot_cancel_combined_trx_operaion cancel_op;
	   cancel_op.fail_trx_id = transaction_id_type(txid);
	   cancel_op.guard = addr;
	   cancel_op.guard_id = guard_obj.id;
	   const chain_parameters& current_params = get_global_properties().parameters;
	   prop_op.proposed_ops.emplace_back(cancel_op);
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	   signed_transaction trx;
	   trx.operations.emplace_back(prop_op);
	   set_operation_fees(trx, current_params.current_fees);
	   trx.validate();

	   return sign_transaction(trx, broadcast);
   }
   full_transaction update_asset_private_keys(const string& from_account, const string& symbol,const string& out_key_file,const string& encrypt_key, bool broadcast,bool use_brain_key=false)
   {
	   try {
		   FC_ASSERT(!is_locked());

		   auto guard_account = get_guard_member(from_account);
		   FC_ASSERT(guard_account.guard_member_account != account_id_type(),"only guard member can do this operation.");
		   auto asset_id = get_asset_id(symbol);
		   auto  hot_keys =create_crosschain_symbol(symbol+"|etguard",false,use_brain_key);
		   //string hot_pri = cross_interface->export_private_key(symbol, "");
		   auto cold_keys = create_crosschain_symbol(symbol + "|etguard",true, use_brain_key);

		   //auto encrypted = fc::aes_encrypt(fc::sha512(encrypt_key.c_str(), encrypt_key.length()), plain_txt);
		   //
		   //fc::ofstream outfile{ fc::path(out_key_file) };
		   //outfile.write(encrypted.data(), encrypted.size());
		   //outfile.flush();
		   //outfile.close();
		   string tmpf=out_key_file+".tmp";
		   try {
			   boost::filesystem::remove(tmpf);
		   }
		   catch (boost::filesystem::filesystem_error& )
		   {

		   }
		   
		   boost::system::error_code ercode;
		   map<string, crosschain_prkeys> keys;
		   std::ifstream in(out_key_file, std::ios::in | std::ios::binary);
		   if (in.is_open())
		   {

			   std::vector<char> key_file_data((std::istreambuf_iterator<char>(in)),
				   (std::istreambuf_iterator<char>()));
			   in.close();
			   boost::filesystem::copy_file(out_key_file, tmpf);
			   if (key_file_data.size() > 0)
			   {
				   const auto plain_text = fc::aes_decrypt(fc::sha512(encrypt_key.c_str(), encrypt_key.length()), key_file_data);
				   keys = fc::raw::unpack<map<string, crosschain_prkeys>>(plain_text);
			   }
		   }
		   keys[symbol+cold_keys.addr] = cold_keys;
		   std::ofstream out(out_key_file, std::ios::out | std::ios::binary|std::ios::trunc);
		   auto plain_txt = fc::raw::pack(keys);
		   auto encrypted = fc::aes_encrypt(fc::sha512(encrypt_key.c_str(), encrypt_key.length()), plain_txt);
		   out.write(encrypted.data(), encrypted.size());
		   out.flush();
		   out.close();
		   //output check
		   std::ifstream out_chk(out_key_file, std::ios::in | std::ios::binary);
		   FC_ASSERT(out_chk.is_open(),"keyfile check failed!Open key file  failed!");
		   std::vector<char> key_file_data_chk((std::istreambuf_iterator<char>(out_chk)),
			   (std::istreambuf_iterator<char>()));
		   FC_ASSERT(key_file_data_chk.size() > 0, "key file shuld not be empty");
		   const auto plain_text_chk = fc::aes_decrypt(fc::sha512(encrypt_key.c_str(), encrypt_key.length()), key_file_data_chk);
		   map<string, crosschain_prkeys> keys_chk = fc::raw::unpack<map<string, crosschain_prkeys>>(plain_text_chk);
		   FC_ASSERT(keys == keys_chk,"Key file check faild!");

		   //output check end
		   account_multisig_create_operation op;
		   op.addr = get_account(guard_account.guard_member_account).addr;
		   op.account_id = get_account(guard_account.guard_member_account).get_id();
		   op.new_address_cold = cold_keys.addr;
		   op.new_pubkey_cold = cold_keys.pubkey;
		   op.new_address_hot = hot_keys.addr;
		   op.new_pubkey_hot = hot_keys.pubkey;
		   op.crosschain_type = symbol;
		   FC_ASSERT(_keys.find(op.addr) != _keys.end(),"there is no privatekey of ${addr}",("addr",op.addr));
		   fc::optional<fc::ecc::private_key> key = wif_to_key(_keys[op.addr]);
		   op.signature = key->sign_compact(fc::sha256::hash(op.new_address_hot + op.new_address_cold));

		   signed_transaction trx;
		   trx.operations.emplace_back(op);
		   set_operation_fees(trx, get_global_properties().parameters.current_fees);
		   trx.validate();
		   auto res= sign_transaction(trx,broadcast);
		   boost::filesystem::remove(tmpf);
		   return res;

	   }FC_CAPTURE_AND_RETHROW((from_account)(symbol)(broadcast))
   }
   vector<optional<multisig_address_object>> get_multi_account_guard(const string & multi_address, const string& symbol) {
	   get_asset_id(symbol);
	   return _remote_db->get_multi_account_guard(multi_address,symbol);
   }
   full_transaction transfer_from_cold_to_hot(const string& proposer, const string& from_account, const string& to_account, const string& amount, const string& asset_symbol,const string& memo,const int64_t& expiration_time, bool broadcast)
   {
	   try
	   {
		   FC_ASSERT(!is_locked());
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(proposer).get_id();
		   prop_op.fee_paying_account = get_account(proposer).addr;
		   coldhot_transfer_operation coldhot_transfer_op;
		   
		   coldhot_transfer_op.multi_account_withdraw = from_account;
		   coldhot_transfer_op.multi_account_deposit = to_account;
		   auto asset_obj = get_asset(asset_symbol);
		   
		   auto float_pos = amount.find('.');
		   string temp_amount = amount;
		   if (float_pos != amount.npos) {
			   auto float_temp = amount.substr(float_pos + 1, asset_obj.precision);
			   temp_amount = amount.substr(0, float_pos + 1) + float_temp;
		   }
		   fc::variant amount_fc = temp_amount;
		   char temp[1024];
		   std::sprintf(temp, "%.8f", amount_fc.as_double());
		   coldhot_transfer_op.amount = graphene::utilities::remove_zero_for_str_amount(temp);
		   
		   coldhot_transfer_op.asset_symbol = asset_symbol;
		   coldhot_transfer_op.memo = memo;
		   auto guard_obj = get_guard_member(proposer);
		   auto guard_id = guard_obj.guard_member_account;
		   auto guard_account_obj = get_account(guard_id);
		   coldhot_transfer_op.guard = guard_account_obj.addr;
		   coldhot_transfer_op.guard_id = guard_obj.id;
		  
		   coldhot_transfer_op.asset_id = asset_obj.id;
		   const chain_parameters& current_params = get_global_properties().parameters;
		   prop_op.proposed_ops.emplace_back(coldhot_transfer_op);
		   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
		   signed_transaction trx;
		   trx.operations.emplace_back(prop_op);
		   set_operation_fees(trx, current_params.current_fees);
		   trx.validate();

		   return sign_transaction(trx, broadcast);		   
	   }FC_CAPTURE_AND_RETHROW((proposer)(from_account)(to_account)(amount)(asset_symbol)(memo)(expiration_time)(broadcast))
   }

   vector<multisig_asset_transfer_object> get_multisig_asset_tx()
   {
	   try {
		   return _remote_db->get_multisigs_trx();
	   }FC_CAPTURE_AND_RETHROW()
   }
   vector<optional<multisig_address_object>> get_multi_address_obj(const string symbol,const account_id_type& guard) const
   {
	   try {
		   return _remote_db->get_multisig_address_obj(symbol,guard);
	   }FC_CAPTURE_AND_RETHROW()
   }
  
   multisig_asset_transfer_object get_multisig_asset_tx(multisig_asset_transfer_id_type id)
   {
	   try {
		   auto obj = _remote_db->lookup_multisig_asset(id);
		   FC_ASSERT(obj.valid());
		   return *obj;
	   }FC_CAPTURE_AND_RETHROW((id))
   }
   
   full_transaction sign_multi_asset_trx(const string& account, multisig_asset_transfer_id_type id,const string& guard, bool broadcast)
   {
	   try {
		   
		   FC_ASSERT(!is_locked());
		   const auto& acct = get_account(account);
		   const auto multisig_trx_obj = get_multisig_asset_tx(id);
		   
		   sign_multisig_asset_operation op;
		   guard_member_object guard_obj = get_guard_member(guard);
		   vector<optional<multisig_address_object>> multisig_obj = _remote_db->get_multisig_address_obj(multisig_trx_obj.chain_type,guard_obj.guard_member_account);
		   FC_ASSERT(multisig_obj[0].valid());
		   string config = (*_crosschain_manager)->get_config();
		   auto crosschain = crosschain::crosschain_manager::get_instance().get_crosschain_handle(multisig_trx_obj.chain_type);
		   crosschain->initialize_config(fc::json::from_string(config).get_object());
		   auto signature = "";// crosschain->sign_multisig_transaction(multisig_trx_obj.trx,multisig_obj[0]->new_address_hot,"");
		   op.signature = signature;
		   op.addr = acct.addr;
		   op.multisig_trx_id = multisig_trx_obj.id;
		   signed_transaction trx;
		   trx.operations.emplace_back(op);
		   
		   set_operation_fees(trx, get_global_properties().parameters.current_fees);
		   trx.validate();
		   return sign_transaction(trx,broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(id)(broadcast))
   }

   full_transaction account_change_for_crosschain(const string& proposer,const string& symbol,const string& hot,const string& cold, int64_t expiration_time,bool broadcast)
   {
	   try 
	   {
		   
		   FC_ASSERT(!is_locked());
		   proposal_create_operation prop_op;
		   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
		   prop_op.proposer = get_account(proposer).get_id();
		   prop_op.fee_paying_account = get_account(proposer).addr;

		   guard_update_multi_account_operation update_op;
		   const auto asset_id = get_asset_id(symbol);
		   update_op.chain_type = symbol;
		   string config = (*_crosschain_manager)->get_config();
		   FC_ASSERT((*_crosschain_manager)->contain_symbol(symbol), "no this plugin");
		   auto crosschain = crosschain::crosschain_manager::get_instance().get_crosschain_handle(symbol);
		   crosschain->initialize_config(fc::json::from_string(config).get_object());
		   
		   update_op.cold = cold;
		   update_op.hot = hot;

		   const chain_parameters& current_params = get_global_properties().parameters;
		   prop_op.proposed_ops.emplace_back(update_op);
		   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);

		   signed_transaction trx;
		   trx.operations.emplace_back(prop_op);
		   set_operation_fees(trx, current_params.current_fees);
		   trx.validate();

		   return sign_transaction(trx,broadcast);
		   
	   }FC_CAPTURE_AND_RETHROW((proposer)(symbol)(hot)(cold)(expiration_time)(broadcast))
   }
   
   full_transaction withdraw_from_link(const string& account, const string& symbol, int64_t amount, bool broadcast = true)
   {
	   try
	   {
		   FC_ASSERT(!is_locked());

		   signed_transaction trx;
		   trx.validate();

		   return sign_transaction(trx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account)(symbol)(amount)(broadcast))
   }
   
   full_transaction bind_tunnel_account(const string& link_account, const string& tunnel_account, const string& symbol, bool broadcast = true)
   {
	   try
	   {
		   FC_ASSERT(!is_locked());
		   account_bind_operation op;
		   auto acct_obj = get_account(link_account);
		   if (symbol.find("ERC") != symbol.npos) {
			   auto eth_bind_account = get_binding_account(acct_obj.addr.address_to_string(), "ETH");
			   FC_ASSERT(eth_bind_account.size() != 0, "Must bind eth tunnel account first");
			   std::string eth_tunnel_account = eth_bind_account.at(0)->bind_account;
			   FC_ASSERT(eth_tunnel_account == tunnel_account, "erc tunnel account must consistent with eth tunnel account");
		   }
		   op.addr = acct_obj.addr;
		   op.crosschain_type = symbol;
		   op.tunnel_address = tunnel_account;
		   FC_ASSERT(_keys.find(acct_obj.addr) != _keys.end(), "there is no privatekey of ${addr}", ("addr", acct_obj.addr));
		   fc::optional<fc::ecc::private_key> key = wif_to_key(_keys[acct_obj.addr]);
		   op.account_signature = key->sign_compact(fc::sha256::hash(acct_obj.addr));
		   op.guarantee_id = get_guarantee_id();
		   string config = (*_crosschain_manager)->get_config();
		   FC_ASSERT((*_crosschain_manager)->contain_symbol(symbol), "no this plugin");
		   auto crosschain = crosschain::crosschain_manager::get_instance().get_crosschain_handle(symbol);
		   crosschain->initialize_config(fc::json::from_string(config).get_object());
		   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(symbol);
		   FC_ASSERT(_crosschain_keys.count(tunnel_account)>0, "private key doesnt belong to this wallet.");
		   auto wif_key = _crosschain_keys[tunnel_account].wif_key;
		   auto key_ptr = prk_ptr->import_private_key(wif_key);
		   FC_ASSERT(key_ptr.valid());
		   prk_ptr->set_key(*key_ptr);

		   crosschain->create_signature(prk_ptr, tunnel_account, op.tunnel_signature);
		   signed_transaction trx;
		   trx.operations.emplace_back(op);
		   set_operation_fees(trx, _remote_db->get_global_properties().parameters.current_fees);
		   trx.validate();
		   
		   return sign_transaction(trx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((link_account)(tunnel_account)(symbol)(broadcast))
   }

   full_transaction unbind_tunnel_account(const string& link_account, const string& tunnel_account, const string& symbol, bool broadcast = true)
   {
	   try
	   {
		   FC_ASSERT(!is_locked());
		   account_unbind_operation op;
		   auto acct_obj = get_account(link_account);
		   op.addr = acct_obj.addr;
		   op.crosschain_type = symbol;
		   op.tunnel_address = tunnel_account;
		   FC_ASSERT(_keys.find(acct_obj.addr) != _keys.end(), "there is no privatekey of ${addr}", ("addr", acct_obj.addr));
		   fc::optional<fc::ecc::private_key> key = wif_to_key(_keys[acct_obj.addr]);
		   op.account_signature = key->sign_compact(fc::sha256::hash(acct_obj.addr));
		   op.guarantee_id = get_guarantee_id();
		   string config = (*_crosschain_manager)->get_config();
		   FC_ASSERT((*_crosschain_manager)->contain_symbol(symbol),"no this plugin");
		   auto crosschain = crosschain::crosschain_manager::get_instance().get_crosschain_handle(symbol);
		   crosschain->initialize_config(fc::json::from_string(config).get_object());

		   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(symbol);
		   FC_ASSERT(_crosschain_keys.count(tunnel_account)>0, "private key doesnt belong to this wallet.");
		   auto wif_key = _crosschain_keys[tunnel_account].wif_key;
		   auto key_ptr = prk_ptr->import_private_key(wif_key);
		   FC_ASSERT(key_ptr.valid());
		   prk_ptr->set_key(*key_ptr);

		   crosschain->create_signature(prk_ptr, tunnel_account, op.tunnel_signature);
		   signed_transaction trx;
		   trx.operations.emplace_back(op);
		   set_operation_fees(trx, _remote_db->get_global_properties().parameters.current_fees);
		   trx.validate();
		   return sign_transaction(trx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((link_account)(tunnel_account)(symbol)(broadcast))
   }
   vector<optional<multisig_account_pair_object>> get_multisig_account_pair(const string& symbol) const
   {
	   auto obj = get_asset(symbol);
	   return _remote_db->get_multisig_account_pair(symbol);
   }
   optional<multisig_account_pair_object> get_multisig_account_pair(const multisig_account_pair_id_type & id) const
   {
	   return _remote_db->lookup_multisig_account_pair(id);
   }

   full_transaction vote_for_committee_member(string voting_account,
                                        string committee_member,
                                        bool approve,
                                        bool broadcast /* = false */)
   { try {
      account_object voting_account_object = get_account(voting_account);
      account_id_type committee_member_owner_account_id = get_account_id(committee_member);
      fc::optional<guard_member_object> committee_member_obj = _remote_db->get_guard_member_by_account(committee_member_owner_account_id);
      if (!committee_member_obj)
         FC_THROW("Account ${committee_member} is not registered as a committee_member", ("committee_member", committee_member));
      if (approve)
      {
         auto insert_result = voting_account_object.options.votes.insert(committee_member_obj->vote_id);
         if (!insert_result.second)
            FC_THROW("Account ${account} was already voting for committee_member ${committee_member}", ("account", voting_account)("committee_member", committee_member));
      }
      else
      {
         unsigned votes_removed = voting_account_object.options.votes.erase(committee_member_obj->vote_id);
         if (!votes_removed)
            FC_THROW("Account ${account} is already not voting for committee_member ${committee_member}", ("account", voting_account)("committee_member", committee_member));
      }
      account_update_operation account_update_op;
      account_update_op.account = voting_account_object.id;
      account_update_op.new_options = voting_account_object.options;

      signed_transaction tx;
      tx.operations.push_back( account_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (voting_account)(committee_member)(approve)(broadcast) ) }

   full_transaction vote_for_witness(string voting_account,
                                        string witness,
                                        bool approve,
                                        bool broadcast /* = false */)
   { try {
      account_object voting_account_object = get_account(voting_account);
      account_id_type witness_owner_account_id = get_account_id(witness);
      fc::optional<miner_object> witness_obj = _remote_db->get_miner_by_account(witness_owner_account_id);
      if (!witness_obj)
         FC_THROW("Account ${witness} is not registered as a witness", ("witness", witness));
      if (approve)
      {
         auto insert_result = voting_account_object.options.votes.insert(witness_obj->vote_id);
         if (!insert_result.second)
            FC_THROW("Account ${account} was already voting for witness ${witness}", ("account", voting_account)("witness", witness));
      }
      else
      {
         unsigned votes_removed = voting_account_object.options.votes.erase(witness_obj->vote_id);
         if (!votes_removed)
            FC_THROW("Account ${account} is already not voting for witness ${witness}", ("account", voting_account)("witness", witness));
      }
      account_update_operation account_update_op;
      account_update_op.account = voting_account_object.id;
      account_update_op.new_options = voting_account_object.options;

      signed_transaction tx;
      tx.operations.push_back( account_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (voting_account)(witness)(approve)(broadcast) ) }

   full_transaction set_voting_proxy(string account_to_modify,
                                       optional<string> voting_account,
                                       bool broadcast /* = false */)
   { try {
      account_object account_object_to_modify = get_account(account_to_modify);
      if (voting_account)
      {
         account_id_type new_voting_account_id = get_account_id(*voting_account);
         if (account_object_to_modify.options.voting_account == new_voting_account_id)
            FC_THROW("Voting proxy for ${account} is already set to ${voter}", ("account", account_to_modify)("voter", *voting_account));
         account_object_to_modify.options.voting_account = new_voting_account_id;
      }
      else
      {
         if (account_object_to_modify.options.voting_account == GRAPHENE_PROXY_TO_SELF_ACCOUNT)
            FC_THROW("Account ${account} is already voting for itself", ("account", account_to_modify));
         account_object_to_modify.options.voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;
      }

      account_update_operation account_update_op;
      account_update_op.account = account_object_to_modify.id;
      account_update_op.new_options = account_object_to_modify.options;

      signed_transaction tx;
      tx.operations.push_back( account_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account_to_modify)(voting_account)(broadcast) ) }

   full_transaction set_desired_miner_and_guard_member_count(string account_to_modify,
                                                             uint16_t desired_number_of_witnesses,
                                                             uint16_t desired_number_of_committee_members,
                                                             bool broadcast /* = false */)
   { try {
      account_object account_object_to_modify = get_account(account_to_modify);

      if (account_object_to_modify.options.num_witness == desired_number_of_witnesses &&
          account_object_to_modify.options.num_committee == desired_number_of_committee_members)
         FC_THROW("Account ${account} is already voting for ${witnesses} witnesses and ${committee_members} committee_members",
                  ("account", account_to_modify)("witnesses", desired_number_of_witnesses)("committee_members",desired_number_of_witnesses));
      account_object_to_modify.options.num_witness = desired_number_of_witnesses;
      account_object_to_modify.options.num_committee = desired_number_of_committee_members;

      account_update_operation account_update_op;
      account_update_op.account = account_object_to_modify.id;
      account_update_op.new_options = account_object_to_modify.options;

      signed_transaction tx;
      tx.operations.push_back( account_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account_to_modify)(desired_number_of_witnesses)(desired_number_of_committee_members)(broadcast) ) }

   signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false, bool ignore_error_log=false)
   {
	   
	   
      flat_set<account_id_type> req_active_approvals;
      flat_set<account_id_type> req_owner_approvals;
      vector<authority>         other_auths;

      tx.get_required_authorities( req_active_approvals, req_owner_approvals, other_auths );
	  /*
      for( const auto& auth : other_auths )
         for( const auto& a : auth.account_auths )
            req_active_approvals.insert(a.first);

      // std::merge lets us de-duplicate account_id's that occur in both
      //   sets, and dump them into a vector (as required by remote_db api)
      //   at the same time
      vector<account_id_type> v_approving_account_ids;
      std::merge(req_active_approvals.begin(), req_active_approvals.end(),
                 req_owner_approvals.begin() , req_owner_approvals.end(),
                 std::back_inserter(v_approving_account_ids));

      /// TODO: fetch the accounts specified via other_auths as well.

      vector< optional<account_object> > approving_account_objects =
            _remote_db->get_accounts( v_approving_account_ids );

      /// TODO: recursively check one layer deeper in the authority tree for keys

      FC_ASSERT( approving_account_objects.size() == v_approving_account_ids.size() );

      flat_map<account_id_type, account_object*> approving_account_lut;
      size_t i = 0;
      for( optional<account_object>& approving_acct : approving_account_objects )
      {
         if( !approving_acct.valid() )
         {
            wlog( "operation_get_required_auths said approval of non-existing account ${id} was needed",
                  ("id", v_approving_account_ids[i]) );
            i++;
            continue;
         }
         approving_account_lut[ approving_acct->id ] = &(*approving_acct);
         i++;
      }

      flat_set<public_key_type> approving_key_set;
      for( account_id_type& acct_id : req_active_approvals )
      {
         const auto it = approving_account_lut.find( acct_id );
         if( it == approving_account_lut.end() )
            continue;
         const account_object* acct = it->second;
         vector<public_key_type> v_approving_keys = acct->active.get_keys();
         for( const public_key_type& approving_key : v_approving_keys )
            approving_key_set.insert( approving_key );
      }
      for( account_id_type& acct_id : req_owner_approvals )
      {
         const auto it = approving_account_lut.find( acct_id );
         if( it == approving_account_lut.end() )
            continue;
         const account_object* acct = it->second;
         vector<public_key_type> v_approving_keys = acct->owner.get_keys();
         for( const public_key_type& approving_key : v_approving_keys )
            approving_key_set.insert( approving_key );
      }
      for( const authority& a : other_auths )
      {
         for( const auto& k : a.key_auths )
            approving_key_set.insert( k.first );
      }

      auto dyn_props = get_dynamic_global_properties();
      tx.set_reference_block( dyn_props.head_block_id );

      // first, some bookkeeping, expire old items from _recently_generated_transactions
      // since transactions include the head block id, we just need the index for keeping transactions unique
      // when there are multiple transactions in the same block.  choose a time period that should be at
      // least one block long, even in the worst case.  2 minutes ought to be plenty.
      fc::time_point_sec oldest_transaction_ids_to_track(dyn_props.time - fc::minutes(2));
      auto oldest_transaction_record_iter = _recently_generated_transactions.get<timestamp_index>().lower_bound(oldest_transaction_ids_to_track);
      auto begin_iter = _recently_generated_transactions.get<timestamp_index>().begin();
      _recently_generated_transactions.get<timestamp_index>().erase(begin_iter, oldest_transaction_record_iter);
	  */

      uint32_t expiration_time_offset = 0;
	  auto dyn_props = get_dynamic_global_properties();
	  tx.set_reference_block(dyn_props.head_block_id);
	  flat_set<address> approving_key_set;
	  for (const authority& a : other_auths)
	  {
		  for (const auto& k : a.address_auths)
			  approving_key_set.insert(k.first);
	  }
      for (;;)
      {
         tx.set_expiration( dyn_props.time + fc::seconds(600 + expiration_time_offset) );
         tx.signatures.clear();

         for( auto& addr :  approving_key_set)
         {
            auto it = _keys.find(addr);
			FC_ASSERT(it != _keys.end(),"No private key in this wallet.");
            //if( it != _keys.end() )
            {
               fc::optional<fc::ecc::private_key> privkey = wif_to_key( it->second );
               FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
               tx.sign( *privkey, _chain_id );
            }
            /// TODO: if transaction has enough signatures to be "valid" don't add any more,
            /// there are cases where the wallet may have more keys than strictly necessary and
            /// the transaction will be rejected if the transaction validates without requiring
            /// all signatures provided
         }
         graphene::chain::transaction_id_type this_transaction_id = tx.id();
         auto iter = _recently_generated_transactions.find(this_transaction_id);
         if (iter == _recently_generated_transactions.end())
         {
            // we haven't generated this transaction before, the usual case
            recently_generated_transaction_record this_transaction_record;
            this_transaction_record.generation_time = dyn_props.time;
            this_transaction_record.transaction_id = this_transaction_id;
            _recently_generated_transactions.insert(this_transaction_record);
            break;
         }

         // else we've generated a dupe, increment expiration time and re-sign it
         expiration_time_offset+=10;
      }
	  FC_ASSERT(tx.signatures.size()!=0);
      if( broadcast )
      {
         try
         {
            _remote_net_broadcast->broadcast_transaction( tx );
         }
         catch (const fc::exception& e)
         {
			if(!ignore_error_log)
				elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
            throw;
         }
      }
      return tx;
   }

   full_transaction sell_asset(string seller_account,
                                 string amount_to_sell,
                                 string symbol_to_sell,
                                 string min_to_receive,
                                 string symbol_to_receive,
                                 uint32_t timeout_sec = 0,
                                 bool   fill_or_kill = false,
                                 bool   broadcast = false)
   {
      account_object seller   = get_account( seller_account );

      limit_order_create_operation op;
      op.seller = seller.id;
      op.amount_to_sell = get_asset(symbol_to_sell).amount_from_string(amount_to_sell);
      op.min_to_receive = get_asset(symbol_to_receive).amount_from_string(min_to_receive);
      if( timeout_sec )
         op.expiration = fc::time_point::now() + fc::seconds(timeout_sec);
      op.fill_or_kill = fill_or_kill;

      signed_transaction tx;
      tx.operations.push_back(op);
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   }

   full_transaction borrow_asset(string seller_name, string amount_to_borrow, string asset_symbol,
                                       string amount_of_collateral, bool broadcast = false)
   {
      account_object seller = get_account(seller_name);
      asset_object mia = get_asset(asset_symbol);
      FC_ASSERT(mia.is_market_issued());
      asset_object collateral = get_asset(get_object(*mia.bitasset_data_id).options.short_backing_asset);

      call_order_update_operation op;
      op.funding_account = seller.id;
      op.delta_debt   = mia.amount_from_string(amount_to_borrow);
      op.delta_collateral = collateral.amount_from_string(amount_of_collateral);

      signed_transaction trx;
      trx.operations = {op};
      set_operation_fees( trx, _remote_db->get_global_properties().parameters.current_fees);
      trx.validate();
      idump((broadcast));

      return sign_transaction(trx, broadcast);
   }
   std::vector<crosschain_trx_object> get_account_crosschain_transaction(string account_address, string trx_id) {
	   std::vector<crosschain_trx_object> resluts;
	   if (trx_id == "")
	   {
		   auto temp = address(account_address);
		   auto crosschain_all = _remote_db->get_account_crosschain_transaction(account_address, transaction_id_type());
		   if (trx_id == "") {
			   return crosschain_all;
		   }
	   }
	   else if ((account_address == "") && (transaction_id_type(trx_id) != transaction_id_type())){
		   return _remote_db->get_account_crosschain_transaction(account_address, transaction_id_type(trx_id));
	   }
	   return resluts;
   }
   std::map<transaction_id_type, signed_transaction>get_coldhot_transaction(const int & type) {
	   std::map<transaction_id_type, signed_transaction> results;
	   std::vector<coldhot_transfer_object> coldhot_txs = _remote_db->get_coldhot_transaction((coldhot_trx_state)type, transaction_id_type());
	   for (const auto& coldhot_tx : coldhot_txs) {
		  
		   results[coldhot_tx.current_id] = coldhot_tx.current_trx;
	   }
	   return results;
   }
   std::map<transaction_id_type, signed_transaction> get_coldhot_transaction_by_blocknum(const string& symbol,
	   const uint32_t& start_block_num,
	   const uint32_t& stop_block_num,
	   int crosschain_trx_state) {
	   std::map<transaction_id_type, signed_transaction> results;
	   std::vector<optional<coldhot_transfer_object>> coldhot_txs = _remote_db->get_coldhot_transaction_by_blocknum(symbol,start_block_num, stop_block_num, (coldhot_trx_state)crosschain_trx_state);
	   for (const auto& coldhot_tx : coldhot_txs) {
		   results[coldhot_tx->current_id] = coldhot_tx->current_trx;
	   }
	   return results;
   }
   std::map<transaction_id_type, signed_transaction> get_crosschain_transaction(int type) {
	   std::map<transaction_id_type, signed_transaction> result;
	   std::vector<crosschain_trx_object> cct_objs = _remote_db->get_crosschain_transaction((transaction_stata)type, transaction_id_type());
	   for (const auto& cct : cct_objs) {
		   auto id = cct.transaction_id;
		   result[id] = cct.real_transaction;
	   }
	   return result;
   }
   std::map<transaction_id_type, signed_transaction> get_crosschain_transaction_by_block_num(const string& symbol,
	   const string& account,
	   const uint32_t& start_block_num,
	   const uint32_t& stop_block_num,
	   int crosschain_trx_state) {
	   std::map<transaction_id_type, signed_transaction> result;
	   std::vector<optional<crosschain_trx_object>> cct_objs = _remote_db->get_crosschain_transaction_by_blocknum(symbol, account, start_block_num, stop_block_num,(transaction_stata)crosschain_trx_state);
	   for (const auto& cct : cct_objs) {
		   auto id = cct->transaction_id;
		   result[id] = cct->real_transaction;
	   }
	   return result;
   }
   std::map<transaction_id_type, signed_transaction> get_withdraw_crosschain_without_sign_transaction(){
	   std::map<transaction_id_type, signed_transaction> result;
	   std::vector<crosschain_trx_object> cct_objs = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_without_sign_trx_create,transaction_id_type());
	   for (const auto& cct : cct_objs){
		   auto id = cct.real_transaction.id();
		   result[id] = cct.real_transaction;
	   }
	   return result;
   }

   

   optional<multisig_address_object> get_current_multi_address_obj(const string& symbol, const account_id_type& guard) const
   {
	   auto vec_objs = get_multi_address_obj(symbol,guard);
	   optional<multisig_address_object> ret ;

	   int max = 0;
	   auto dynamic_props = get_dynamic_global_properties();
	   auto head_num = dynamic_props.head_block_number;
	   for (auto vec : vec_objs)
	   {
		   if (vec->multisig_account_pair_object_id == multisig_account_pair_id_type())
			   continue;
		   auto account_pair = get_multisig_account_pair(vec->multisig_account_pair_object_id);
		   if (account_pair->effective_block_num > head_num)
			   continue;
		   if (max < account_pair->effective_block_num)
		   {
			   max = account_pair->effective_block_num;
			   ret = vec;
		   }
	   }
	   return ret;
   }

   optional<multisig_account_pair_object> get_current_multi_address(const string& symbol) const
   {
	   auto guard_ids = get_global_properties().active_committee_members;
	   auto guards = _remote_db->get_guard_members(guard_ids);
	   account_id_type guard_acc;
	   for (auto guard : guards)
	   {
		   if (guard.valid())
		   {
			   if (guard->formal == true)
			   {
				   guard_acc = guard->guard_member_account;
				   break;
			   }
		   }
	   }
	   auto ret = get_current_multi_address_obj(symbol, guard_acc);
	   if (!ret.valid())
		   return optional<multisig_account_pair_object>();
	   return get_multisig_account_pair(ret->multisig_account_pair_object_id);
   }
   void senator_sign_eths_multi_account_create_trx(const string& tx_id, const string& senator,const string& keyfile,const string&decryptkey) {
		FC_ASSERT(!is_locked());
		auto guard_obj = get_guard_member(senator);
		const account_object & account_obj = get_account(senator);
		//get transaction and eth crosschain plugin
		FC_ASSERT(transaction_id_type(tx_id) != transaction_id_type(), "not correct transction.");
		auto trx_range = _remote_db->get_eths_multi_create_account_trx(eth_multi_account_trx_state::sol_create_need_guard_sign, transaction_id_type(tx_id));
		FC_ASSERT(trx_range.size() == 1);
		auto trx = trx_range[0];
		FC_ASSERT(trx.valid(), "Transaction find error");
		FC_ASSERT(trx->op_type == operation::tag<graphene::chain::eth_series_multi_sol_create_operation>::value, "Transaction find error");
		auto op = trx->object_transaction.operations[0];
		auto multi_account_op = op.get<graphene::chain::eth_series_multi_sol_create_operation>();
		FC_ASSERT(guard_obj.id == multi_account_op.guard_to_sign);
		auto & manager = graphene::crosschain::crosschain_manager::get_instance();
		auto crosschain_plugin = manager.get_crosschain_handle(multi_account_op.chain_type);
		string config = (*_crosschain_manager)->get_config();
		crosschain_plugin->initialize_config(fc::json::from_string(config).get_object());
		auto prk_cold_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(multi_account_op.chain_type);
		auto prk_hot_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(multi_account_op.chain_type);
		FC_ASSERT(_crosschain_keys.count(multi_account_op.guard_sign_hot_address) > 0, "private key doesnt belong to this wallet.");
		std::ifstream in(keyfile, std::ios::in | std::ios::binary);
		std::vector<char> key_file_data;
		if (in.is_open())
		{
			key_file_data= std::vector<char>((std::istreambuf_iterator<char>(in)),
				(std::istreambuf_iterator<char>()));
			in.close();
		}

		map<string, crosschain_prkeys> decrypted_keys;
		if (key_file_data.size() > 0)
		{
			const auto plain_text = fc::aes_decrypt(fc::sha512(decryptkey.c_str(), decryptkey.length()), key_file_data);
			decrypted_keys = fc::raw::unpack<map<string, crosschain_prkeys>>(plain_text);
		}
		FC_ASSERT(_crosschain_keys.count(multi_account_op.guard_sign_cold_address) > 0|| decrypted_keys.find(multi_account_op.chain_type+multi_account_op.guard_sign_cold_address)!= decrypted_keys.end(), "private key doesnt belong to this wallet or keyfile.");
		string wif_key_cold;
		if(_crosschain_keys.count(multi_account_op.guard_sign_cold_address) > 0)
			wif_key_cold=_crosschain_keys[multi_account_op.guard_sign_cold_address].wif_key;
		else
		{
			//std::cout << fc::json::to_pretty_string(decrypted_keys) << std::endl << multi_account_op.chain_type + multi_account_op.guard_sign_cold_address << std::endl;
			wif_key_cold = decrypted_keys[multi_account_op.chain_type+multi_account_op.guard_sign_cold_address].wif_key;
		}
		auto wif_key_hot = _crosschain_keys[multi_account_op.guard_sign_hot_address].wif_key;

		auto key_cold_ptr = prk_cold_ptr->import_private_key(wif_key_cold);
		FC_ASSERT(key_cold_ptr.valid());
		prk_cold_ptr->set_key(*key_cold_ptr);
		string siging_cold = crosschain_plugin->sign_multisig_transaction(fc::variant_object("without_sign_trx_sign",multi_account_op.multi_account_tx_without_sign_cold), prk_cold_ptr, "", false);

		auto key_hot_ptr = prk_hot_ptr->import_private_key(wif_key_hot);
		FC_ASSERT(key_hot_ptr.valid());
		prk_hot_ptr->set_key(*key_hot_ptr);
		string siging_hot = crosschain_plugin->sign_multisig_transaction(fc::variant_object("without_sign_trx_sign", multi_account_op.multi_account_tx_without_sign_hot), prk_hot_ptr, "", false);
		auto hot_hdtx = crosschain_plugin->turn_trx(fc::variant_object("get_with_sign", siging_hot));
		auto cold_hdtx = crosschain_plugin->turn_trx(fc::variant_object("get_with_sign", siging_cold));
		eths_multi_sol_guard_sign_operation tx_op;
		tx_op.guard_to_sign = multi_account_op.guard_to_sign;
		tx_op.guard_sign_address = account_obj.addr;
		tx_op.multi_hot_sol_guard_sign = siging_hot;
		tx_op.multi_cold_sol_guard_sign = siging_cold;
		tx_op.sol_without_sign_txid = transaction_id_type(tx_id);
		tx_op.chain_type = multi_account_op.chain_type;
		tx_op.multi_cold_trxid = cold_hdtx.trx_id;
		tx_op.multi_hot_trxid = hot_hdtx.trx_id;

		signed_transaction transaction;
		transaction.operations.push_back(tx_op);
		set_operation_fees(transaction, _remote_db->get_global_properties().parameters.current_fees);
		transaction.validate();
		sign_transaction(transaction, true);
   }

   void guard_sign_coldhot_transaction(const string& tx_id, const string& guard,const string& keyfile,const string& decryptkey) {
	   FC_ASSERT(!is_locked());
	   FC_ASSERT(transaction_id_type(tx_id) != transaction_id_type(),"not correct transction.");
	   auto trx = _remote_db->get_coldhot_transaction(coldhot_trx_state::coldhot_without_sign_trx_create, transaction_id_type(tx_id));
	   FC_ASSERT(trx.size() == 1, "Transaction find error");
	   FC_ASSERT(trx[0].op_type == operation::tag<graphene::chain::coldhot_transfer_without_sign_operation>::value, "Transaction find error");
	   auto op = trx[0].current_trx.operations[0];
	   auto coldhot_op = op.get<graphene::chain::coldhot_transfer_without_sign_operation>();
	   auto & manager = graphene::crosschain::crosschain_manager::get_instance();
	   auto crosschain_plugin = manager.get_crosschain_handle(coldhot_op.asset_symbol);
	   string config = (*_crosschain_manager)->get_config();
	   crosschain_plugin->initialize_config(fc::json::from_string(config).get_object());
	   string temp_guard(guard);
	   crosschain_trx handled_trx;
	   if ((coldhot_op.asset_symbol == "ETH") || (coldhot_op.asset_symbol.find("ERC") != coldhot_op.asset_symbol.npos)){
		   handled_trx = crosschain_plugin->turn_trxs(fc::variant_object("eth_trx",coldhot_op.coldhot_trx_original_chain));
	   }
	   else {
		   handled_trx = crosschain_plugin->turn_trxs(coldhot_op.coldhot_trx_original_chain);
	   }
	  
	   FC_ASSERT(handled_trx.trxs.size() == 1, "Transcation turn error in guard sign cold hot transaction");
	   auto multi_objs = _remote_db->get_multisig_account_pair(coldhot_op.asset_symbol);
	   string redeemScript = "";
	   string guard_address = "";
	   bool cold = false;
	   const account_object & account_obj = get_account(guard);
	   const auto& guard_obj = _remote_db->get_guard_member_by_account(account_obj.get_id());
	   auto guard_multi_address_objs = get_multi_address_obj(coldhot_op.asset_symbol, account_obj.id);
	   for (auto multi_obj : multi_objs) {
		   if (multi_obj->bind_account_hot == handled_trx.trxs.begin()->second.from_account) {
			   redeemScript = multi_obj->redeemScript_hot;
			   for (auto guard_multi_address_obj : guard_multi_address_objs) {
				   if (guard_multi_address_obj->multisig_account_pair_object_id == multi_obj->id) {
					   guard_address = guard_multi_address_obj->new_address_hot;
					   break;
				   }
			   }
			   break;
		   }
		   if (multi_obj->bind_account_cold == handled_trx.trxs.begin()->second.from_account) {
			   redeemScript = multi_obj->redeemScript_cold;
			   for (auto guard_multi_address_obj : guard_multi_address_objs) {
				   if (guard_multi_address_obj->multisig_account_pair_object_id == multi_obj->id) {
					   guard_address = guard_multi_address_obj->new_address_cold;
					   cold = true;
					   break;
				   }
			   }
			   break;
		   }
	   }
	   FC_ASSERT((redeemScript != "") && (guard_address != ""), "redeemScript exist error");
	   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(coldhot_op.asset_symbol);
	  
	   if (!cold)
	   {
		   FC_ASSERT(_crosschain_keys.count(guard_address) > 0, "private key doesnt belong to this wallet.");
		   auto wif_key = _crosschain_keys[guard_address].wif_key;

		   auto key_ptr = prk_ptr->import_private_key(wif_key);

		   prk_ptr->set_key(*key_ptr);
	   }
	   else
	   {
		   std::ifstream in(keyfile, std::ios::in | std::ios::binary);
		   std::vector<char> key_file_data;
		   if (in.is_open())
		   {
			   key_file_data = std::vector<char>((std::istreambuf_iterator<char>(in)),
				   (std::istreambuf_iterator<char>()));
			   in.close();
		   }
		   map<string, crosschain_prkeys> keys;
		   if (key_file_data.size() > 0)
		   {
			   const auto plain_text = fc::aes_decrypt(fc::sha512(decryptkey.c_str(), decryptkey.length()), key_file_data);
			   keys = fc::raw::unpack<map<string, crosschain_prkeys>>(plain_text);
		   }
		   FC_ASSERT(keys.find(coldhot_op.asset_symbol + guard_address) != keys.end(),"cold key can't be found in keyfile");
		   //std::cout << fc::json::to_pretty_string(keys) << std::endl << coldhot_op.asset_symbol + guard_address << std::endl;
		   auto key_ptr = prk_ptr->import_private_key(keys[coldhot_op.asset_symbol+guard_address].wif_key);
		   prk_ptr->set_key(*key_ptr);
	   }
	   string siging;
	   if (coldhot_op.asset_symbol == "ETH" || coldhot_op.asset_symbol.find("ERC") != coldhot_op.asset_symbol.npos) {
		   siging = crosschain_plugin->sign_multisig_transaction(fc::variant_object("get_param_hash", coldhot_op.coldhot_trx_original_chain), prk_ptr, redeemScript, false);
	   }
	   else {
		   siging = crosschain_plugin->sign_multisig_transaction(coldhot_op.coldhot_trx_original_chain, prk_ptr, redeemScript, false);
	   }
	   coldhot_transfer_with_sign_operation tx_op;

	   tx_op.coldhot_trx_id = trx[0].current_id;
	   tx_op.coldhot_trx_original_chain = coldhot_op.coldhot_trx_original_chain;
	   tx_op.sign_guard = guard_obj->id;
	   tx_op.asset_symbol = coldhot_op.asset_symbol;
	   tx_op.guard_address = account_obj.addr;
	   tx_op.coldhot_transfer_sign = siging;
	   signed_transaction transaction;
	   transaction.operations.push_back(tx_op);
	   set_operation_fees(transaction, _remote_db->get_global_properties().parameters.current_fees);
	   transaction.validate();
	   sign_transaction(transaction, true);
	   
   }
   void senator_sign_eths_coldhot_final_trx(const string& trx_id, const string & senator, const string& keyfile, const string&decryptkey) {
	   FC_ASSERT(!is_locked());
	   FC_ASSERT(transaction_id_type(trx_id) != transaction_id_type(), "not correct transction.");
	   auto trx = _remote_db->get_coldhot_transaction(coldhot_trx_state::coldhot_eth_guard_need_sign, transaction_id_type(trx_id));
	   FC_ASSERT(trx.size() == 1, "Transaction find error");
	   FC_ASSERT(trx[0].op_type == operation::tag<graphene::chain::coldhot_transfer_combine_sign_operation>::value, "Transaction find error");
	   auto op = trx[0].current_trx.operations[0];
	   auto coldhot_op = op.get<graphene::chain::coldhot_transfer_combine_sign_operation>();
	   auto & manager = graphene::crosschain::crosschain_manager::get_instance();
	   auto crosschain_plugin = manager.get_crosschain_handle(coldhot_op.asset_symbol);
	   string config = (*_crosschain_manager)->get_config();
	   crosschain_plugin->initialize_config(fc::json::from_string(config).get_object());
	   auto eth_without_sign_trx_obj = coldhot_op.coldhot_trx_original_chain;
	   auto sign_senator = eth_without_sign_trx_obj["signer"].as_string();
	   auto without_sign = eth_without_sign_trx_obj["without_sign"].as_string();

	   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(coldhot_op.asset_symbol);
	   


	   string wif_key;
	   std::ifstream in(keyfile, std::ios::in | std::ios::binary);
	   std::vector<char> key_file_data;
	   if (in.is_open())
	   {
		   key_file_data = std::vector<char>((std::istreambuf_iterator<char>(in)),
			   (std::istreambuf_iterator<char>()));
		   in.close();
	   }
	   map<string, crosschain_prkeys> decrypted_keys;
	   if (key_file_data.size() > 0)
	   {
		   const auto plain_text = fc::aes_decrypt(fc::sha512(decryptkey.c_str(), decryptkey.length()), key_file_data);
		   decrypted_keys = fc::raw::unpack<map<string, crosschain_prkeys>>(plain_text);
	   }
	   FC_ASSERT(_crosschain_keys.count(sign_senator) > 0|| decrypted_keys.find(coldhot_op.asset_symbol + sign_senator) != decrypted_keys.end(), "private key doesnt belong to this wallet or keyfile.");
	   if (_crosschain_keys.count(sign_senator) > 0)
		   wif_key = _crosschain_keys[sign_senator].wif_key;
	   else
	   {
		   //std::cout << fc::json::to_pretty_string(decrypted_keys) << std::endl << coldhot_op.asset_symbol + sign_senator << std::endl;
		   wif_key = decrypted_keys[coldhot_op.asset_symbol + sign_senator].wif_key;
	   }

	   auto key_ptr = prk_ptr->import_private_key(wif_key);
	   FC_ASSERT(key_ptr.valid());

	   string siging = crosschain_plugin->sign_multisig_transaction(fc::variant_object("without_sign_trx_sign", without_sign), prk_ptr, "", false);
	   std::cout << siging << std::endl;
	   auto hdtx = crosschain_plugin->turn_trx(fc::variant_object("get_sign_final_trx_id", siging));
	   eths_coldhot_guard_sign_final_operation trx_op;
	   trx_op.signed_crosschain_trx_id = hdtx.trx_id;
	   account_object account_obj = get_account(senator);
	   const auto& guard_obj = _remote_db->get_guard_member_by_account(account_obj.get_id());
	   trx_op.guard_to_sign = guard_obj->id;
	   trx_op.chain_type = coldhot_op.asset_symbol;
	   trx_op.combine_trx_id = transaction_id_type(trx_id);
	   trx_op.signed_crosschain_trx = siging;
	   trx_op.guard_address = account_obj.addr;
	   signed_transaction transaction;
	   transaction.operations.push_back(trx_op);
	   set_operation_fees(transaction, _remote_db->get_global_properties().parameters.current_fees);
	   transaction.validate();
	   sign_transaction(transaction, true);

	  
   }
   account_id_type get_eth_signer(const string & symbol, const string & address) {
	   FC_ASSERT(!is_locked());
	   account_id_type guardId;
	   auto multi_accounts =  get_multisig_account_pair(symbol);
	   for (auto multiAccount : multi_accounts) {
		   auto senators = get_multi_account_guard(multiAccount->bind_account_hot,symbol);
		   bool bFinder = false;
		   for (auto multiSenator : senators)
		   {
			   if (!multiSenator.valid())
				   continue;
			   if (multiSenator->new_address_hot == address || multiSenator->new_address_cold == address)
			   {
				   guardId = multiSenator->guard_account;
				   bFinder = true;
				   break;
			   }
			   if (multiSenator->new_pubkey_hot == address || multiSenator->new_pubkey_cold == address)
			   {
				   guardId = multiSenator->guard_account;
				   bFinder = true;
				   break;
			   }
		   }
		   if (bFinder){
			   break;
		   }
	   }
	   return guardId;
   }
   void senator_changer_eth_coldhot_singer_trx(const string guard, const string txid, const string& newaddress, const int64_t& expiration_time, const string& keyfile, const string& decryptkey, bool broadcast) {

	   auto trx = _remote_db->get_coldhot_transaction(coldhot_trx_state::coldhot_eth_guard_need_sign, transaction_id_type(txid));
	   FC_ASSERT(trx.size() == 1, "Transaction error");
	   auto& op = trx[0].current_trx.operations[0];
	   auto withop_without_sign = op.get<coldhot_transfer_combine_sign_operation>();
	   auto& manager = graphene::crosschain::crosschain_manager::get_instance();
	   auto hdl = manager.get_crosschain_handle(std::string(withop_without_sign.asset_symbol));
	   string config = (*_crosschain_manager)->get_config();
	   hdl->initialize_config(fc::json::from_string(config).get_object());
	   auto eth_without_sign_trx_obj = withop_without_sign.coldhot_trx_original_chain;
	   auto sign_senator = newaddress;
	   auto without_sign = eth_without_sign_trx_obj["without_sign"].as_string();
	   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(withop_without_sign.asset_symbol);
	   std::ifstream in(keyfile, std::ios::in | std::ios::binary);
	   std::vector<char> key_file_data;
	   if (in.is_open())
	   {
		   key_file_data = std::vector<char>((std::istreambuf_iterator<char>(in)),
			   (std::istreambuf_iterator<char>()));
		   in.close();
	   }

	   map<string, crosschain_prkeys> decrypted_keys;
	   if (key_file_data.size() > 0)
	   {
		   const auto plain_text = fc::aes_decrypt(fc::sha512(decryptkey.c_str(), decryptkey.length()), key_file_data);
		   decrypted_keys = fc::raw::unpack<map<string, crosschain_prkeys>>(plain_text);
	   }

	   FC_ASSERT(_crosschain_keys.count(sign_senator) > 0|| decrypted_keys.find(withop_without_sign.asset_symbol+ sign_senator)!= decrypted_keys.end(), "private key doesnt belong to this wallet.");
	   string wif_key;
	   if(_crosschain_keys.count(sign_senator) > 0)
		   wif_key = _crosschain_keys[sign_senator].wif_key;
	   else
	   {
		   wif_key = decrypted_keys[withop_without_sign.asset_symbol + sign_senator].wif_key;
	   }
	   auto key_ptr = prk_ptr->import_private_key(wif_key);
	   FC_ASSERT(key_ptr.valid());
	   string siging = hdl->sign_multisig_transaction(fc::variant_object("change_signer_sign", without_sign), prk_ptr, "", false);
	   std::cout << siging << std::endl;
	   auto hdtx = hdl->turn_trx(fc::variant_object("get_sign_final_trx_id", siging));
	   proposal_create_operation prop_op;
	   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
	   prop_op.proposer = get_account(guard).get_id();
	   prop_op.fee_paying_account = get_account(guard).addr;
	   prop_op.type = vote_id_type::vote_type::cancel_commit;
	   eths_guard_coldhot_change_signer_operation trx_op;
	   trx_op.signed_crosschain_trx_id = hdtx.trx_id;
	   account_object account_obj = get_account(guard);
	   const auto& guard_obj = _remote_db->get_guard_member_by_account(account_obj.get_id());
	   trx_op.guard_to_sign = guard_obj->id;
	   trx_op.new_signer = newaddress;
	   trx_op.chain_type = withop_without_sign.asset_symbol;
	   trx_op.combine_trx_id = transaction_id_type(txid);
	   trx_op.signed_crosschain_trx = siging;
	   trx_op.guard_address = account_obj.addr;
	   prop_op.proposed_ops.emplace_back(trx_op);
	   const chain_parameters& current_params = get_global_properties().parameters;
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	   signed_transaction transaction;
	   transaction.operations.push_back(prop_op);
	   set_operation_fees(transaction, current_params.current_fees);
	   transaction.validate();
	   sign_transaction(transaction, true);
   }
   void senator_changer_eth_singer_trx(const string guard, const string txid, const string& newaddress, const int64_t& expiration_time, bool broadcast) {
	   
	   auto trx = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_eth_guard_need_sign, transaction_id_type(txid));
	   FC_ASSERT(trx.size() == 1, "Transaction error");
	   auto& op = trx[0].real_transaction.operations[0];
	   auto withop_without_sign = op.get<crosschain_withdraw_combine_sign_operation>();
	   auto& manager = graphene::crosschain::crosschain_manager::get_instance();
	   auto hdl = manager.get_crosschain_handle(std::string(withop_without_sign.asset_symbol));
	   string config = (*_crosschain_manager)->get_config();
	   hdl->initialize_config(fc::json::from_string(config).get_object());
	   auto eth_without_sign_trx_obj = withop_without_sign.cross_chain_trx;
	   auto sign_senator = newaddress;
	   auto without_sign = eth_without_sign_trx_obj["without_sign"].as_string();
	   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(withop_without_sign.asset_symbol);
	   FC_ASSERT(_crosschain_keys.count(sign_senator) > 0, "private key doesnt belong to this wallet.");
	   auto wif_key = _crosschain_keys[sign_senator].wif_key;
	   auto key_ptr = prk_ptr->import_private_key(wif_key);
	   FC_ASSERT(key_ptr.valid());
	   string siging = hdl->sign_multisig_transaction(fc::variant_object("change_signer_sign", without_sign), prk_ptr, "", false);
	   std::cout << siging << std::endl;
	   auto hdtx = hdl->turn_trx(fc::variant_object("get_sign_final_trx_id", siging));
	   proposal_create_operation prop_op;
	   prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
	   prop_op.proposer = get_account(guard).get_id();
	   prop_op.fee_paying_account = get_account(guard).addr;
	   prop_op.type = vote_id_type::vote_type::cancel_commit;
	   eths_guard_change_signer_operation trx_op;
	   trx_op.signed_crosschain_trx_id = hdtx.trx_id;
	   account_object account_obj = get_account(guard);
	   const auto& guard_obj = _remote_db->get_guard_member_by_account(account_obj.get_id());
	   trx_op.guard_to_sign = guard_obj->id;
	   trx_op.new_signer = newaddress;
	   trx_op.chain_type = withop_without_sign.asset_symbol;
	   trx_op.combine_trx_id = transaction_id_type(txid);
	   trx_op.signed_crosschain_trx = siging;
	   trx_op.guard_address = account_obj.addr;
	   prop_op.proposed_ops.emplace_back(trx_op);
	   const chain_parameters& current_params = get_global_properties().parameters;
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	   signed_transaction transaction;
	   transaction.operations.push_back(prop_op);
	   set_operation_fees(transaction, current_params.current_fees);
	   transaction.validate();
	   sign_transaction(transaction, true);
   }
   void senator_sign_eths_final_trx(const string& trx_id, const string & senator) {
	   FC_ASSERT(!is_locked());
	   auto guard_obj = get_guard_member(senator);
	   auto guard_id = guard_obj.guard_member_account;
	   /*
	   if (trx_id == "ALL") {
		   auto trxs = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_eth_guard_need_sign, transaction_id_type());

			   auto operations = trx.real_transaction.operations;
			   auto op = trx.real_transaction.operations[0];
			   if (op.which() != operation::tag<graphene::chain::crosschain_withdraw_combine_sign_operation>::value)
			   {
				   continue;
			   }
			   auto withop_without_sign = op.get<graphene::chain::crosschain_withdraw_combine_sign_operation>();
			   auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			   auto hdl = manager.get_crosschain_handle(std::string(withop_without_sign.asset_symbol));
			   string config = (*_crosschain_manager)->get_config();
			   hdl->initialize_config(fc::json::from_string(config).get_object());
			   string temp_guard(senator);
			   auto current_multi_obj = get_current_multi_address_obj(withop_without_sign.asset_symbol, guard_id);
			   FC_ASSERT(current_multi_obj.valid());
			   auto account_pair_obj = get_multisig_account_pair(current_multi_obj->multisig_account_pair_object_id);

			   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(withop_without_sign.asset_symbol);
			   FC_ASSERT(_crosschain_keys.count(current_multi_obj->new_address_hot)>0, "private key doesnt belong to this wallet.");
			   auto wif_key = _crosschain_keys[current_multi_obj->new_address_hot].wif_key;
			   auto key_ptr = prk_ptr->import_private_key(wif_key);
			   FC_ASSERT(key_ptr.valid());
			   string siging = hdl->sign_multisig_transaction(withop_without_sign.cross_chain_trx, prk_ptr, account_pair_obj->redeemScript_hot, false);
			   eths_guard_sign_final_operation trx_op;
			   account_object account_obj = get_account(senator);
			   const auto& guard_obj = _remote_db->get_guard_member_by_account(account_obj.get_id());
			   trx_op.guard_to_sign = guard_obj->id;
			   trx_op.combine_trx_id = trx.transaction_id;
			   trx_op.signed_crosschain_trx = siging;
			   trx_op.guard_address = account_obj.addr;
			   signed_transaction transaction;
			   transaction.operations.push_back(trx_op);
			   set_operation_fees(transaction, _remote_db->get_global_properties().parameters.current_fees);
			   transaction.validate();
			   sign_transaction(transaction, true);
		   }
	   }
	   else {
	   */
		   auto trx = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_eth_guard_need_sign, transaction_id_type(trx_id));
		   FC_ASSERT(trx.size() == 1, "Transaction error");
		   auto& op = trx[0].real_transaction.operations[0];
		   auto withop_without_sign = op.get<crosschain_withdraw_combine_sign_operation>();
		   auto& manager = graphene::crosschain::crosschain_manager::get_instance();
		   auto hdl = manager.get_crosschain_handle(std::string(withop_without_sign.asset_symbol));
		   string config = (*_crosschain_manager)->get_config();
		   hdl->initialize_config(fc::json::from_string(config).get_object());
		   auto eth_without_sign_trx_obj = withop_without_sign.cross_chain_trx;
		   auto sign_senator = eth_without_sign_trx_obj["signer"].as_string();
		   auto without_sign = eth_without_sign_trx_obj["without_sign"].as_string();
		   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(withop_without_sign.asset_symbol);
		   FC_ASSERT(_crosschain_keys.count(sign_senator) > 0, "private key doesnt belong to this wallet.");
		   auto wif_key = _crosschain_keys[sign_senator].wif_key;
		   auto key_ptr = prk_ptr->import_private_key(wif_key);
		   FC_ASSERT(key_ptr.valid());
		   string siging = hdl->sign_multisig_transaction(fc::variant_object("without_sign_trx_sign", without_sign), prk_ptr, "", false);
		   std::cout << siging << std::endl;
		   auto hdtx = hdl->turn_trx(fc::variant_object("get_sign_final_trx_id", siging));
		   eths_guard_sign_final_operation trx_op;
		   trx_op.signed_crosschain_trx_id = hdtx.trx_id;
		   account_object account_obj = get_account(senator);
		   const auto& guard_obj_x = _remote_db->get_guard_member_by_account(account_obj.get_id());
		   trx_op.guard_to_sign = guard_obj_x->id;
		   trx_op.chain_type = withop_without_sign.asset_symbol;
		   trx_op.combine_trx_id = transaction_id_type(trx_id);
		   trx_op.signed_crosschain_trx = siging;
		   trx_op.guard_address = account_obj.addr;
		   signed_transaction transaction;
		   transaction.operations.push_back(trx_op);
		   set_operation_fees(transaction, _remote_db->get_global_properties().parameters.current_fees);
		   transaction.validate();
		   sign_transaction(transaction, true);
	  // }

   }
   void guard_sign_crosschain_transaction(const string& trx_id,const string & guard){
	   FC_ASSERT(!is_locked());
	   auto guard_obj = get_guard_member(guard);
	   auto guard_id = guard_obj.guard_member_account;
	   if (trx_id == "ALL"){
		   auto trxs = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_without_sign_trx_create, transaction_id_type());
		   
		   for (const auto& trx : trxs) {
			   /*auto id = trx.transaction_id.str();
			   std::cout << id << std::endl;
			   auto op = trx.real_transaction.operations[0];
			   std::cout << op.which() << std::endl;*/
			   
			   auto operations = trx.real_transaction.operations;
			   auto op = trx.real_transaction.operations[0];
			   if (op.which() != operation::tag<graphene::chain::crosschain_withdraw_without_sign_operation>::value)
			   {
				   continue;
			   }
			   auto withop_without_sign = op.get<graphene::chain::crosschain_withdraw_without_sign_operation>();
			   auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			   auto hdl = manager.get_crosschain_handle(std::string(withop_without_sign.asset_symbol));
			   string config = (*_crosschain_manager)->get_config();
			   hdl->initialize_config(fc::json::from_string(config).get_object());
			   string temp_guard(guard);
			   auto current_multi_obj = get_current_multi_address_obj(withop_without_sign.asset_symbol, guard_id);
			   FC_ASSERT(current_multi_obj.valid());
			   auto account_pair_obj = get_multisig_account_pair(current_multi_obj->multisig_account_pair_object_id);

			   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(withop_without_sign.asset_symbol);
			   FC_ASSERT(_crosschain_keys.count(current_multi_obj->new_address_hot)>0, "private key doesnt belong to this wallet.");
			   auto wif_key = _crosschain_keys[current_multi_obj->new_address_hot].wif_key;
			   auto key_ptr = prk_ptr->import_private_key(wif_key);
			   FC_ASSERT(key_ptr.valid());
			   string siging;
			   if (withop_without_sign.asset_symbol == "ETH" || withop_without_sign.asset_symbol.find("ERC") != withop_without_sign.asset_symbol.npos) {
				   siging = hdl->sign_multisig_transaction(fc::variant_object("get_param_hash", withop_without_sign.withdraw_source_trx), prk_ptr, account_pair_obj->redeemScript_hot, false);
			   }
			   else {
				   siging = hdl->sign_multisig_transaction(withop_without_sign.withdraw_source_trx, prk_ptr, account_pair_obj->redeemScript_hot, false);
			   }
			  // string siging = hdl->sign_multisig_transaction(withop_without_sign.withdraw_source_trx, prk_ptr, account_pair_obj->redeemScript_hot, false);
			   crosschain_withdraw_with_sign_operation trx_op;
			   const account_object & account_obj = get_account(guard);
			   const auto& guard_obj = _remote_db->get_guard_member_by_account(account_obj.get_id());
			   trx_op.ccw_trx_id = trx.transaction_id;
			   trx_op.ccw_trx_signature = siging;
			   trx_op.withdraw_source_trx = withop_without_sign.withdraw_source_trx;
			   trx_op.asset_symbol = withop_without_sign.asset_symbol;
			   trx_op.sign_guard = guard_obj->id;
			   trx_op.guard_address = account_obj.addr;
			   signed_transaction transaction;
			   transaction.operations.push_back(trx_op);
			   set_operation_fees(transaction, _remote_db->get_global_properties().parameters.current_fees);
			   transaction.validate();
			   sign_transaction(transaction, true);
		   }
	   }
	   else{
		   auto trx = _remote_db->get_crosschain_transaction(transaction_stata::withdraw_without_sign_trx_create, transaction_id_type(trx_id));
		   FC_ASSERT(trx.size() == 1, "Transaction error");
		   auto& op = trx[0].real_transaction.operations[0];
		   auto withop_without_sign = op.get<crosschain_withdraw_without_sign_operation>();
		   auto& manager = graphene::crosschain::crosschain_manager::get_instance();
		   auto hdl = manager.get_crosschain_handle(std::string(withop_without_sign.asset_symbol));
		   string config = (*_crosschain_manager)->get_config();
		   hdl->initialize_config(fc::json::from_string(config).get_object());

		   auto current_multi_obj = get_current_multi_address_obj(withop_without_sign.asset_symbol, guard_id);
		   FC_ASSERT(current_multi_obj.valid());
		   auto account_pair_obj = get_multisig_account_pair(current_multi_obj->multisig_account_pair_object_id);
		   auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(withop_without_sign.asset_symbol);
		   FC_ASSERT(_crosschain_keys.count(current_multi_obj->new_address_hot)>0, "private key doesnt belong to this wallet.");
		   auto wif_key = _crosschain_keys[current_multi_obj->new_address_hot].wif_key;
		   auto key_ptr = prk_ptr->import_private_key(wif_key);
		   FC_ASSERT(key_ptr.valid());
		   string siging;
		   if (withop_without_sign.asset_symbol == "ETH" || withop_without_sign.asset_symbol.find("ERC") != withop_without_sign.asset_symbol.npos) {
				siging = hdl->sign_multisig_transaction(fc::variant_object("get_param_hash",withop_without_sign.withdraw_source_trx), prk_ptr, account_pair_obj->redeemScript_hot, false);
		   }
		   else {
			   siging = hdl->sign_multisig_transaction( withop_without_sign.withdraw_source_trx, prk_ptr, account_pair_obj->redeemScript_hot, false);
		   }
		   
		   std::cout << siging << std::endl;
		   crosschain_withdraw_with_sign_operation trx_op;
	
		   const account_object & account_obj = get_account(guard);
		   const auto& guard_obj = _remote_db->get_guard_member_by_account(account_obj.get_id());
		   trx_op.ccw_trx_id = trx[0].transaction_id;
		   trx_op.ccw_trx_signature = siging;
		   trx_op.withdraw_source_trx = withop_without_sign.withdraw_source_trx;
		   trx_op.asset_symbol = withop_without_sign.asset_symbol;
		   trx_op.sign_guard = guard_obj->id;
		   trx_op.guard_address = account_obj.addr;
		   signed_transaction transaction;
		   transaction.operations.push_back(trx_op);
		   set_operation_fees(transaction, _remote_db->get_global_properties().parameters.current_fees);
		   transaction.validate();
		   sign_transaction(transaction, true);
		   
	   }
	   
   }
   std::vector<lockbalance_object> get_account_lock_balance(const string& account)const {
	   FC_ASSERT(account.size() != 0, "Param without account name");
	   const auto& account_obj = get_account(account);
	   return _remote_db->get_account_lock_balance(account_obj.get_id());
   }
   std::vector<guard_lock_balance_object> get_guard_lock_balance(const string& miner) const{
	   FC_ASSERT(miner.size() != 0, "Param without miner account ");
	   const account_object & account_obj = get_account(miner);
	   const auto& guard_obj = _remote_db->get_guard_member_by_account(account_obj.get_id());
	   FC_ASSERT(guard_obj.valid(), "Guard is not exist");
	   return _remote_db->get_guard_lock_balance(guard_obj->id);
   }
   std::vector<acquired_crosschain_trx_object> get_acquire_transaction(const int & type, const string & trxid) {
	  return _remote_db->get_acquire_transaction(type,trxid);
   }
   std::vector<lockbalance_object> get_miner_lock_balance(const string& miner)const
   {
	   FC_ASSERT(miner.size() != 0, "Param without miner account ");
	   const account_object & account_obj = get_account(miner);
	   const auto& miner_obj = _remote_db->get_miner_by_account(account_obj.get_id());
	   FC_ASSERT(miner_obj.valid(), "Miner is not exist");
	   return _remote_db->get_miner_lock_balance(miner_obj->id);
   }
   full_transaction cancel_order(object_id_type order_id, bool broadcast = false)
   { try {
         FC_ASSERT(!is_locked());
         FC_ASSERT(order_id.space() == protocol_ids, "Invalid order ID ${id}", ("id", order_id));
         signed_transaction trx;

         limit_order_cancel_operation op;
         op.fee_paying_account = get_object<limit_order_object>(order_id).seller;
         op.order = order_id;
         trx.operations = {op};
         set_operation_fees( trx, _remote_db->get_global_properties().parameters.current_fees);

         trx.validate();
         return sign_transaction(trx, broadcast);
   } FC_CAPTURE_AND_RETHROW((order_id)) }
   full_transaction withdraw_cross_chain_transaction(string account_name,
	   string amount,
	   string asset_symbol,
	   string crosschain_account,
	   string memo,
	   bool broadcast = false) {
	   try {
		   FC_ASSERT(!self.is_locked());
		   fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
		   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
		   FC_ASSERT(asset_obj->allow_withdraw_deposit,"${asset} does not allow withdraw and deposit",("asset", asset_symbol));
		   auto& iter = _wallet.my_accounts.get<by_name>();
		   FC_ASSERT(iter.find(account_name) != iter.end(), "Could not find account name ${account}", ("account", account_name));
		   crosschain_withdraw_operation op;
		   op.withdraw_account = iter.find(account_name)->addr;
		   op.amount = amount;
		   op.asset_id = asset_obj->id;
		   op.asset_symbol = asset_symbol;
		   op.crosschain_account = crosschain_account;
		   signed_transaction tx;

		   tx.operations.push_back(op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();

		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((account_name)(amount)(asset_symbol)(crosschain_account)(memo))
   }
   full_transaction transfer_guard_multi_account(string multi_account,
	   string amount,
	   string asset_symbol,
	   string multi_to_account,
	   string memo,
	   bool broadcast/* = false*/) {
	   try {
		   fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
		   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
		   auto multiAccountObjs = _remote_db->get_multisig_account_pair(asset_symbol);
		   bool withdraw_multi = false;
		   bool deposit_multi = false;
		   for (const auto& multiAccountObj : multiAccountObjs){
			   if (multi_account == multiAccountObj->bind_account_hot || multi_account == multiAccountObj->bind_account_cold) {
				   withdraw_multi = true;
			   }
			   if (multi_to_account == multiAccountObj->bind_account_hot || multi_to_account == multiAccountObj->bind_account_cold) {
				   deposit_multi = true;
			   }
			   if (withdraw_multi && deposit_multi) {
				   break;
			   }
		   }
		   FC_ASSERT((withdraw_multi &&deposit_multi), "Cant Transfer account which is not multi account");
		   crosschain_withdraw_operation op;
		   //op.withdraw_account = multi_account;
		   op.amount = amount;
		   op.asset_id = asset_obj->id;
		   op.asset_symbol = asset_symbol;
		   op.crosschain_account = multi_to_account;
		   signed_transaction tx;
		   tx.operations.push_back(op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((multi_account)(amount)(asset_symbol)(multi_to_account)(memo))
   }

   full_transaction lock_balance_to_miners(string lock_account, map<string, vector<asset>> lockbalances, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   auto& acct_iter = _wallet.my_accounts.get<by_name>();
		   auto lock_acct = acct_iter.find(lock_account);
		   FC_ASSERT(lock_acct != acct_iter.end(), "Could not find account name ${account}", ("account", lock_account));
		   signed_transaction tx;
		   for (const auto& iter : lockbalances)
		   {
			   const auto& miner_account = iter.first;
			   fc::optional<miner_object> miner_obj = get_miner(miner_account);
			   FC_ASSERT(miner_obj, "Could not find miner matching ${miner}", ("miner", miner_account));
			   const auto& assets = iter.second;
			   for (const auto& asset_vec : assets)
			   {
				   fc::optional<asset_object> asset_obj = get_asset(asset_vec.asset_id);
				   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_vec));
				   lockbalance_operation lb_op;
				   lb_op.lock_asset_id = asset_vec.asset_id;
				   lb_op.lock_asset_amount = asset_vec.amount;
				   lb_op.lock_balance_account = lock_acct->get_id();
				   lb_op.lock_balance_addr = lock_acct->addr;
				   lb_op.lockto_miner_account = miner_obj->id;
				   tx.operations.push_back(lb_op);
			   }
			   
		   }
		   FC_ASSERT(tx.operations.size() <= 100, "should less than 100 operations.");
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((lock_account)(lockbalances)(broadcast))
   }

   full_transaction lock_balance_to_miner(string miner_account,
	   string lock_account,
	   string amount,
	   string asset_symbol,
	   bool broadcast = false) {
	   try {
		   FC_ASSERT(!self.is_locked());
		   fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
		   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
		   auto& iter = _wallet.my_accounts.get<by_name>();
		   FC_ASSERT(iter.find(lock_account) != iter.end(), "Could not find account name ${account}", ("account", lock_account));
		   fc::optional<miner_object> miner_obj = get_miner(miner_account);
		   FC_ASSERT(miner_obj, "Could not find miner matching ${miner}", ("miner", miner_account));
		   lockbalance_operation lb_op;
		   lb_op.lock_asset_id = asset_obj->get_id();
		   lb_op.lock_asset_amount = asset_obj->amount_from_string(amount).amount;
		   lb_op.lock_balance_account = iter.find(lock_account)->get_id();
		   lb_op.lock_balance_addr = iter.find(lock_account)->addr;
		   lb_op.lockto_miner_account = miner_obj->id;
		   signed_transaction tx;

		   tx.operations.push_back(lb_op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();

		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((miner_account)(lock_account)(amount)(asset_symbol)(broadcast))
   }
   full_transaction guard_lock_balance(string guard_account,
	   string amount,
	   string asset_symbol,
	   bool broadcast = false) {
	   try {
		   FC_ASSERT(!self.is_locked());
		   fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
		   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
		   auto& iter = _wallet.my_accounts.get<by_name>();
		   FC_ASSERT(iter.find(guard_account) != iter.end(), "Could not find account name ${account}", ("account", guard_account));
		   fc::optional<guard_member_object> guard_obj = get_guard_member(guard_account);
		   FC_ASSERT(guard_obj, "Could not find miner matching ${guard}", ("guard", guard_account));
		   guard_lock_balance_operation guard_lb_op;
		   //lockbalance_operation lb_op;
		   guard_lb_op.lock_asset_id = asset_obj->get_id();
		   guard_lb_op.lock_asset_amount = asset_obj->amount_from_string(amount).amount;
		   guard_lb_op.lock_balance_account = guard_obj->id;
		   guard_lb_op.lock_balance_account_id = guard_obj->guard_member_account;
		   guard_lb_op.lock_address = iter.find(guard_account)->addr;
		   signed_transaction tx;

		   tx.operations.push_back(guard_lb_op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();

		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((guard_account)(amount)(asset_symbol)(broadcast))
   }
   full_transaction foreclose_balance_from_miners(string foreclose_account, map<string, vector<asset>> foreclose_balances, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   auto& acct_iter = _wallet.my_accounts.get<by_name>();
		   auto lock_acct = acct_iter.find(foreclose_account);
		   FC_ASSERT(lock_acct != acct_iter.end(), "Could not find account name ${account}", ("account", foreclose_account));
		   signed_transaction tx;
		   for (const auto& iter : foreclose_balances)
		   {
			   const auto& miner_account = iter.first;
			   fc::optional<miner_object> miner_obj = get_miner(miner_account);
			   FC_ASSERT(miner_obj, "Could not find miner matching ${miner}", ("miner", miner_account));
			   const auto& assets = iter.second;
			   for (const auto& asset_vec : assets)
			   {
				   fc::optional<asset_object> asset_obj = get_asset(asset_vec.asset_id);
				   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_vec));
				   foreclose_balance_operation fcb_op;
				   fcb_op.foreclose_asset_id = asset_vec.asset_id;
				   fcb_op.foreclose_asset_amount = asset_vec.amount;
				   fcb_op.foreclose_account = lock_acct->get_id();
				   fcb_op.foreclose_addr = lock_acct->addr;
				   fcb_op.foreclose_miner_account = miner_obj->id;
				   tx.operations.push_back(fcb_op);
			   }

		   }
		   FC_ASSERT(tx.operations.size() <= 100, "should less than 100 operations.");
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();
		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((foreclose_account)(foreclose_balances)(broadcast))
   }


   full_transaction foreclose_balance_from_miner(string miner_account,
	   string foreclose_account,
	   string amount,
	   string asset_symbol,
	   bool broadcast = false) {
	   try {
		   FC_ASSERT(!self.is_locked());
		   fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
		   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
		   auto& iter = _wallet.my_accounts.get<by_name>();
		   FC_ASSERT(iter.find(foreclose_account) != iter.end(), "Could not find account name ${account}", ("account", foreclose_account));
		   fc::optional<miner_object> miner_obj = get_miner(miner_account);
		   FC_ASSERT(miner_obj, "Could not find miner matching ${miner}", ("miner", miner_account));
		   foreclose_balance_operation fcb_op;
		   fcb_op.foreclose_asset_id = asset_obj->get_id();
		   fcb_op.foreclose_asset_amount = asset_obj->amount_from_string(amount).amount;
		   fcb_op.foreclose_account = iter.find(foreclose_account)->get_id();
		   fcb_op.foreclose_addr = iter.find(foreclose_account)->addr;
		   fcb_op.foreclose_miner_account = miner_obj->id;
		   signed_transaction tx;

		   tx.operations.push_back(fcb_op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();

		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((miner_account)(foreclose_account)(amount)(asset_symbol)(broadcast))
   }
   full_transaction guard_foreclose_balance(string guard_account,
	   string amount,
	   string asset_symbol,
	   bool broadcast = false) {
	   try {
		   FC_ASSERT(!self.is_locked());
		   fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
		   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
		   auto& iter = _wallet.my_accounts.get<by_name>();
		   FC_ASSERT(iter.find(guard_account) != iter.end(), "Could not find account name ${account}", ("account", guard_account));
		   fc::optional<guard_member_object> guard_obj = get_guard_member(guard_account);
		   FC_ASSERT(guard_obj, "Could not find miner matching ${guard}", ("guard", guard_account));
		   guard_foreclose_balance_operation guard_fcb_op;
		   //lockbalance_operation lb_op;
		   guard_fcb_op.foreclose_asset_id = asset_obj->get_id();
		   guard_fcb_op.foreclose_asset_amount = asset_obj->amount_from_string(amount).amount;
		   guard_fcb_op.foreclose_balance_account = guard_obj->id;
		   guard_fcb_op.foreclose_balance_account_id = iter.find(guard_account)->get_id();
		   guard_fcb_op.foreclose_address = iter.find(guard_account)->addr;
		   signed_transaction tx;

		   tx.operations.push_back(guard_fcb_op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();

		   return sign_transaction(tx, broadcast);
	   }FC_CAPTURE_AND_RETHROW((guard_account)(amount)(asset_symbol)(broadcast))
   }
   string transfer_from_to_address(string from, string to, string amount, string asset_symbol, string memo)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
		   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
		   transfer_operation xfer_op;
		   xfer_op.from_addr = address(from);
		   xfer_op.to_addr = address(to);
		   xfer_op.amount = asset_obj->amount_from_string(amount);
		   xfer_op.guarantee_id = get_guarantee_id();
		   if(memo.size())
		   {
			   xfer_op.memo = memo_data();
			   xfer_op.memo->from = public_key_type();
			   xfer_op.memo->to = public_key_type();
			   xfer_op.memo->set_message(private_key_type(),
				   public_key_type(), memo);

		   }
		   signed_transaction tx;
		   tx.operations.push_back(xfer_op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);

		   uint32_t expiration_time_offset = 0;
		   auto dyn_props = get_dynamic_global_properties();
		   tx.set_reference_block(dyn_props.head_block_id);
		   tx.set_expiration(dyn_props.time + fc::seconds(3600*24 + expiration_time_offset));
		   tx.validate();
		   auto json_str = fc::json::to_string(tx);
		   auto base_str = fc::to_base58(json_str.c_str(), json_str.size());
		   return base_str;
		   //return tx;

	   } FC_CAPTURE_AND_RETHROW((from)(to)(amount)(asset_symbol)(memo))
   }

   full_transaction combine_transaction(const vector<signed_transaction>& trxs, bool broadcast)
   {
	   try {
		   FC_ASSERT(!is_locked());
		   signed_transaction trx;
		   if (trxs.size() > 0)
			   trx = trxs[0];
		   trx.signatures.clear();
		   for (const auto& tx : trxs)
		   {
			   for (const auto& sig : tx.signatures)
			   {
				   if (std::find(trx.signatures.begin(),trx.signatures.end(),sig) == trx.signatures.end())
					   trx.signatures.push_back(sig);
			   }
		   }
		   if (broadcast)
			   _remote_net_broadcast->broadcast_transaction(trx);
		   return trx;

	   }FC_CAPTURE_AND_RETHROW((trxs)(broadcast))
   }


   full_transaction transfer_to_address(string from, string to, string amount,
	   string asset_symbol, string memo, bool broadcast = false)
   {
	   try {
		   FC_ASSERT(!self.is_locked());
		   fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
		   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
		   auto& iter = _wallet.my_accounts.get<by_name>();
		   FC_ASSERT(iter.find(from) != iter.end(), "Could not find account name ${account}", ("account", from));

		   transfer_operation xfer_op;
		   xfer_op.from_addr = iter.find(from)->addr;
		   xfer_op.to_addr = address(to);
		   FC_ASSERT(xfer_op.to_addr.version != addressVersion::CONTRACT,"address should not be a contract address.");
		   xfer_op.amount = asset_obj->amount_from_string(amount);
		   xfer_op.guarantee_id=get_guarantee_id();
		   if (memo.size())
		   {
			   xfer_op.memo = memo_data();
			   xfer_op.memo->from = public_key_type();
			   xfer_op.memo->to = public_key_type();
			   xfer_op.memo->set_message(private_key_type(),
			      public_key_type(), memo);
			   
		   }
		   signed_transaction tx;

		   tx.operations.push_back(xfer_op);
		   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
		   tx.validate();

		   return sign_transaction(tx, broadcast);
	   } FC_CAPTURE_AND_RETHROW((from)(to)(amount)(asset_symbol)(memo)(broadcast))

   }

   string  lightwallet_broadcast(signed_transaction tx)        
   {
       try {

         _remote_net_broadcast->broadcast_transaction(tx); 
         return tx.id().str();

        }FC_CAPTURE_AND_RETHROW((tx))
      
      
   }

   string lightwallet_get_refblock_info()
   {
       try {

         auto dyn_props = get_dynamic_global_properties();
         transaction tmp;
         tmp.set_reference_block(dyn_props.head_block_id);
        
         auto res = fc::to_string(tmp.ref_block_num);
         res += "," + fc::to_string(tmp.ref_block_prefix);
         return res;

       }FC_CAPTURE_AND_RETHROW()
   }




   full_transaction transfer(string from, string to, string amount,
                               string asset_symbol, string memo, bool broadcast = false)
   { try {
      FC_ASSERT( !self.is_locked() );
      fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
      FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

      account_object from_account = get_account(from);
      account_object to_account = get_account(to);
      account_id_type from_id = from_account.id;
      account_id_type to_id = get_account_id(to);

      transfer_operation xfer_op;

      xfer_op.from = from_id;
      xfer_op.to = to_id;
      xfer_op.amount = asset_obj->amount_from_string(amount);

      if( memo.size() )
         {
            xfer_op.memo = memo_data();
            xfer_op.memo->from = from_account.options.memo_key;
            xfer_op.memo->to = to_account.options.memo_key;
            xfer_op.memo->set_message(get_private_key(from_account.options.memo_key),
                                      to_account.options.memo_key, memo);
         }

      signed_transaction tx;
      tx.operations.push_back(xfer_op);
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   } FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(asset_symbol)(memo)(broadcast) ) }

   full_transaction issue_asset(string to_account, string amount, string symbol,
                                  string memo, bool broadcast = false)
   {
      auto asset_obj = get_asset(symbol);

      account_object to = get_account(to_account);
      account_object issuer = get_account(asset_obj.issuer);

      asset_issue_operation issue_op;
      issue_op.issuer           = asset_obj.issuer;
      issue_op.asset_to_issue   = asset_obj.amount_from_string(amount);
      issue_op.issue_to_account = to.id;

      if( memo.size() )
      {
         issue_op.memo = memo_data();
         issue_op.memo->from = issuer.options.memo_key;
         issue_op.memo->to = to.options.memo_key;
         issue_op.memo->set_message(get_private_key(issuer.options.memo_key),
                                    to.options.memo_key, memo);
      }

      signed_transaction tx;
      tx.operations.push_back(issue_op);
      set_operation_fees(tx,_remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }

   std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const
   {
      std::map<string,std::function<string(fc::variant,const fc::variants&)> > m;
      m["help"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["gethelp"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["get_account_history"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<vector<operation_detail>>();
         std::stringstream ss;

         for( operation_detail& d : r )
         {
            operation_history_object& i = d.op;
            auto b = _remote_db->get_block_header(i.block_num);
            FC_ASSERT(b);
            ss << b->timestamp.to_iso_string() << " ";
            i.op.visit(operation_printer(ss, *this, i.result));
            ss << " \n";
         }

         return ss.str();
      };
      m["get_relative_account_history"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<vector<operation_detail>>();
         std::stringstream ss;

         for( operation_detail& d : r )
         {
            operation_history_object& i = d.op;
            auto b = _remote_db->get_block_header(i.block_num);
            FC_ASSERT(b);
            ss << b->timestamp.to_iso_string() << " ";
            i.op.visit(operation_printer(ss, *this, i.result));
            ss << " \n";
         }

         return ss.str();
      };

      m["list_account_balances"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<vector<asset>>();
         vector<asset_object> asset_recs;
         std::transform(r.begin(), r.end(), std::back_inserter(asset_recs), [this](const asset& a) {
            return get_asset(a.asset_id);
         });

         std::stringstream ss;
         for( unsigned i = 0; i < asset_recs.size(); ++i )
            ss << asset_recs[i].amount_to_pretty_string(r[i]) << "\n";

         return ss.str();
      };

      m["get_blind_balances"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<vector<asset>>();
         vector<asset_object> asset_recs;
         std::transform(r.begin(), r.end(), std::back_inserter(asset_recs), [this](const asset& a) {
            return get_asset(a.asset_id);
         });

         std::stringstream ss;
         for( unsigned i = 0; i < asset_recs.size(); ++i )
            ss << asset_recs[i].amount_to_pretty_string(r[i]) << "\n";

         return ss.str();
      };
      m["transfer_to_blind"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<blind_confirmation>();
         std::stringstream ss;
         r.trx.operations[0].visit( operation_printer( ss, *this, operation_result() ) );
         ss << "\n";
         for( const auto& out : r.outputs )
         {
            asset_object a = get_asset( out.decrypted_memo.amount.asset_id );
            ss << a.amount_to_pretty_string( out.decrypted_memo.amount ) << " to  " << out.label << "\n\t  receipt: " << out.confirmation_receipt <<"\n\n";
         }
         return ss.str();
      };
      m["blind_transfer"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<blind_confirmation>();
         std::stringstream ss;
         r.trx.operations[0].visit( operation_printer( ss, *this, operation_result() ) );
         ss << "\n";
         for( const auto& out : r.outputs )
         {
            asset_object a = get_asset( out.decrypted_memo.amount.asset_id );
            ss << a.amount_to_pretty_string( out.decrypted_memo.amount ) << " to  " << out.label << "\n\t  receipt: " << out.confirmation_receipt <<"\n\n";
         }
         return ss.str();
      };
      m["receive_blind_transfer"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<blind_receipt>();
         std::stringstream ss;
         asset_object as = get_asset( r.amount.asset_id );
         ss << as.amount_to_pretty_string( r.amount ) << "  " << r.from_label << "  =>  " << r.to_label  << "  " << r.memo <<"\n";
         return ss.str();
      };
      m["blind_history"] = [this](variant result, const fc::variants& a)
      {
         auto records = result.as<vector<blind_receipt>>();
         std::stringstream ss;
         ss << "WHEN         "
            << "  " << "AMOUNT"  << "  " << "FROM" << "  =>  " << "TO" << "  " << "MEMO" <<"\n";
         ss << "====================================================================================\n";
         for( auto& r : records )
         {
            asset_object as = get_asset( r.amount.asset_id );
            ss << fc::get_approximate_relative_time_string( r.date )
               << "  " << as.amount_to_pretty_string( r.amount ) << "  " << r.from_label << "  =>  " << r.to_label  << "  " << r.memo <<"\n";
         }
         return ss.str();
      };
      m["get_order_book"] = [this](variant result, const fc::variants& a)
      {
         auto orders = result.as<order_book>();
         auto bids = orders.bids;
         auto asks = orders.asks;
         std::stringstream ss;
         std::stringstream sum_stream;
         sum_stream << "Sum(" << orders.base << ')';
         double bid_sum = 0;
         double ask_sum = 0;
         const int spacing = 20;

         auto prettify_num = [&]( double n )
         {
            //ss << n;
            if (abs( round( n ) - n ) < 0.00000000001 )
            {
               //ss << setiosflags( !ios::fixed ) << (int) n;     // doesn't compile on Linux with gcc
               ss << (int) n;
            }
            else if (n - floor(n) < 0.000001)
            {
               ss << setiosflags( ios::fixed ) << setprecision(10) << n;
            }
            else
            {
               ss << setiosflags( ios::fixed ) << setprecision(6) << n;
            }
         };

         ss << setprecision( 8 ) << setiosflags( ios::fixed ) << setiosflags( ios::left );

         ss << ' ' << setw( (spacing * 4) + 6 ) << "BUY ORDERS" << "SELL ORDERS\n"
            << ' ' << setw( spacing + 1 ) << "Price" << setw( spacing ) << orders.quote << ' ' << setw( spacing )
            << orders.base << ' ' << setw( spacing ) << sum_stream.str()
            << "   " << setw( spacing + 1 ) << "Price" << setw( spacing ) << orders.quote << ' ' << setw( spacing )
            << orders.base << ' ' << setw( spacing ) << sum_stream.str()
            << "\n====================================================================================="
            << "|=====================================================================================\n";

         for (int i = 0; i < bids.size() || i < asks.size() ; i++)
         {
            if ( i < bids.size() )
            {
                bid_sum += bids[i].base;
                ss << ' ' << setw( spacing );
                prettify_num( bids[i].price );
                ss << ' ' << setw( spacing );
                prettify_num( bids[i].quote );
                ss << ' ' << setw( spacing );
                prettify_num( bids[i].base );
                ss << ' ' << setw( spacing );
                prettify_num( bid_sum );
                ss << ' ';
            }
            else
            {
                ss << setw( (spacing * 4) + 5 ) << ' ';
            }

            ss << '|';

            if ( i < asks.size() )
            {
               ask_sum += asks[i].base;
               ss << ' ' << setw( spacing );
               prettify_num( asks[i].price );
               ss << ' ' << setw( spacing );
               prettify_num( asks[i].quote );
               ss << ' ' << setw( spacing );
               prettify_num( asks[i].base );
               ss << ' ' << setw( spacing );
               prettify_num( ask_sum );
            }

            ss << '\n';
         }

         ss << endl
            << "Buy Total:  " << bid_sum << ' ' << orders.base << endl
            << "Sell Total: " << ask_sum << ' ' << orders.base << endl;

         return ss.str();
      };


      return m;
   }
   
	full_transaction propose_guard_pledge_change(
	  const string& proposing_account,
	  fc::time_point_sec expiration_time,
	  const variant_object& changed_values,
	  bool broadcast = false)
	{
	  FC_ASSERT(changed_values.contains("minimum_guard_pledge_line"));
	  variant_object temp_asset_set1 = changed_values.find("minimum_guard_pledge_line")->value().get_object();
	  const chain_parameters& current_params = get_global_properties().parameters;
	  chain_parameters new_params = current_params;
	  for (const auto& item : temp_asset_set1) {
		  new_params.minimum_guard_pledge_line[item.key()] = asset(item.value().as_uint64(), get_asset_id(item.key()));
	  }
	  committee_member_update_global_parameters_operation update_op;
	  update_op.new_parameters = new_params;

	  proposal_create_operation prop_op;
	  prop_op.proposer = get_account(proposing_account).get_id();
	  prop_op.expiration_time = expiration_time;
	  prop_op.fee_paying_account = get_account(proposing_account).addr;
	  prop_op.proposed_ops.emplace_back(update_op);
	  current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);
	
	  signed_transaction tx;
	  tx.operations.push_back(prop_op);
	  set_operation_fees(tx, current_params.current_fees);
	  tx.validate();
	
	  return sign_transaction(tx, broadcast);
	}

	full_transaction propose_pay_back_asset_rate_change(
		const string& proposing_account,
		const int64_t& expiration_time,
		const variant_object& changed_values,
		bool broadcast = false
	) {
		FC_ASSERT(changed_values.contains("min_pay_back_balance_other_asset"));
		auto temp_asset_set1 = changed_values.find("min_pay_back_balance_other_asset")->value().get_object();
		const chain_parameters& current_params = get_global_properties().parameters;
		
		chain_parameters new_params = current_params;
		for (const auto& item : temp_asset_set1) {
			new_params.min_pay_back_balance_other_asset[item.key()] = asset(item.value().as_uint64(), get_asset_id(item.key()));
		}
		committee_member_update_global_parameters_operation update_op;
		update_op.new_parameters = new_params;

		proposal_create_operation prop_op;
		auto guard_obj = get_guard_member(proposing_account);
		prop_op.proposer = get_account(proposing_account).get_id();
		prop_op.fee_paying_account = get_account(proposing_account).addr;
		prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);

		prop_op.proposed_ops.emplace_back(update_op);
		current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);

		signed_transaction tx;
		tx.operations.push_back(prop_op);
		set_operation_fees(tx, current_params.current_fees);
		tx.validate();

		return sign_transaction(tx, broadcast);
	}

   full_transaction propose_parameter_change(
      const string& proposing_account,
     const int64_t& expiration_time,
      const variant_object& changed_values,
      bool broadcast = false)
   {
      FC_ASSERT( !changed_values.contains("current_fees") );

      const chain_parameters& current_params = get_global_properties().parameters;
      chain_parameters new_params = current_params;
      fc::reflector<chain_parameters>::visit(
         fc::from_variant_visitor<chain_parameters>( changed_values, new_params )
         );

      committee_member_update_global_parameters_operation update_op;
      update_op.new_parameters = new_params;

      proposal_create_operation prop_op;
	  auto guard_obj = get_guard_member(proposing_account);
	  prop_op.proposer = get_account(proposing_account).get_id();
	  prop_op.fee_paying_account = get_account(proposing_account).addr;
      prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
      prop_op.proposed_ops.emplace_back( update_op );
      current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

      signed_transaction tx;
      tx.operations.push_back(prop_op);
      set_operation_fees(tx, current_params.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }



   full_transaction propose_coin_destory(
	   const string& proposing_account,
	   fc::time_point_sec expiration_time,
	   const variant_object& destory_values,
	   bool broadcast = false)
   {
	   FC_ASSERT(destory_values.contains("loss_asset"));
	   FC_ASSERT(destory_values.contains("loss_asset_symbol"));
	   FC_ASSERT(destory_values.contains("commitee_member_handle_percent"));
	   

	   const chain_parameters& current_params = get_global_properties().parameters;
	   asset loss_asset;
	   asset_id_type asset_id = get_asset_id(destory_values.find("loss_asset_symbol")->value().as_string());
	   

	   loss_asset.asset_id = asset_id;
	   loss_asset.amount = destory_values.find("loss_asset")->value().as_uint64();
	   committee_member_execute_coin_destory_operation update_op;
	   update_op.loss_asset = loss_asset;
	   update_op.commitee_member_handle_percent = destory_values.find("commitee_member_handle_percent")->value().as_int64();

	   proposal_create_operation prop_op;

	   prop_op.expiration_time = expiration_time;

	   prop_op.fee_paying_account = get_account(proposing_account).addr;

	   prop_op.proposed_ops.emplace_back(update_op);
	   current_params.current_fees->set_fee(prop_op.proposed_ops.back().op);

	   signed_transaction tx;
	   tx.operations.push_back(prop_op);
	   set_operation_fees(tx, current_params.current_fees);
	   tx.validate();

	   return sign_transaction(tx, broadcast);
   }


   full_transaction propose_fee_change(
      const string& proposing_account,
      int64_t expiration_time,
      const variant_object& changed_fees,
      bool broadcast = false)
   {
      const chain_parameters& current_params = get_global_properties().parameters;
      const fee_schedule_type& current_fees = *(current_params.current_fees);

      flat_map< int, fee_parameters > fee_map;
      fee_map.reserve( current_fees.parameters.size() );
      for( const fee_parameters& op_fee : current_fees.parameters )
         fee_map[ op_fee.which() ] = op_fee;
      uint32_t scale = current_fees.scale;

      for( const auto& item : changed_fees )
      {
         const string& key = item.key();
         if( key == "scale" )
         {
            int64_t _scale = item.value().as_int64();
            FC_ASSERT( _scale >= 0 );
            FC_ASSERT( _scale <= std::numeric_limits<uint32_t>::max() );
            scale = uint32_t( _scale );
            continue;
         }
         // is key a number?
         auto is_numeric = [&]() -> bool
         {
            size_t n = key.size();
            for( size_t i=0; i<n; i++ )
            {
               if( !isdigit( key[i] ) )
                  return false;
            }
            return true;
         };

         int which;
         if( is_numeric() )
            which = std::stoi( key );
         else
         {
            const auto& n2w = _operation_which_map.name_to_which;
            auto it = n2w.find( key );
            FC_ASSERT( it != n2w.end(), "unknown operation" );
            which = it->second;
         }

         fee_parameters fp = from_which_variant< fee_parameters >( which, item.value() );
         fee_map[ which ] = fp;
      }

      fee_schedule_type new_fees;

      for( const std::pair< int, fee_parameters >& item : fee_map )
         new_fees.parameters.insert( item.second );
      new_fees.scale = scale;

      chain_parameters new_params = current_params;
      new_params.current_fees = new_fees;

      committee_member_update_global_parameters_operation update_op;
      update_op.new_parameters = new_params;

      proposal_create_operation prop_op;

      prop_op.expiration_time = fc::time_point_sec(time_point::now()) + fc::seconds(expiration_time);
      prop_op.fee_paying_account = get_account(proposing_account).addr;

      prop_op.proposed_ops.emplace_back( update_op );
      current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

      signed_transaction tx;
      tx.operations.push_back(prop_op);
      set_operation_fees(tx, current_params.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }

   full_transaction approve_proposal(
      const string& fee_paying_account,
      const string& proposal_id,
      const approval_delta& delta,
      bool broadcast = false)
   {
      proposal_update_operation update_op;

      update_op.fee_paying_account = get_account(fee_paying_account).addr;
      update_op.proposal = fc::variant(proposal_id).as<proposal_id_type>();
      // make sure the proposal exists
      get_object( update_op.proposal );

      for( const std::string& name : delta.active_approvals_to_add )
         update_op.active_approvals_to_add.insert( get_account( name ).id );
      for( const std::string& name : delta.active_approvals_to_remove )
         update_op.active_approvals_to_remove.insert( get_account( name ).id );
      for( const std::string& name : delta.owner_approvals_to_add )
         update_op.owner_approvals_to_add.insert( get_account( name ).id );
      for( const std::string& name : delta.owner_approvals_to_remove )
         update_op.owner_approvals_to_remove.insert( get_account( name ).id );
      for( const std::string& k : delta.key_approvals_to_add )
         update_op.key_approvals_to_add.insert( address( k ) );
      for( const std::string& k : delta.key_approvals_to_remove )
         update_op.key_approvals_to_remove.insert( address( k ) );

      signed_transaction tx;
      tx.operations.push_back(update_op);
      set_operation_fees(tx, get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }

   full_transaction approve_referendum(const string& fee_paying_account,
	   const string& referendum_id,
	   const approval_delta& delta,
	   bool broadcast = false)
   {
	   referendum_update_operation update_op;

	   update_op.fee_paying_account = get_account(fee_paying_account).addr;
	   update_op.referendum = fc::variant(referendum_id).as<referendum_id_type>();
	   // make sure the proposal exists
	   get_object(update_op.referendum);
	   for (const std::string& k : delta.key_approvals_to_add)
		   update_op.key_approvals_to_add.insert(address(k));
	   for (const std::string& k : delta.key_approvals_to_remove)
		   update_op.key_approvals_to_remove.insert(address(k));

	   signed_transaction tx;
	   tx.operations.push_back(update_op);
	   set_operation_fees(tx, get_global_properties().parameters.current_fees);
	   tx.validate();
	   return sign_transaction(tx, broadcast);
   }

   vector<proposal_object>  get_proposal(const string& proposer)
   {
	   auto acc = get_account(proposer);
	   FC_ASSERT(acc.get_id() != account_object().get_id(),"the propser doesnt exist in the chain.");

	   return _remote_db->get_proposer_transactions(acc.get_id());
   }
   vector<referendum_object> get_referendum_for_voter(const string& voter)
   {
	   auto acc = get_account(voter);
	   FC_ASSERT(acc.get_id() != account_object().get_id(), "the propser doesnt exist in the chain.");
	   return _remote_db->get_referendum_transactions_waiting(acc.addr);
   }

   vector<proposal_object>  get_proposal_for_voter(const string& voter )
   {
	   auto acc = get_account(voter);
	   FC_ASSERT(acc.get_id() != account_object().get_id(), "the propser doesnt exist in the chain.");

	   return _remote_db->get_voter_transactions_waiting(acc.addr);
   }
   /*
   void dbg_make_uia(string creator, string symbol)
   {
      asset_options opts;
      opts.flags &= ~(white_list | disable_force_settle | global_settle);
      opts.issuer_permissions = opts.flags;
      opts.core_exchange_rate = price(asset(1), asset(1,asset_id_type(1)));
      create_asset(get_account(creator).name, symbol, 2, opts, {}, true);
   }

   void dbg_make_mia(string creator, string symbol)
   {
      asset_options opts;
      opts.flags &= ~white_list;
      opts.issuer_permissions = opts.flags;
      opts.core_exchange_rate = price(asset(1), asset(1,asset_id_type(1)));
      bitasset_options bopts;
      create_asset(get_account(creator).name, symbol, 2, opts, bopts, true);
   }
   */

   void dbg_push_blocks( const std::string& src_filename, uint32_t count )
   {
      use_debug_api();
      (*_remote_debug)->debug_push_blocks( src_filename, count );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_generate_blocks( const std::string& debug_wif_key, uint32_t count )
   {
      use_debug_api();
      (*_remote_debug)->debug_generate_blocks( debug_wif_key, count );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_stream_json_objects( const std::string& filename )
   {
      use_debug_api();
      (*_remote_debug)->debug_stream_json_objects( filename );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_update_object( const fc::variant_object& update )
   {
      use_debug_api();
      (*_remote_debug)->debug_update_object( update );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void use_network_node_api()
   {
      if( _remote_net_node )
         return;
      try
      {
         _remote_net_node = _remote_api->network_node();
      }
      catch( const fc::exception& e )
      {
         std::cerr << "\nCouldn't get network node API.  You probably are not configured\n"
         "to access the network API on the witness_node you are\n"
         "connecting to.  Please follow the instructions in README.md to set up an apiaccess file.\n"
         "\n";
         throw(e);
      }
   }

   void use_debug_api()
   {
      if( _remote_debug )
         return;
      try
      {
        _remote_debug = _remote_api->debug();
      }
      catch( const fc::exception& e )
      {
         std::cerr << "\nCouldn't get debug node API.  You probably are not configured\n"
         "to access the debug API on the node you are connecting to.\n"
         "\n"
         "To fix this problem:\n"
         "- Please ensure you are running debug_node, not witness_node.\n"
         "- Please follow the instructions in README.md to set up an apiaccess file.\n"
         "\n";
      }
   }
   void use_miner_api()
   {
       if (_remote_miner)
           return;
       try
       {
           _remote_miner = _remote_api->miner();
       }
       catch (const fc::exception& e)
       {
           std::cerr << "\nCouldn't get miner API.  You probably are not configured\n"
               "to access the network API on the witness_node you are\n"
               "connecting to.  Please follow the instructions in README.md to set up an apiaccess file.\n"
               "\n";
           throw(e);
       }
   }
   void use_localnode_api()
   {
       if (_remote_local_node)
           return;
       try
       {
           _remote_local_node = _remote_api->localnode();
       }
       catch (const fc::exception& e)
       {
           std::cerr << "\nCouldn't get localnode API.  You probably are not configured\n"
               "to access the network API on the witness_node you are\n"
               "connecting to.  Please follow the instructions in README.md to set up an apiaccess file.\n"
               "\n";
           throw(e);
       }
   }
   void witness_node_stop()
   {
       use_localnode_api();
       (*_remote_local_node)->witness_node_stop();
   }
   void start_miner(bool start)
   {
       use_miner_api();
       (*_remote_miner)->start_miner(start);
   }
   void start_mining(const std::map<chain::miner_id_type, fc::ecc::private_key>& keys)
   {
	   use_miner_api();
	   if(!keys.empty())
			(*_remote_miner)->set_miner(keys,true);
	   else
		   (*_remote_miner)->set_miner(keys, false);
   }
   void network_add_nodes( const vector<string>& nodes )
   {
      use_network_node_api();
      for( const string& node_address : nodes )
      {
         (*_remote_net_node)->add_node( fc::ip::endpoint::from_string( node_address ) );
      }
   }

   vector< variant > network_get_connected_peers()
   {
      use_network_node_api();
      const auto peers = (*_remote_net_node)->get_connected_peers();
      vector< variant > result;
      result.reserve( peers.size() );
      for( const auto& peer : peers )
      {
         variant v;
         fc::to_variant( peer, v );
         result.push_back( v );
      }
      return result;
   }

   fc::variant_object network_get_info()
   {
	   use_network_node_api();
	   return (*_remote_net_node)->get_info();
   }

   void flood_network(string prefix, uint32_t number_of_transactions)
   {
      try
      {
         const account_object& master = *_wallet.my_accounts.get<by_name>().lower_bound("import");
         int number_of_accounts = number_of_transactions / 3;
         number_of_transactions -= number_of_accounts;
         //auto key = derive_private_key("floodshill", 0);
         try {
           // dbg_make_uia(master.name, "SHILL");
         } catch(...) {/* Ignore; the asset probably already exists.*/}

         fc::time_point start = fc::time_point::now();
         for( int i = 0; i < number_of_accounts; ++i )
         {
            std::ostringstream brain_key;
            brain_key << "brain key for account " << prefix << i;
            signed_transaction trx = create_account_with_brain_key(brain_key.str(), prefix + fc::to_string(i), master.name, master.name, /* broadcast = */ true, /* save wallet = */ false);
         }
         fc::time_point end = fc::time_point::now();
         ilog("Created ${n} accounts in ${time} milliseconds",
              ("n", number_of_accounts)("time", (end - start).count() / 1000));

         start = fc::time_point::now();
         for( int i = 0; i < number_of_accounts; ++i )
         {
            signed_transaction trx = transfer(master.name, prefix + fc::to_string(i), "10", "CORE", "", true);
            trx = transfer(master.name, prefix + fc::to_string(i), "1", "CORE", "", true);
         }
         end = fc::time_point::now();
         ilog("Transferred to ${n} accounts in ${time} milliseconds",
              ("n", number_of_accounts*2)("time", (end - start).count() / 1000));

         start = fc::time_point::now();
         for( int i = 0; i < number_of_accounts; ++i )
         {
            signed_transaction trx = issue_asset(prefix + fc::to_string(i), "1000", "SHILL", "", true);
         }
         end = fc::time_point::now();
         ilog("Issued to ${n} accounts in ${time} milliseconds",
              ("n", number_of_accounts)("time", (end - start).count() / 1000));
      }
      catch (...)
      {
         throw;
      }

   }

   operation get_prototype_operation( string operation_name )
   {
      auto it = _prototype_ops.find( operation_name );
      if( it == _prototype_ops.end() )
         FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
      return it->second;
   }

   string                  _wallet_filename;
   wallet_data             _wallet;

   //map<public_key_type,string> _keys;
   map<address, string> _keys;
   optional<brain_key_usage_info> _current_brain_key;
   fc::sha512                  _checksum;

   chain_id_type           _chain_id;
   fc::api<login_api>      _remote_api;
   fc::api<database_api>   _remote_db;
   fc::api<network_broadcast_api>   _remote_net_broadcast;
   fc::api<history_api>    _remote_hist;
   fc::api<transaction_api> _remote_trx;
   optional<fc::api<miner_api>> _remote_miner;
   optional<fc::api<localnode_api>> _remote_local_node;
   optional< fc::api<network_node_api> > _remote_net_node;
   optional< fc::api<graphene::debug_miner::debug_api> > _remote_debug;
   optional< fc::api<crosschain_api> >   _crosschain_manager;
   flat_map<string, operation> _prototype_ops;
   optional<guarantee_object_id_type>    _guarantee_id;
   map<string, crosschain_prkeys> _crosschain_keys;
   static_variant_map _operation_which_map = create_static_variant_map< operation >();

#ifdef __unix__
   mode_t                  _old_umask;
#endif
   const string _wallet_filename_extension = ".wallet";

   mutable map<asset_id_type, asset_object> _asset_cache;
};

std::string operation_printer::fee(const asset& a)const {
   out << "   (Fee: " << wallet.get_asset(a.asset_id).amount_to_pretty_string(a) << ")";
   return "";
}

template<typename T>
std::string operation_printer::operator()(const T& op)const
{
   //balance_accumulator acc;
   //op.get_balance_delta( acc, result );
   auto a = wallet.get_asset( op.fee.asset_id );
   auto payer = wallet.get_account( op.fee_payer() );

   string op_name = fc::get_typename<T>::name();
   if( op_name.find_last_of(':') != string::npos )
      op_name.erase(0, op_name.find_last_of(':')+1);
   out << op_name <<" ";
  // out << "balance delta: " << fc::json::to_string(acc.balance) <<"   ";
   out << payer.name << " fee: " << a.amount_to_pretty_string( op.fee );
   operation_result_printer rprinter(wallet);
   std::string str_result = result.visit(rprinter);
   if( str_result != "" )
   {
      out << "   result: " << str_result;
   }
   return "";
}
std::string operation_printer::operator()(const transfer_from_blind_operation& op)const
{
   auto a = wallet.get_asset( op.fee.asset_id );
   auto receiver = wallet.get_account( op.to );

   out <<  receiver.name
   << " received " << a.amount_to_pretty_string( op.amount ) << " from blinded balance";
   return "";
}
std::string operation_printer::operator()(const transfer_to_blind_operation& op)const
{
   auto fa = wallet.get_asset( op.fee.asset_id );
   auto a = wallet.get_asset( op.amount.asset_id );
   auto sender = wallet.get_account( op.from );

   out <<  sender.name
   << " sent " << a.amount_to_pretty_string( op.amount ) << " to " << op.outputs.size() << " blinded balance" << (op.outputs.size()>1?"s":"")
   << " fee: " << fa.amount_to_pretty_string( op.fee );
   return "";
}
string operation_printer::operator()(const transfer_operation& op) const
{
   out << "Transfer " << wallet.get_asset(op.amount.asset_id).amount_to_pretty_string(op.amount)
       << " from " << wallet.get_account(op.from).name << " to " << wallet.get_account(op.to).name;
   std::string memo;
   if( op.memo )
   {
      if( wallet.is_locked() )
      {
         out << " -- Unlock wallet to see memo.";
      } else {
         try {
            //FC_ASSERT(wallet._keys.count(op.memo->to) || wallet._keys.count(op.memo->from), "Memo is encrypted to a key ${to} or ${from} not in this wallet.", ("to", op.memo->to)("from",op.memo->from));
            if( wallet._keys.count(op.memo->to) ) {
               auto my_key = wif_to_key(wallet._keys.at(op.memo->to));
               FC_ASSERT(my_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
               memo = op.memo->get_message(*my_key, op.memo->from);
               out << " -- Memo: " << memo;
            } else if (wallet._keys.count(op.memo->from)){
               auto my_key = wif_to_key(wallet._keys.at(op.memo->from));
               FC_ASSERT(my_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
               memo = op.memo->get_message(*my_key, op.memo->to);
               out << " -- Memo: " << memo;
            }
			else {
				memo = op.memo->get_message(private_key_type(), public_key_type());
				out << " -- Memo: " << memo;
			}
         } catch (const fc::exception& e) {
            out << " -- could not decrypt memo";
            elog("Error when decrypting memo: ${e}", ("e", e.to_detail_string()));
         }
      }
   }
   fee(op.fee);
   return memo;
}

std::string operation_printer::operator()(const account_create_operation& op) const
{
   out << "Create Account '" << op.name << "'";
   return fee(op.fee);
}

std::string operation_printer::operator()(const account_update_operation& op) const
{
   out << "Update Account '" << wallet.get_account(op.account).name << "'";
   return fee(op.fee);
}

std::string operation_printer::operator()(const asset_create_operation& op) const
{
   out << "Create ";
   if( op.bitasset_opts.valid() )
      out << "BitAsset ";
   else
      out << "User-Issue Asset ";
   out << "'" << op.symbol << "' with issuer " << wallet.get_account(op.issuer).name;
   return fee(op.fee);
}

std::string operation_result_printer::operator()(const void_result& x) const
{
   return "";
}

std::string operation_result_printer::operator()(const object_id_type& oid)
{
   return std::string(oid);
}

std::string operation_result_printer::operator()(const asset& a)
{
   return _wallet.get_asset(a.asset_id).amount_to_pretty_string(a);
}

std::string operation_result_printer::operator()(const string& a)
{
	return a;
}



}}}

namespace graphene { namespace wallet {
	bool crosschain_prkeys::operator==(const crosschain_prkeys& key) const
	{
		return (addr == key.addr) && (pubkey == key.pubkey) && (wif_key == key.wif_key);
	}
   vector<brain_key_info> utility::derive_owner_keys_from_brain_key(string brain_key, int number_of_desired_keys)
   {
      // Safety-check
      FC_ASSERT( number_of_desired_keys >= 1 );

      // Create as many derived owner keys as requested
      vector<brain_key_info> results;
      brain_key = graphene::wallet::detail::normalize_brain_key(brain_key);
      for (int i = 0; i < number_of_desired_keys; ++i) {
        fc::ecc::private_key priv_key = graphene::wallet::detail::derive_private_key( brain_key, i );

        brain_key_info result;
        result.brain_priv_key = brain_key;
        result.wif_priv_key = key_to_wif( priv_key );
        result.pub_key = priv_key.get_public_key();

        results.push_back(result);
      }

      return results;
   }
}}

namespace graphene { namespace wallet {

wallet_api::wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi)
   : my(new detail::wallet_api_impl(*this, initial_data, rapi))
{
}

wallet_api::~wallet_api()
{
}

bool wallet_api::copy_wallet_file(string destination_filename)
{
   return my->copy_wallet_file(destination_filename);
}

optional<signed_block_with_info> wallet_api::get_block(uint32_t num)
{
   auto block = my->_remote_db->get_block(num);
   if (!block.valid())
	   return optional<signed_block_with_info>();
   signed_block_with_info block_with_info(*block);
   auto trxs = my->_remote_db->fetch_block_transactions(num);
   vector <transaction_id_type> tx_ids;
   for (const auto & full_trx :trxs)
   {
	   tx_ids.push_back(full_trx.trxid);
   }
   auto glob_info = get_global_properties();
   block_with_info.reward = my->_remote_db->get_miner_pay_per_block(num)+block->trxfee;
   block_with_info.transaction_ids.swap(tx_ids);
   return block_with_info;
}

uint64_t wallet_api::get_account_count() const
{
   return my->_remote_db->get_account_count();
}

vector<account_object> wallet_api::list_my_accounts()
{
   return vector<account_object>(my->_wallet.my_accounts.begin(), my->_wallet.my_accounts.end());
}

map<string,account_id_type> wallet_api::list_accounts(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_accounts(lowerbound, limit);
}

vector<asset> wallet_api::list_account_balances(const string& id)
{
   if( auto real_id = maybe_id<account_id_type>(id) )
      return my->_remote_db->get_account_balances(*real_id, flat_set<asset_id_type>());
   return my->_remote_db->get_account_balances(my->get_account(id).id, flat_set<asset_id_type>());
}

std::vector<guard_lock_balance_object> wallet_api::get_senator_lock_balance(const string& miner)const {
	return my->get_guard_lock_balance(miner);
}
std::vector<lockbalance_object> wallet_api::get_citizen_lock_balance(const string& miner)const {
	return my->get_miner_lock_balance(miner);
}
std::vector<acquired_crosschain_trx_object> wallet_api::get_acquire_transaction(const int & type, const string & trxid) {
	return my->get_acquire_transaction(type, trxid);
}
vector<asset> wallet_api::get_addr_balances(const string& addr)
{
	if (address::is_valid(addr) == false)
	{
		return vector<asset>();
	}
	auto add = address(addr);
	vector<address> vec;
	vec.push_back(add);
	auto vec_balance = my->_remote_db->get_balance_objects(vec);
	vector<asset> ret;
	for (auto balance : vec_balance)
	{
		ret.push_back(balance.balance);
	}

	return ret;

}
variant_object wallet_api::get_multisig_address(const address& addr)
{
	vector<address> vec_addr;
	vec_addr.push_back(addr);
	auto vec_bal = my->_remote_db->get_balance_objects(vec_addr);
	if (vec_bal.size() == 0)
		return variant_object();
	balance_object obj;
	for (auto balance : vec_bal)
	{
		if (balance.multisignatures.valid())
		{
			obj = balance;
			break;
		}
	}
	if (!obj.multisignatures.valid())
		return variant_object();
	map<int, vector<address>> t_map;
	vec_addr.clear();
	for (auto pubkey : obj.multisignatures->begin()->second)
	{
		vec_addr.emplace_back(pubkey);
	}
	t_map[obj.multisignatures->begin()->first] = vec_addr;
	fc::mutable_variant_object ret = fc::variant(obj).as<fc::mutable_variant_object>();
	ret.erase(string("id"));
	ret.erase(string("balance"));
	ret.erase(string("frozen"));
	ret.erase(string("vesting_policy"));
	ret.erase(string("last_claim_date"));
	ret = ret.set("multisignatures", fc::variant(t_map));
	return ret;
}
guard_member_object wallet_api::get_eth_signer(const string& symbol, const string& address) {
	auto guardId = my->get_eth_signer(symbol, address);
	auto guard_member_objects = my->_remote_db->get_guard_member_by_account(guardId);
	if (guard_member_objects.valid())
		return *guard_member_objects;
	return guard_member_object();
	/*
	
	FC_ASSERT(guard_member_objects.size() != 0, "don`t find guard");
	
	auto guard_member = *(guard_member_objects.begin());
	FC_ASSERT(guard_member.valid(), "don`t find guard");
	return *guard_member;
	*/
}
vector<asset> wallet_api::get_account_balances(const string& account)
{
	auto acc = my->get_account(account);
	auto add = acc.addr;
	FC_ASSERT(address() != acc.addr, "account is not in the chain.");
	vector<address> vec;
	vec.push_back(add);
	auto vec_balance = my->_remote_db->get_balance_objects(vec);
	vector<asset> ret;
	for (auto balance : vec_balance)
	{
		ret.push_back(balance.balance);
	}

	return ret;
}

vector<asset_object> wallet_api::list_assets(const string& lowerbound, uint32_t limit)const
{
   return my->_remote_db->list_assets( lowerbound, limit );
}

vector<operation_detail> wallet_api::get_account_history(string name, int limit)const
{
   vector<operation_detail> result;
   auto account_id = my->get_account(name).get_id();

   while( limit > 0 )
   {
      operation_history_id_type start;
      if( result.size() )
      {
         start = result.back().op.id;
         start = start + 1;
      }


      vector<operation_history_object> current = my->_remote_hist->get_account_history(account_id, operation_history_id_type(), std::min(100,limit), start);
      for( auto& o : current ) {
         std::stringstream ss;
         auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
         result.push_back( operation_detail{ memo, ss.str(), o } );
      }
      if( current.size() < std::min(100,limit) )
         break;
      limit -= current.size();
   }

   return result;
}

vector<operation_detail> wallet_api::get_relative_account_history(string name, uint32_t stop, int limit, uint32_t start)const
{
   
   FC_ASSERT( start > 0 || limit <= 100 );
   
   vector<operation_detail> result;
   auto account_id = my->get_account(name).get_id();

   while( limit > 0 )
   {
      vector <operation_history_object> current = my->_remote_hist->get_relative_account_history(account_id, stop, std::min<uint32_t>(100, limit), start);
      for (auto &o : current) {
         std::stringstream ss;
         auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
         result.push_back(operation_detail{memo, ss.str(), o});
      }
      if (current.size() < std::min<uint32_t>(100, limit))
         break;
      limit -= current.size();
      start -= 100;
      if( start == 0 ) break;
   }
   return result;
}

vector<bucket_object> wallet_api::get_market_history( string symbol1, string symbol2, uint32_t bucket , fc::time_point_sec start, fc::time_point_sec end )const
{
   return my->_remote_hist->get_market_history( get_asset_id(symbol1), get_asset_id(symbol2), bucket, start, end );
}

vector<limit_order_object> wallet_api::get_limit_orders(string a, string b, uint32_t limit)const
{
   return my->_remote_db->get_limit_orders(get_asset(a).id, get_asset(b).id, limit);
}

vector<call_order_object> wallet_api::get_call_orders(string a, uint32_t limit)const
{
   return my->_remote_db->get_call_orders(get_asset(a).id, limit);
}

vector<force_settlement_object> wallet_api::get_settle_orders(string a, uint32_t limit)const
{
   return my->_remote_db->get_settle_orders(get_asset(a).id, limit);
}

brain_key_info wallet_api::suggest_brain_key()const
{
   brain_key_info result;
   // create a private key for secure entropy
   fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
   fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
   fc::bigint entropy1( sha_entropy1.data(), sha_entropy1.data_size() );
   fc::bigint entropy2( sha_entropy2.data(), sha_entropy2.data_size() );
   fc::bigint entropy(entropy1);
   entropy <<= 8*sha_entropy1.data_size();
   entropy += entropy2;
   string brain_key = "";

   for( int i=0; i<BRAIN_KEY_WORD_COUNT; i++ )
   {
      fc::bigint choice = entropy % graphene::words::word_list_size;
      entropy /= graphene::words::word_list_size;
      if( i > 0 )
         brain_key += " ";
      brain_key += graphene::words::word_list[ choice.to_int64() ];
   }

   brain_key = normalize_brain_key(brain_key);
   fc::ecc::private_key priv_key = derive_private_key( brain_key, 0 );
   result.brain_priv_key = brain_key;
   result.wif_priv_key = key_to_wif( priv_key );
   result.pub_key = priv_key.get_public_key();
   return result;
}

vector<brain_key_info> wallet_api::derive_owner_keys_from_brain_key(string brain_key, int number_of_desired_keys) const
{
   return graphene::wallet::utility::derive_owner_keys_from_brain_key(brain_key, number_of_desired_keys);
}

bool wallet_api::is_public_key_registered(string public_key) const
{
   bool is_known = my->_remote_db->is_public_key_registered(public_key);
   return is_known;
}


string wallet_api::serialize_transaction( signed_transaction tx )const
{
   return fc::to_hex(fc::raw::pack(tx));
}

variant wallet_api::get_object( object_id_type id ) const
{
   return my->_remote_db->get_objects({id});
}

string wallet_api::get_wallet_filename() const
{
   return my->get_wallet_filename();
}

transaction_handle_type wallet_api::begin_builder_transaction()
{
   return my->begin_builder_transaction();
}

void wallet_api::add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
{
   my->add_operation_to_builder_transaction(transaction_handle, op);
}

full_transaction wallet_api::proposal_block_address(const string& account, const fc::flat_set<address>& block_addr, int64_t expiration_time, bool broadcast /* = true */)
{
	return my->proposal_block_address(account,block_addr,expiration_time,broadcast);
}

full_transaction wallet_api::proposal_cancel_block_address(const string& account, const fc::flat_set<address>& block_addr, int64_t expiration_time, bool broadcast /* = true */)
{
	return my->proposal_cancel_block_address(account,block_addr,expiration_time,broadcast);
}
void wallet_api::replace_operation_in_builder_transaction(transaction_handle_type handle, unsigned operation_index, const operation& new_op)
{
   my->replace_operation_in_builder_transaction(handle, operation_index, new_op);
}

asset wallet_api::set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset)
{
   return my->set_fees_on_builder_transaction(handle, fee_asset);
}

transaction wallet_api::preview_builder_transaction(transaction_handle_type handle)
{
   return my->preview_builder_transaction(handle);
}

full_transaction wallet_api::sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast)
{
   return my->sign_builder_transaction(transaction_handle, broadcast);
}

full_transaction wallet_api::propose_builder_transaction(
   transaction_handle_type handle,
   time_point_sec expiration,
   uint32_t review_period_seconds,
   bool broadcast)
{
   return my->propose_builder_transaction(handle, expiration, review_period_seconds, broadcast);
}

full_transaction wallet_api::propose_builder_transaction2(
   transaction_handle_type handle,
   string account_name_or_id,
   time_point_sec expiration,
   uint32_t review_period_seconds,
   bool broadcast)
{
   return my->propose_builder_transaction2(handle, account_name_or_id, expiration, review_period_seconds, broadcast);
}
std::map<string,asset> wallet_api::get_address_pay_back_balance(const address& owner_addr, std::string asset_symbol) const {
	return my->get_address_pay_back_balance(owner_addr, asset_symbol);
}

std::map<string, share_type> wallet_api::get_bonus_balance(const address& owner) const
{
	return my->_remote_db->get_bonus_balances(owner);
}

full_transaction wallet_api::obtain_pay_back_balance(const string& pay_back_owner, std::map<std::string, asset> nums, bool broadcast) {
	return my->obtain_pay_back_balance(pay_back_owner,nums,broadcast);
}

full_transaction wallet_api::obtain_bonus_balance(const string& bonus_owner, std::map<std::string, share_type> nums, bool broadcast) {
	return my->obtain_bonus_balance(bonus_owner,nums,broadcast);
}

void wallet_api::remove_builder_transaction(transaction_handle_type handle)
{
   return my->remove_builder_transaction(handle);
}

fc::variant_object wallet_api::get_account(string account_name_or_id) const
{
   auto acc =  my->get_account(account_name_or_id);
   if (acc.addr == address())
	   return fc::variant_object();
   const auto& obj = get_account_by_addr(acc.addr);
   if (obj.valid())
   {
	   fc::mutable_variant_object m_obj = fc::variant(obj).as<fc::mutable_variant_object>();
	   m_obj.set("online",fc::variant("true"));
	   return m_obj;
   }
   fc::mutable_variant_object m_obj = fc::variant(acc).as<fc::mutable_variant_object>();
   m_obj.set("online", fc::variant("false"));
   return m_obj;
}

optional<account_object> wallet_api::get_account_by_addr(const address& addr) const
{
	return my->get_account_by_addr(addr);
}

account_object wallet_api::change_account_name(const string& oldname, const string& newname)
{
	return my->change_account_name(oldname, newname);
}
void wallet_api::remove_local_account(const string & account_name) {
	return my->remove_local_account(account_name);
}
asset_object wallet_api::get_asset(string asset_name_or_id) const
{
   auto a = my->find_asset(asset_name_or_id);
   FC_ASSERT(a);
   return *a;
}

fc::variant wallet_api::get_asset_imp(string asset_name_or_id) const
{
	auto asset_obj = get_asset(asset_name_or_id);
	auto dynamic_data = my->_remote_db->get_asset_dynamic_data(asset_name_or_id);
	if (!dynamic_data.valid())
	{
		return fc::variant(asset_obj);
	}
	fc::mutable_variant_object obj = fc::variant(asset_obj).as<fc::mutable_variant_object>();
	return obj.set("dynamic_data", fc::variant(*dynamic_data));
}


asset_bitasset_data_object wallet_api::get_bitasset_data(string asset_name_or_id) const
{
   auto asset = get_asset(asset_name_or_id);
   FC_ASSERT(asset.is_market_issued() && asset.bitasset_data_id);
   return my->get_object<asset_bitasset_data_object>(*asset.bitasset_data_id);
}

account_id_type wallet_api::get_account_id(string account_name_or_id) const
{
   return my->get_account_id(account_name_or_id);
}

address wallet_api::get_account_addr(string account_name) const
{
	return my->get_account_addr(account_name);
}

asset_id_type wallet_api::get_asset_id(string asset_symbol_or_id) const
{
   return my->get_asset_id(asset_symbol_or_id);
}

public_key_type wallet_api::get_pubkey_from_priv(const string& privkey)
{
	auto priv = wif_to_key(privkey);
	FC_ASSERT(priv.valid());
	return priv->get_public_key();
}

public_key_type wallet_api::get_pubkey_from_account(const string& acc)
{
	auto wif = dump_private_key(acc);
	FC_ASSERT(wif.size() >0 , "there is no private key in this wallet.");
	return get_pubkey_from_priv(wif.begin()->second);
}


signed_transaction wallet_api::decode_multisig_transaction(const string& trx)
{
	auto vec = fc::from_base58(trx);
	auto recovered = fc::json::from_string(string(vec.begin(), vec.end()));
	return recovered.as<signed_transaction>();
}

string wallet_api::sign_multisig_trx(const address& addr, const string& trx)
{
	auto vec = fc::from_base58(trx);
	auto recovered = fc::json::from_string(string(vec.begin(), vec.end()));
	return my->sign_multisig_trx(addr,recovered.as<signed_transaction>());
}

bool wallet_api::import_key(string account_name_or_id, string wif_key)
{
   FC_ASSERT(!is_locked());
   // backup wallet
   fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
   if (!optional_private_key)
      FC_THROW("Invalid private key");
   string shorthash = detail::address_to_shorthash(optional_private_key->get_public_key());
   copy_wallet_file( "before-import-key-" + shorthash );

   if( my->import_key(account_name_or_id, wif_key) )
   {
      save_wallet_file();
      copy_wallet_file( "after-import-key-" + shorthash );
      return true;
   }
   return false;
}

bool wallet_api::import_crosschain_key(string wif_key, string symbol)
{
	FC_ASSERT(!is_locked());
	string shorthash;
	if (symbol == "ETH" || symbol.find("ERC") != symbol.npos || symbol == "HC")
	{
		auto ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(symbol);
		FC_ASSERT(ptr != nullptr, "plugin doesnt exist.");
		ptr->import_private_key(wif_key);
		FC_ASSERT(ptr->get_private_key() != fc::ecc::private_key());
		auto addr = ptr->get_address();
		shorthash = addr;
	}
	else {
		fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
		if (!optional_private_key)
			FC_THROW("Invalid private key");
		shorthash = detail::address_to_shorthash(optional_private_key->get_public_key());
	}
	// backup wallet	
	copy_wallet_file("before-import-key-" + shorthash);
	if (my->import_crosschain_key(wif_key, symbol))
	{
		save_wallet_file();
		copy_wallet_file("after-import-key-" + shorthash);
		return true;
	}
	return false;
}



map<string, bool> wallet_api::import_accounts( string filename, string password )
{
   FC_ASSERT( !is_locked() );
   FC_ASSERT( fc::exists( filename ) );

   const auto imported_keys = fc::json::from_file<exported_keys>( filename );

   const auto password_hash = fc::sha512::hash( password );
   FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

   map<string, bool> result;
   for( const auto& item : imported_keys.account_keys )
   {
       const auto import_this_account = [ & ]() -> bool
       {
           try
           {
               const account_object account = my->get_account( item.account_name );
               const auto& owner_keys = account.owner.get_keys();
               const auto& active_keys = account.active.get_keys();

               for( const auto& public_key : item.public_keys )
               {
                   if( std::find( owner_keys.begin(), owner_keys.end(), public_key ) != owner_keys.end() )
                       return true;

                   if( std::find( active_keys.begin(), active_keys.end(), public_key ) != active_keys.end() )
                       return true;
               }
           }
           catch( ... )
           {
           }

           return false;
       };

       const auto should_proceed = import_this_account();
       result[ item.account_name ] = should_proceed;

       if( should_proceed )
       {
           uint32_t import_successes = 0;
           uint32_t import_failures = 0;
           // TODO: First check that all private keys match public keys
           for( const auto& encrypted_key : item.encrypted_private_keys )
           {
               try
               {
                  const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
                  const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

                  import_key( item.account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
                  ++import_successes;
               }
               catch( const fc::exception& e )
               {
                  elog( "Couldn't import key due to exception ${e}", ("e", e.to_detail_string()) );
                  ++import_failures;
               }
           }
           ilog( "successfully imported ${n} keys for account ${name}", ("n", import_successes)("name", item.account_name) );
           if( import_failures > 0 )
              elog( "failed to import ${n} keys for account ${name}", ("n", import_failures)("name", item.account_name) );
       }
   }

   return result;
}

bool wallet_api::import_account_keys( string filename, string password, string src_account_name, string dest_account_name )
{
   FC_ASSERT( !is_locked() );
   FC_ASSERT( fc::exists( filename ) );

   bool is_my_account = false;
   const auto accounts = list_my_accounts();
   for( const auto& account : accounts )
   {
       if( account.name == dest_account_name )
       {
           is_my_account = true;
           break;
       }
   }
   FC_ASSERT( is_my_account );

   const auto imported_keys = fc::json::from_file<exported_keys>( filename );

   const auto password_hash = fc::sha512::hash( password );
   FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

   bool found_account = false;
   for( const auto& item : imported_keys.account_keys )
   {
       if( item.account_name != src_account_name )
           continue;

       found_account = true;

       for( const auto& encrypted_key : item.encrypted_private_keys )
       {
           const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
           const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

           my->import_key( dest_account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
       }

       return true;
   }
   save_wallet_file();

   FC_ASSERT( found_account );

   return false;
}
void wallet_api::change_acquire_plugin_num(const string&symbol, const uint32_t& blocknum) {
	my->change_acquire_plugin_num(symbol, blocknum);
}
string wallet_api::normalize_brain_key(string s) const
{
   return detail::normalize_brain_key( s );
}

variant wallet_api::info()
{
   return my->info();
}

variant_object wallet_api::about() const
{
    return my->about();
}

fc::ecc::private_key wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
{
   return detail::derive_private_key( prefix_string, sequence_number );
}
/*
signed_transaction wallet_api::register_account(string name,
public_key_type owner_pubkey,
public_key_type active_pubkey,
string  registrar_account,
string  referrer_account,
uint32_t referrer_percent,
bool broadcast)
{
return my->register_account( name, owner_pubkey, active_pubkey, registrar_account, referrer_account, referrer_percent, broadcast );
}
*/


full_transaction wallet_api::register_account(string name, bool broadcast)
{
	return my->register_account(name, broadcast);
}




//full_transaction wallet_api::create_account_with_brain_key(string brain_key, string account_name,
//                                                             string registrar_account, string referrer_account,
//                                                             bool broadcast /* = false */)
//{
//   return my->create_account_with_brain_key(
//            brain_key, account_name, registrar_account,
//            referrer_account, broadcast
//            );
//}

address wallet_api::wallet_create_account_with_brain_key(const string& name)
{
	return my->create_account(name,true);
}

graphene::chain::map<std::string, int> wallet_api::list_address_indexes(string& password)
{
	return graphene::chain::map<std::string, int>();
	//FC_ASSERT(!is_locked());
    //auto pw = fc::sha512::hash(password.c_str(), password.size());
    //FC_ASSERT(_wallet.checksum == pw,"password is not correct");
	//return my->_wallet.used_indexes;
}

std::string wallet_api::derive_wif_key(const string& brain_key, int index, const string& symbol)
{
	return my->derive_wif_key(brain_key, index, symbol);
}

address wallet_api::wallet_create_account(string account_name)
{
	return my->create_account(account_name);
}

full_transaction wallet_api::set_citizen_pledge_pay_back_rate(const string& citizen, int pledge_pay_back_rate, bool broadcast)
{
	return my->set_citizen_pledge_pay_back_rate(citizen, pledge_pay_back_rate, broadcast);
}
full_transaction wallet_api::issue_asset(string to_account, string amount, string symbol,
                                           string memo, bool broadcast)
{
   return my->issue_asset(to_account, amount, symbol, memo, broadcast);
}

full_transaction wallet_api::transfer(string from, string to, string amount,
                                        string asset_symbol, string memo, bool broadcast /* = false */)
{
   return my->transfer(from, to, amount, asset_symbol, memo, broadcast);
}

full_transaction wallet_api::transfer_to_address(string from, string to, string amount,
	string asset_symbol, string memo, bool broadcast /* = false */)
{
	return my->transfer_to_address(from, to, amount, asset_symbol, memo, broadcast);
}

full_transaction wallet_api::combine_transaction(const vector<string>& trxs, bool broadcast)
{
	vector<signed_transaction> vecs;
	for (const auto& trx : trxs)
	{
		auto vec = fc::from_base58(trx);
		auto recovered = fc::json::from_string(string(vec.begin(), vec.end()));
		vecs.emplace_back(recovered.as<signed_transaction>());
	}
	return my->combine_transaction(vecs,broadcast);
}


string wallet_api::lightwallet_broadcast(signed_transaction trx)
{
    return my->lightwallet_broadcast(trx);
}


string wallet_api::lightwallet_get_refblock_info()
{
    return my->lightwallet_get_refblock_info();
}

string wallet_api::transfer_from_to_address(string from, string to, string amount,
	string asset_symbol, string memo)
{
	return my->transfer_from_to_address(from,to,amount,asset_symbol,memo);

}

full_transaction wallet_api::transfer_to_account(string from, string to, string amount,
	string asset_symbol, string memo, bool broadcast /* = false */)
{
	const auto acc = my->get_account(to);
	FC_ASSERT(address() != acc.addr,"account should be in the chain.");
	return my->transfer_to_address(from, string(acc.addr), amount, asset_symbol, memo, broadcast);
}
std::vector<crosschain_trx_object> wallet_api::get_account_crosschain_transaction(string account_address, string trx_id) {
	return my->get_account_crosschain_transaction(account_address, trx_id);
}
std::map<transaction_id_type, signed_transaction> wallet_api::get_coldhot_transaction(const int& type) {
	return my->get_coldhot_transaction(type);
}
std::map<transaction_id_type, signed_transaction> wallet_api::get_coldhot_transaction_by_blocknum(const string& symbol,
	const uint32_t& start_block_num,
	const uint32_t& stop_block_num,
	int crosschain_trx_state) {
	return my->get_coldhot_transaction_by_blocknum(symbol, start_block_num, stop_block_num, crosschain_trx_state);
}
std::map<transaction_id_type, signed_transaction> wallet_api::get_crosschain_transaction(int type) {
	return my->get_crosschain_transaction(type);
}
std::map<transaction_id_type, signed_transaction> wallet_api::get_crosschain_transaction_by_block_num(const string& symbol, 
	const uint32_t& start_block_num,
	const uint32_t& stop_block_num,
	int crosschain_trx_state) {
	std::string account = "";
	return my->get_crosschain_transaction_by_block_num(symbol, account, start_block_num, stop_block_num, crosschain_trx_state);
}
std::map<transaction_id_type, signed_transaction> wallet_api::get_withdraw_crosschain_without_sign_transaction(){
	
	return my->get_withdraw_crosschain_without_sign_transaction();
}
void wallet_api::senator_sign_coldhot_transaction(const string& tx_id, const string& senator, const string& keyfile, const string& decryptkey) {
	return my->guard_sign_coldhot_transaction(tx_id, senator, keyfile,decryptkey);
}

void wallet_api::senator_sign_eths_multi_account_create_trx(const string& tx_id, const string& senator, const string& keyfile, const string& decryptkey) {
	my->senator_sign_eths_multi_account_create_trx(tx_id, senator,keyfile,decryptkey);
}
void wallet_api::senator_sign_eths_final_trx(const string& trx_id, const string & senator) {
	my->senator_sign_eths_final_trx(trx_id, senator);
}
void wallet_api::senator_changer_eth_singer_trx(const string guard, const string txid, const string& newaddress, const int64_t& expiration_time, bool broadcast) {
	my->senator_changer_eth_singer_trx(guard, txid, newaddress, expiration_time,broadcast);
}
void wallet_api::senator_changer_eth_coldhot_singer_trx(const string guard, const string txid, const string& newaddress, const int64_t& expiration_time, const string& keyfile, const string& decryptkey, bool broadcast) {
	my->senator_changer_eth_coldhot_singer_trx(guard, txid, newaddress, expiration_time, keyfile, decryptkey, broadcast);
}

void wallet_api::senator_sign_eths_coldhot_final_trx(const string& trx_id, const string & senator, const string& keyfile, const string& decryptkey) {
	my->senator_sign_eths_coldhot_final_trx(trx_id, senator,keyfile,decryptkey);
}
void wallet_api::senator_sign_crosschain_transaction(const string& trx_id,const string& guard){
	return my->guard_sign_crosschain_transaction(trx_id,guard);
}
optional<multisig_address_object> wallet_api::get_current_multi_address_obj(const string& symbol, const account_id_type& guard) const
{
	return my->get_current_multi_address_obj(symbol,guard);
}
std::vector<lockbalance_object> wallet_api::get_account_lock_balance(const string& account)const {
	return my->get_account_lock_balance(account);
}
full_transaction wallet_api::lock_balance_to_citizen(string miner_account,
	string lock_account,
	string amount,
	string asset_symbol,
	bool broadcast/* = false*/) {
	return my->lock_balance_to_miner(miner_account, lock_account,amount, asset_symbol, broadcast);
}

full_transaction wallet_api::lock_balance_to_citizens(string lock_account, map<string, vector<asset>> lockbalances, bool broadcast /* = false */)
{
	return my->lock_balance_to_miners(lock_account, lockbalances,broadcast);
}

full_transaction wallet_api::withdraw_cross_chain_transaction(string account_name,
	string amount,
	string asset_symbol,
	string crosschain_account,
	string memo,
	bool broadcast/* = false*/) {
	return my->withdraw_cross_chain_transaction(account_name, amount, asset_symbol, crosschain_account, memo, broadcast);
}
full_transaction wallet_api::transfer_senator_multi_account(string multi_account,
	string amount,
	string asset_symbol,
	string multi_to_account,
	string memo,
	bool broadcast/* = false*/){
	return my->transfer_guard_multi_account(multi_account, amount, asset_symbol, multi_to_account,memo, broadcast);
}
full_transaction wallet_api::senator_lock_balance(string guard_account,
	string amount,
	string asset_symbol,
	bool broadcast/* = false*/) {
	return my->guard_lock_balance(guard_account, amount, asset_symbol, broadcast);
}
full_transaction wallet_api::foreclose_balance_from_citizen(string miner_account,
	string foreclose_account,
	string amount,
	string asset_symbol,
	bool broadcast/* = false*/) {
	return my->foreclose_balance_from_miner(miner_account, foreclose_account, amount, asset_symbol, broadcast);
}

full_transaction wallet_api::foreclose_balance_from_citizens(string foreclose_account, map<string, vector<asset>> foreclose_balances, bool broadcast /* = false */)
{
	return my->foreclose_balance_from_miners(foreclose_account,foreclose_balances,broadcast);
}

full_transaction wallet_api::senator_foreclose_balance(string guard_account,
	string amount,
	string asset_symbol,
	bool broadcast/* = false*/) {
	return my->guard_foreclose_balance(guard_account, amount, asset_symbol, broadcast);
}

full_transaction wallet_api::create_asset(string issuer,
                                            string symbol,
                                            uint8_t precision,
                                            asset_options common,
                                            fc::optional<bitasset_options> bitasset_opts,
                                            bool broadcast)

{
   return my->create_asset(issuer, symbol, precision, common, bitasset_opts, broadcast);
}


full_transaction wallet_api::wallet_create_asset(string issuer,
	string symbol,
	uint8_t precision,
	share_type max_supply,
	share_type core_fee_paid,
	bool broadcast)

{
	return my->wallet_create_asset(issuer, symbol, precision,max_supply, core_fee_paid, broadcast);
}
full_transaction wallet_api::wallet_create_erc_asset(string issuer,
	string symbol,
	uint8_t precision,
	share_type max_supply,
	share_type core_fee_paid,
	std::string erc_address,
	bool broadcast) {
	return my->wallet_create_erc_asset(issuer, symbol, precision, max_supply, core_fee_paid, erc_address, broadcast);

}
full_transaction wallet_api::update_asset(const string& account, const std::string& symbol,
	const std::string& description,
	bool broadcast)
{
   return my->update_asset(account,symbol, description, broadcast);
}

full_transaction wallet_api::update_bitasset(string symbol,
                                               bitasset_options new_options,
                                               bool broadcast /* = false */)
{
   return my->update_bitasset(symbol, new_options, broadcast);
}

full_transaction wallet_api::update_asset_feed_producers(string symbol,
                                                           flat_set<string> new_feed_producers,
                                                           bool broadcast /* = false */)
{
   return my->update_asset_feed_producers(symbol, new_feed_producers, broadcast);
}

full_transaction wallet_api::publish_asset_feed(string publishing_account,
                                                  string symbol,
                                                  price_feed feed,
                                                  bool broadcast /* = false */)
{
   return my->publish_asset_feed(publishing_account, symbol, feed, broadcast);
}
full_transaction wallet_api::publish_normal_asset_feed(string publishing_account,
	string symbol,
	price_feed feed,
	bool broadcast /* = false */)
{
	return my->publish_normal_asset_feed(publishing_account, symbol, feed, broadcast);
}

full_transaction wallet_api::fund_asset_fee_pool(string from,
                                                   string symbol,
                                                   string amount,
                                                   bool broadcast /* = false */)
{
   return my->fund_asset_fee_pool(from, symbol, amount, broadcast);
}

full_transaction wallet_api::reserve_asset(string from,
                                          string amount,
                                          string symbol,
                                          bool broadcast /* = false */)
{
   return my->reserve_asset(from, amount, symbol, broadcast);
}

full_transaction wallet_api::global_settle_asset(string symbol,
                                                   price settle_price,
                                                   bool broadcast /* = false */)
{
   return my->global_settle_asset(symbol, settle_price, broadcast);
}

full_transaction wallet_api::settle_asset(string account_to_settle,
                                            string amount_to_settle,
                                            string symbol,
                                            bool broadcast /* = false */)
{
   return my->settle_asset(account_to_settle, amount_to_settle, symbol, broadcast);
}

full_transaction wallet_api::whitelist_account(string authorizing_account,
                                                 string account_to_list,
                                                 account_whitelist_operation::account_listing new_listing_status,
                                                 bool broadcast /* = false */)
{
   return my->whitelist_account(authorizing_account, account_to_list, new_listing_status, broadcast);
}

full_transaction wallet_api::create_senator_member(string account, bool broadcast /* = false */)
{
   return my->create_guard_member( account, broadcast);
}


full_transaction wallet_api::update_senator_formal(string proposing_account, map<account_id_type, account_id_type> replace_queue,int64_t expiration_time,
	bool broadcast /* = false */)
{
	return my->update_guard_formal(proposing_account, replace_queue, expiration_time,broadcast);
}

full_transaction wallet_api::citizen_referendum_for_senator(const string& citizen,const string& amount ,const map<account_id_type, account_id_type>& replacement,bool broadcast /* = true */)
{
	return my->citizen_referendum_for_senator(citizen,amount,replacement,broadcast);
}

full_transaction wallet_api::referendum_accelerate_pledge(const referendum_id_type referendum_id, const string& amount, bool broadcast /* = true */)
{
	return my->referendum_accelerate_pledge(referendum_id,amount,broadcast);
}

full_transaction wallet_api::resign_senator_member(string proposing_account, string account,
    int64_t expiration_time, bool broadcast)
{
    return my->resign_guard_member(proposing_account, account, expiration_time, broadcast);
}

map<string,miner_id_type> wallet_api::list_citizens(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_miner_accounts(lowerbound, limit);
}

map<string,guard_member_id_type> wallet_api::list_senator_members(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_guard_member_accounts(lowerbound, limit,false);
}

map<string, guard_member_id_type> wallet_api::list_all_senators(const string& lowerbound, uint32_t limit)
{
	return my->_remote_db->lookup_guard_member_accounts(lowerbound, limit,true);
}


miner_object wallet_api::get_citizen(string owner_account)
{
   return my->get_miner(owner_account);
}

guard_member_object wallet_api::get_senator_member(string owner_account)
{
   return my->get_guard_member(owner_account);
}

flat_set<miner_id_type> wallet_api::list_active_citizens()
{
	return my->list_active_citizens();
}


full_transaction wallet_api::create_citizen(string owner_account,
                                              string url,
                                              bool broadcast /* = false */)
{
   return my->create_miner(owner_account, url, broadcast);
}

full_transaction wallet_api::create_worker(
   string owner_account,
   time_point_sec work_begin_date,
   time_point_sec work_end_date,
   share_type daily_pay,
   string name,
   string url,
   variant worker_settings,
   bool broadcast /* = false */)
{
   return my->create_worker( owner_account, work_begin_date, work_end_date,
      daily_pay, name, url, worker_settings, broadcast );
}

full_transaction wallet_api::update_worker_votes(
   string owner_account,
   worker_vote_delta delta,
   bool broadcast /* = false */)
{
   return my->update_worker_votes( owner_account, delta, broadcast );
}

full_transaction wallet_api::update_witness(
   string witness_name,
   string url,
   string block_signing_key,
   bool broadcast /* = false */)
{
   return my->update_witness(witness_name, url, block_signing_key, broadcast);
}

vector< vesting_balance_object_with_info > wallet_api::get_vesting_balances( string account_name )
{
   return my->get_vesting_balances( account_name );
}

full_transaction wallet_api::withdraw_vesting(
   string witness_name,
   string amount,
   string asset_symbol,
   bool broadcast /* = false */)
{
   return my->withdraw_vesting( witness_name, amount, asset_symbol, broadcast );
}

full_transaction wallet_api::vote_for_committee_member(string voting_account,
                                                 string witness,
                                                 bool approve,
                                                 bool broadcast /* = false */)
{
   return my->vote_for_committee_member(voting_account, witness, approve, broadcast);
}

full_transaction wallet_api::vote_for_witness(string voting_account,
                                                string witness,
                                                bool approve,
                                                bool broadcast /* = false */)
{
   return my->vote_for_witness(voting_account, witness, approve, broadcast);
}

full_transaction wallet_api::set_voting_proxy(string account_to_modify,
                                                optional<string> voting_account,
                                                bool broadcast /* = false */)
{
   return my->set_voting_proxy(account_to_modify, voting_account, broadcast);
}

full_transaction wallet_api::set_desired_citizen_and_senator_member_count(string account_to_modify,
                                                                      uint16_t desired_number_of_witnesses,
                                                                      uint16_t desired_number_of_committee_members,
                                                                      bool broadcast /* = false */)
{
   return my->set_desired_miner_and_guard_member_count(account_to_modify, desired_number_of_witnesses,
                                                     desired_number_of_committee_members, broadcast);
}

void wallet_api::set_wallet_filename(string wallet_filename)
{
   my->_wallet_filename = wallet_filename;
}

signed_transaction wallet_api::sign_transaction(signed_transaction tx, bool broadcast /* = false */)
{ try {
   return my->sign_transaction( tx, broadcast);
} FC_CAPTURE_AND_RETHROW( (tx) ) }

operation wallet_api::get_prototype_operation(string operation_name)
{
   return my->get_prototype_operation( operation_name );
}
/*
void wallet_api::dbg_make_uia(string creator, string symbol)
{
   FC_ASSERT(!is_locked());
   my->dbg_make_uia(creator, symbol);
}

void wallet_api::dbg_make_mia(string creator, string symbol)
{
   FC_ASSERT(!is_locked());
   my->dbg_make_mia(creator, symbol);
}
*/
void wallet_api::dbg_push_blocks( std::string src_filename, uint32_t count )
{
   my->dbg_push_blocks( src_filename, count );
}

void wallet_api::dbg_generate_blocks( std::string debug_wif_key, uint32_t count )
{
   my->dbg_generate_blocks( debug_wif_key, count );
}

void wallet_api::dbg_stream_json_objects( const std::string& filename )
{
   my->dbg_stream_json_objects( filename );
}

void wallet_api::dbg_update_object( fc::variant_object update )
{
   my->dbg_update_object( update );
}

void wallet_api::network_add_nodes( const vector<string>& nodes )
{
   my->network_add_nodes( nodes );
}

vector< variant > wallet_api::network_get_connected_peers()
{
   return my->network_get_connected_peers();
}

fc::variant_object wallet_api::network_get_info()
{
	return my->network_get_info();
}

void wallet_api::flood_network(string prefix, uint32_t number_of_transactions)
{
   FC_ASSERT(!is_locked());
   my->flood_network(prefix, number_of_transactions);
}

full_transaction wallet_api::propose_parameter_change(
   const string& proposing_account,
   const variant_object& changed_values,
   const int64_t& expiration_time,
   bool broadcast /* = false */
   )
{
   return my->propose_parameter_change( proposing_account, expiration_time, changed_values, broadcast );
}

full_transaction wallet_api::propose_coin_destory(
	const string& proposing_account,
	fc::time_point_sec expiration_time,
	const variant_object& destory_values,
	bool broadcast)
{
	return my->propose_coin_destory(proposing_account, expiration_time, destory_values, broadcast);
}

full_transaction wallet_api::propose_senator_pledge_change(
	const string& proposing_account,
	fc::time_point_sec expiration_time,
	const variant_object& changed_values,
	bool broadcast )
{
	return my->propose_guard_pledge_change(proposing_account, expiration_time, changed_values, broadcast);

}
full_transaction wallet_api::propose_pay_back_asset_rate_change(
	const string& proposing_account,
	const variant_object& changed_values,
	const int64_t& expiration_time,
	bool broadcast
) {
	return my->propose_pay_back_asset_rate_change(proposing_account, expiration_time, changed_values, broadcast);
}



full_transaction wallet_api::propose_fee_change(
   const string& proposing_account,
   const variant_object& changed_fees,
   int64_t expiration_time,
   bool broadcast /* = false */
   )
{
   return my->propose_fee_change( proposing_account, expiration_time, changed_fees, broadcast );
}

full_transaction wallet_api::approve_proposal(
   const string& fee_paying_account,
   const string& proposal_id,
   const approval_delta& delta,
   bool broadcast /* = false */
   )
{
   return my->approve_proposal( fee_paying_account, proposal_id, delta, broadcast );
}

full_transaction wallet_api::approve_referendum(const string& fee_paying_account, const string& referendum_id, const approval_delta& delta, bool broadcast)
{
	return my->approve_referendum(fee_paying_account, referendum_id, delta, broadcast);
}


full_transaction wallet_api::register_contract(const string& caller_account_name, const string& gas_price, const string& gas_limit, const string& contract_filepath)
{
	return my->register_contract(caller_account_name, gas_price, gas_limit, contract_filepath);
}

full_transaction wallet_api::register_contract_like(const string & caller_account_name, const string & gas_price, const string & gas_limit, const string & base)
{
    return my->register_contract_like(caller_account_name,gas_price,gas_limit,base);
}

std::pair<asset, share_type> wallet_api::register_contract_testing(const string & caller_account_name, const string & contract_filepath)
{
    return my->register_contract_testing(caller_account_name, contract_filepath);
}

std::string wallet_api::register_native_contract(const string& caller_account_name, const string& gas_price, const string& gas_limit, const string& native_contract_key)
{
	return my->register_native_contract(caller_account_name, gas_price, gas_limit, native_contract_key);
}

std::pair<asset, share_type> wallet_api::register_native_contract_testing(const string & caller_account_name, const string & native_contract_key)
{
    return my->register_native_contract_testing(caller_account_name, native_contract_key);
}

full_transaction wallet_api::invoke_contract(const string& caller_account_name, const string& gas_price, const string& gas_limit, const string& contract_address_or_name, const string& contract_api, const string& contract_arg)
{
	std::string contract_address;
	contract_object cont;
	bool is_valid_address = true;
	try {
		contract_address = graphene::chain::address(contract_address_or_name).address_to_string();
		auto temp = address(contract_address);
		FC_ASSERT(temp.version == addressVersion::CONTRACT);
	}
	catch (fc::exception& e)
	{
		is_valid_address = false;
	}
	if (!is_valid_address)
	{
		cont = my->_remote_db->get_contract_object_by_name(contract_address_or_name);
		contract_address = string(cont.contract_address);
	}
	return my->invoke_contract(caller_account_name, gas_price, gas_limit, contract_address, contract_api, contract_arg);
}

std::pair<asset, share_type> wallet_api::invoke_contract_testing(const string & caller_account_name, const string & contract_address_or_name, const string & contract_api, const string & contract_arg)
{
	std::string contract_address;
	contract_object cont;
	bool is_valid_address = true;
	try {
		contract_address = graphene::chain::address(contract_address_or_name).address_to_string();
		auto temp = address(contract_address);
		FC_ASSERT(temp.version == addressVersion::CONTRACT);
	}
	catch (fc::exception& e)
	{
		is_valid_address = false;
	}
	if (!is_valid_address)
	{
		cont = my->_remote_db->get_contract_object_by_name(contract_address_or_name);
		contract_address = string(cont.contract_address);
	}
    return my->invoke_contract_testing(caller_account_name, contract_address, contract_api, contract_arg);

}

string wallet_api::invoke_contract_offline(const string& caller_account_name, const string& contract_address_or_name, const string& contract_api, const string& contract_arg)
{
	std::string contract_address;
	contract_object cont;
	bool is_valid_address = true;
	try {
		contract_address = graphene::chain::address(contract_address_or_name).address_to_string();
		auto temp = address(contract_address);
		FC_ASSERT(temp.version == addressVersion::CONTRACT);
	}
	catch (fc::exception& e)
	{
		is_valid_address = false;
	}
	if (!is_valid_address)
	{
		cont = my->_remote_db->get_contract_object_by_name(contract_address_or_name);
		contract_address = string(cont.contract_address);
	}
	return my->invoke_contract_offline(caller_account_name, contract_address, contract_api, contract_arg);
}

full_transaction wallet_api::upgrade_contract(const string& caller_account_name, const string& gas_price, const string& gas_limit, const string& contract_address, const string& contract_name, const string& contract_desc)
{
	return my->upgrade_contract(caller_account_name, gas_price, gas_limit, contract_address, contract_name, contract_desc);
}

fc::variant_object wallet_api::decoderawtransaction(const string& raw_trx, const string& symbol)
{
	return my->decoderawtransaction(raw_trx,symbol);
}

fc::variant_object wallet_api::createrawtransaction(const string& from, const string& to, const string& amount, const string& symbol)
{
	return my->createrawtransaction(from,to,amount, symbol);
}
string wallet_api::signrawtransaction(const string& from,const string& symbol ,const fc::variant_object& trx, bool broadcast)
{
	return my->signrawtransaction(from,symbol,trx,broadcast);
}

std::pair<asset, share_type> wallet_api::upgrade_contract_testing(const string & caller_account_name, const string & contract_address, const string & contract_name, const string & contract_desc)
{
    return  my->upgrade_contract_testing(caller_account_name, contract_address, contract_name, contract_desc);
}

ContractEntryPrintable wallet_api::get_contract_info(const string & contract_address_or_name) const
{
	std::string contract_address;
	try {
		auto temp = graphene::chain::address(contract_address_or_name);
		FC_ASSERT(temp.version == addressVersion::CONTRACT);
		contract_address = temp.operator fc::string();
	}
	catch (fc::exception& e)
	{
		auto cont = my->_remote_db->get_contract_object_by_name(contract_address_or_name);
		contract_address = string(cont.contract_address);
	}
    return   my->_remote_db->get_contract_info(contract_address);
}

ContractEntryPrintable wallet_api::get_simple_contract_info(const string & contract_address_or_name) const
{
	std::string contract_address;
	try {
		auto temp = graphene::chain::address(contract_address_or_name);
		FC_ASSERT(temp.version == addressVersion::CONTRACT);
        contract_address = temp.operator fc::string();
	}
	catch (fc::exception& e)
	{
		auto cont = my->_remote_db->get_contract_object_by_name(contract_address_or_name);
		contract_address = string(cont.contract_address);
	}
	ContractEntryPrintable result = my->_remote_db->get_contract_object(contract_address);
	result.code_printable.printable_code = "";
	return result;
}

full_transaction wallet_api::transfer_to_contract(string from, string to, string amount, string asset_symbol, const string& param, const string& gas_price, const string& gas_limit, bool broadcast)
{
    return my->transfer_to_contract(from, to,amount, asset_symbol, param, gas_price, gas_limit,broadcast);
}

std::pair<asset, share_type> wallet_api::transfer_to_contract_testing(string from, string to, string amount, string asset_symbol, const string& param)
{
    return my->transfer_to_contract_testing(from,to,amount,asset_symbol, param);
}

vector<asset> wallet_api::get_contract_balance(const string & contract_address) const
{
    return my->_remote_db->get_contract_balance(address(contract_address));
}
vector<string> wallet_api::get_contract_addresses_by_owner(const std::string& addr)
{
    address owner_addr;
    if(address::is_valid(addr))
    {
        owner_addr = address(addr);
    }else
    {
        auto acct = my->get_account(addr);
        owner_addr = acct.addr;
    }
    auto addr_res= my->_remote_db->get_contract_addresses_by_owner_address(owner_addr);
    vector<string> res;
    for(auto& out: addr_res)
    {
        res.push_back(out.operator fc::string());
    }
    return res;
}
vector<ContractEntryPrintable> wallet_api::get_contracts_by_owner(const std::string& addr)
{
    vector<ContractEntryPrintable> res;
    address owner_addr;
    if (address::is_valid(addr))
    {
        owner_addr = address(addr);
    }
    else
    {
        auto acct = my->get_account(addr);
        owner_addr = acct.addr;
    }
    auto objs= my->_remote_db->get_contract_objs_by_owner(owner_addr);
    for(auto& obj:objs)
    {
        res.push_back(obj);
    }
    return res;
}

vector<contract_hash_entry> wallet_api::get_contracts_hash_entry_by_owner(const std::string& addr)
{
    address owner_addr;
    if (address::is_valid(addr))
    {
        owner_addr = address(addr);
    }
    else
    {
        auto acct = my->get_account(addr);
        owner_addr = acct.addr;
    }
    auto contracts= my->_remote_db->get_contract_objs_by_owner(owner_addr);
    vector<contract_hash_entry> res;
    for(auto& co:contracts)
    {
        res.push_back(co);
    }
    return res;
}

vector<contract_event_notify_object> wallet_api::get_contract_events(const std::string&addr)
{
    return my->_remote_db->get_contract_events(address(addr));
}

graphene::chain::vector<graphene::chain::contract_event_notify_object> wallet_api::get_contract_events_in_range(const std::string&addr, uint64_t start, uint64_t range) const
{
	return my->_remote_db->get_contract_events_in_range(address(addr),start,range);
}
vector<transaction_id_type> wallet_api::get_contract_history(const string& contract_id, uint64_t start , uint64_t end )
{
	return my->_remote_db->get_contract_history(contract_id,start,end);
}
vector<contract_blocknum_pair> wallet_api::get_contract_storage_changed(const uint32_t block_num)
{
    return  my->_remote_db->get_contract_storage_changed(block_num);
}

graphene::chain::full_transaction wallet_api::create_contract_transfer_fee_proposal(const string& proposer, share_type fee_rate, int64_t expiration_time, bool broadcast)
{
	return my->create_contract_transfer_fee_proposal(proposer, fee_rate, expiration_time,broadcast);
}

vector<contract_blocknum_pair> wallet_api::get_contract_registered(const uint32_t block_num)
{
    return my->_remote_db->get_contract_registered(block_num);
}
vector<graphene::chain::contract_invoke_result_object> wallet_api::get_contract_invoke_object(const std::string&trx_id)
{
    return my->_remote_db->get_contract_invoke_object(trx_id);
}
std::string wallet_api::add_script(const string& script_path) 
{
    script_object spt;
    std::ifstream in(script_path, std::ios::in | std::ios::binary);
    FC_ASSERT(in.is_open());
    std::vector<unsigned char> contract_filedata((std::istreambuf_iterator<char>(in)),
        (std::istreambuf_iterator<char>()));
    in.close();
    auto contract_code = ContractHelper::load_contract_from_file(script_path);
    spt.script = contract_code;
    spt.script_hash = spt.script.GetHash();
    my->_wallet.insert_script(spt);
    save_wallet_file();
    return spt.script_hash;
}
vector<script_object> wallet_api::list_scripts()
{
    return my->_wallet.list_scripts();
}
void wallet_api::remove_script(const string& script_hash)
{
    my->_wallet.remove_script(script_hash);

    save_wallet_file();
}
bool wallet_api::bind_script_to_event(const string& script_hash, const string& contract, const string& event_name)
{
    auto con_info=my->_remote_db->get_contract_object(contract);
    FC_ASSERT(con_info.contract_address == address(contract), "");
    bool res = my->_wallet.bind_script_to_event(script_hash, address(contract), event_name);

    save_wallet_file();
    return res;
}
bool wallet_api::remove_event_handle(const string& script_hash, const string& contract, const string& event_name)
{
    auto con_info = my->_remote_db->get_contract_object(contract);
    FC_ASSERT(con_info.contract_address == address(contract), "");
    bool res = my->_wallet.remove_event_handle(script_hash, address(contract), event_name);
    save_wallet_file();
    return res;
}
vector<proposal_object>  wallet_api::get_proposal(const string& proposer)
{
	return my->get_proposal(proposer);
}
vector<proposal_object>  wallet_api::get_proposal_for_voter(const string& voter)
{
	return my->get_proposal_for_voter(voter);
}
vector<referendum_object> wallet_api::get_referendum_for_voter(const string& voter)
{
	return my->get_referendum_for_voter(voter);
}

global_property_object wallet_api::get_global_properties() const
{
   return my->get_global_properties();
}

dynamic_global_property_object wallet_api::get_dynamic_global_properties() const
{
   return my->get_dynamic_global_properties();
}

string wallet_api::help()const
{
   std::vector<std::string> method_names = my->method_documentation.get_method_names();
   std::stringstream ss;
   for (const std::string method_name : method_names)
   {
      try
      {
         ss << my->method_documentation.get_brief_description(method_name);
      }
      catch (const fc::key_not_found_exception&)
      {
         ss << method_name << " (no help available)\n";
      }
   }
   return ss.str();
}

string wallet_api::gethelp(const string& method)const
{
   fc::api<wallet_api> tmp;
   std::stringstream ss;
   ss << "\n";

   if( method == "import_key" )
   {
      ss << "usage: import_key ACCOUNT_NAME_OR_ID  WIF_PRIVATE_KEY\n\n";
      ss << "example: import_key \"1.3.11\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
      ss << "example: import_key \"usera\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
   }
   else if( method == "transfer" )
   {
      ss << "usage: transfer FROM TO AMOUNT SYMBOL \"memo\" BROADCAST\n\n";
      ss << "example: transfer \"1.3.11\" \"1.3.4\" 1000.03 CORE \"memo\" true\n";
      ss << "example: transfer \"usera\" \"userb\" 1000.123 CORE \"memo\" true\n";
   }
   else if( method == "create_account_with_brain_key" )
   {
      ss << "usage: create_account_with_brain_key BRAIN_KEY ACCOUNT_NAME REGISTRAR REFERRER BROADCAST\n\n";
      ss << "example: create_account_with_brain_key \"my really long brain key\" \"newaccount\" \"1.3.11\" \"1.3.11\" true\n";
      ss << "example: create_account_with_brain_key \"my really long brain key\" \"newaccount\" \"someaccount\" \"otheraccount\" true\n";
      ss << "\n";
      ss << "This method should be used if you would like the wallet to generate new keys derived from the brain key.\n";
      ss << "The BRAIN_KEY will be used as the owner key, and the active key will be derived from the BRAIN_KEY.  Use\n";
      ss << "register_account if you already know the keys you know the public keys that you would like to register.\n";

   }
   else if( method == "register_account" )
   {
      ss << "usage: register_account ACCOUNT_NAME OWNER_PUBLIC_KEY ACTIVE_PUBLIC_KEY REGISTRAR REFERRER REFERRER_PERCENT BROADCAST\n\n";
      ss << "example: register_account \"newaccount\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"1.3.11\" \"1.3.11\" 50 true\n";
      ss << "\n";
      ss << "Use this method to register an account for which you do not know the private keys.";
   }
   else if( method == "create_asset" )
   {
      ss << "usage: ISSUER SYMBOL PRECISION_DIGITS OPTIONS BITASSET_OPTIONS BROADCAST\n\n";
      ss << "PRECISION_DIGITS: the number of digits after the decimal point\n\n";
      ss << "Example value of OPTIONS: \n";
      ss << fc::json::to_pretty_string( graphene::chain::asset_options() );
      ss << "\nExample value of BITASSET_OPTIONS: \n";
      ss << fc::json::to_pretty_string( graphene::chain::bitasset_options() );
      ss << "\nBITASSET_OPTIONS may be null\n";
   }
   else
   {
      std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
      if (!doxygenHelpString.empty())
         ss << doxygenHelpString;
      else
         ss << "No help defined for method " << method << "\n";
   }

   return ss.str();
}

bool wallet_api::load_wallet_file( string wallet_filename )
{
   return my->load_wallet_file( wallet_filename );
}

void wallet_api::save_wallet_file( string wallet_filename )
{
   my->save_wallet_file( wallet_filename );
}

std::map<string,std::function<string(fc::variant,const fc::variants&)> >
wallet_api::get_result_formatters() const
{
   return my->get_result_formatters();
}

bool wallet_api::is_locked()const
{
   return my->is_locked();
}
bool wallet_api::is_new()const
{
   return my->_wallet.cipher_keys.size() == 0;
}

void wallet_api::encrypt_keys()
{
   my->encrypt_keys();
}

fc::string wallet_api::get_first_contract_address()
{
	return contract_register_operation::get_first_contract_id().operator fc::string();
}

void wallet_api::lock()
{ try {
   FC_ASSERT( !is_locked() );
   encrypt_keys();
   for( auto key : my->_keys )
      key.second = key_to_wif(fc::ecc::private_key());
   my->_keys.clear();
   my->_checksum = fc::sha512();
   my->self.lock_changed(true);
   save_wallet_file();
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::unlock(string password)
{ try {
   FC_ASSERT(password.size() > 0);
   auto pw = fc::sha512::hash(password.c_str(), password.size());
   vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   auto pk = fc::raw::unpack<plain_keys>(decrypted);
   FC_ASSERT(pk.checksum == pw);
   my->_keys = std::move(pk.keys);
   my->_crosschain_keys = std::move(pk.crosschain_keys);
   if (my->_wallet.cipher_keys_extend.valid())
   {
	   vector<char> decrypted_brain_key = fc::aes_decrypt(pw,*(my->_wallet.cipher_keys_extend));
	   auto bkey = fc::raw::unpack<brain_key_usage_info>(decrypted_brain_key);
	   my->_current_brain_key = bkey;
	   
   }

   my->_checksum = pk.checksum;
   my->self.lock_changed(false);
   try
   {
	   if (my->_wallet.mining_accounts.size())
	   {
		   start_mining(my->_wallet.mining_accounts);
	   }
	  
   }
   catch (const fc::exception& e)
   {
	   std::cout << "start mining error:" << e.to_detail_string() << std::endl;
   }

} FC_CAPTURE_AND_RETHROW() }

void wallet_api::set_password( string password )
{
	bool bnew = false;
	if (is_new())
		bnew = true;
   if( !is_new() )
      FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   auto bkey_info=suggest_brain_key();
   if(bnew)
	   set_brain_key(bkey_info.brain_priv_key, 1);
   lock();
}

vector< signed_transaction > wallet_api::import_balance( string name_or_id, const vector<string>& wif_keys, bool broadcast )
{
   return my->import_balance( name_or_id, wif_keys, broadcast );
}

full_transaction wallet_api::refund_request(const string& refund_account, const string txid, bool broadcast )
{
	return my->refund_request(refund_account,txid,broadcast);
}
full_transaction wallet_api::refund_uncombined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	return my->refund_uncombined_transaction(guard,txid,expiration_time, broadcast);
}
full_transaction wallet_api::refund_combined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	return my->refund_combined_transaction(guard, txid, expiration_time, broadcast);
}
full_transaction wallet_api::cancel_eth_sign_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	return my->cancel_eth_sign_transaction(guard, txid, expiration_time, broadcast);
}

full_transaction wallet_api::senator_pass_combined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	return my->senator_pass_combined_transaction(guard, txid, expiration_time, broadcast);
}
full_transaction wallet_api::eth_cancel_fail_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	return my->eth_cancel_fail_transaction(guard, txid, expiration_time, broadcast);
}
full_transaction wallet_api::cancel_coldhot_eth_fail_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	return my->cancel_coldhot_eth_fail_transaction(guard, txid, expiration_time, broadcast);
}
full_transaction wallet_api::cancel_coldhot_uncombined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	return my->cancel_coldhot_uncombined_transaction(guard, txid, expiration_time, broadcast);
}
full_transaction wallet_api::cancel_coldhot_combined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	return my->cancel_coldhot_combined_transaction(guard, txid, expiration_time, broadcast);
}
full_transaction wallet_api::senator_pass_coldhot_combined_transaction(const string guard, const string txid, const int64_t& expiration_time, bool broadcast) {
	return my->senator_pass_coldhot_combined_transaction(guard, txid, expiration_time, broadcast);
}

full_transaction wallet_api::cancel_cold_hot_uncreate_transaction(const string& proposer, const string& trxid, const int64_t& exception_time, bool broadcast) {
	return my->cancel_cold_hot_uncreate_transaction(proposer, trxid, exception_time, broadcast);
}
graphene::chain::full_transaction wallet_api::update_asset_private_keys(const string& from_account, const string& symbol, const string& out_key_file, const string& encrypt_key, bool broadcast/*=true*/)
{
	return my->update_asset_private_keys(from_account,symbol,out_key_file,encrypt_key,broadcast);
}
graphene::chain::full_transaction wallet_api::update_asset_private_keys_with_brain_key(const string& from_account, const string& symbol, const string& out_key_file, const string& encrypt_key, bool broadcast/*=true*/)
{
	return my->update_asset_private_keys(from_account, symbol, out_key_file, encrypt_key, broadcast,true);
}


full_transaction wallet_api::transfer_from_cold_to_hot(const string& proposer, const string& from_account, const string& to_account, const string& amount, const string& asset_symbol,const string& memo,const int64_t& exception_time, bool broadcast)
{
	return my->transfer_from_cold_to_hot(proposer,from_account,to_account,amount,asset_symbol, memo,exception_time,broadcast);
}
vector<optional<multisig_address_object>> wallet_api::get_multi_account_senator(const string & multi_address,const string& symbol)const {
	return my->get_multi_account_guard(multi_address, symbol);
}

vector<multisig_asset_transfer_object> wallet_api::get_multisig_asset_tx() const
{
	return my->get_multisig_asset_tx();
}

vector<optional<multisig_address_object>> wallet_api::get_multi_address_obj(const string& symbol,const account_id_type& guard) const
{
	return my->get_multi_address_obj(symbol,guard);
}

optional<multisig_account_pair_object> wallet_api::get_current_multi_address(const string& symbol) const
{
	return my->get_current_multi_address(symbol);
}

full_transaction wallet_api::sign_multi_asset_trx(const string& account, multisig_asset_transfer_id_type id,const string& guard, bool broadcast)
{
	return my->sign_multi_asset_trx(account,id, guard,broadcast);
}

full_transaction wallet_api::account_change_for_crosschain(const string& proposer,const string& symbol, const string& hot, const string& cold,int64_t expiration_time, bool broadcast)
{
	return my->account_change_for_crosschain(proposer,symbol,hot,cold ,expiration_time,broadcast);
}

full_transaction wallet_api::withdraw_from_link(const string& account, const string& symbol, int64_t amount, bool broadcast)
{
	return my->withdraw_from_link(account, symbol, amount, broadcast);
}

full_transaction wallet_api::bind_tunnel_account(const string& link_account, const string& tunnel_account, const string& symbol, bool broadcast)
{
	return my->bind_tunnel_account(link_account, tunnel_account, symbol, broadcast);
}

full_transaction wallet_api::unbind_tunnel_account(const string& link_account, const string& tunnel_account, const string& symbol, bool broadcast)
{
	return my->unbind_tunnel_account(link_account, tunnel_account, symbol, broadcast);
}

namespace detail {

vector< signed_transaction > wallet_api_impl::import_balance( string name_or_id, const vector<string>& wif_keys, bool broadcast )
{ try {
   FC_ASSERT(!is_locked());
   const dynamic_global_property_object& dpo = _remote_db->get_dynamic_global_properties();
   account_object claimer = get_account( name_or_id );
   uint32_t max_ops_per_tx = 30;

   map< address, private_key_type > keys;  // local index of address -> private key
   vector< address > addrs;
   bool has_wildcard = false;
   addrs.reserve( wif_keys.size() );
   for( const string& wif_key : wif_keys )
   {
      if( wif_key == "*" )
      {
         if( has_wildcard )
            continue;
         for( const public_key_type& pub : _wallet.extra_keys[ claimer.id ] )
         {
            addrs.push_back( pub );
            auto it = _keys.find( pub );
            if( it != _keys.end() )
            {
               fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
               FC_ASSERT( privkey );
               keys[ addrs.back() ] = *privkey;
            }
            else
            {
               wlog( "Somehow _keys has no private key for extra_keys public key ${k}", ("k", pub) );
            }
         }
         has_wildcard = true;
      }
      else
      {
         optional< private_key_type > key = wif_to_key( wif_key );
         FC_ASSERT( key.valid(), "Invalid private key" );
         fc::ecc::public_key pk = key->get_public_key();
         addrs.push_back( pk );
         keys[addrs.back()] = *key;
         // see chain/balance_evaluator.cpp
         addrs.push_back( pts_address( pk, false, 56 ) );
         keys[addrs.back()] = *key;
         addrs.push_back( pts_address( pk, true, 56 ) );
         keys[addrs.back()] = *key;
         addrs.push_back( pts_address( pk, false, 0 ) );
         keys[addrs.back()] = *key;
         addrs.push_back( pts_address( pk, true, 0 ) );
         keys[addrs.back()] = *key;
      }
   }

   vector< balance_object > balances = _remote_db->get_balance_objects( addrs );
   wdump((balances));
   addrs.clear();

   set<asset_id_type> bal_types;
   for( auto b : balances ) bal_types.insert( b.balance.asset_id );

   struct claim_tx
   {
      vector< balance_claim_operation > ops;
      set< address > addrs;
   };
   vector< claim_tx > claim_txs;

   for( const asset_id_type& a : bal_types )
   {
      balance_claim_operation op;
      op.deposit_to_account = claimer.id;
      for( const balance_object& b : balances )
      {
         if( b.balance.asset_id == a )
         {
            op.total_claimed = b.available( dpo.time );
            if( op.total_claimed.amount == 0 )
               continue;
            op.balance_to_claim = b.id;
            op.balance_owner_key = keys[b.owner].get_public_key();
            if( (claim_txs.empty()) || (claim_txs.back().ops.size() >= max_ops_per_tx) )
               claim_txs.emplace_back();
            claim_txs.back().ops.push_back(op);
            claim_txs.back().addrs.insert(b.owner);
         }
      }
   }

   vector< signed_transaction > result;

   for( const claim_tx& ctx : claim_txs )
   {
      signed_transaction tx;
      tx.operations.reserve( ctx.ops.size() );
      for( const balance_claim_operation& op : ctx.ops )
         tx.operations.emplace_back( op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();
      signed_transaction signed_tx = sign_transaction( tx, false );
      for( const address& addr : ctx.addrs )
         signed_tx.sign( keys[addr], _chain_id );
      // if the key for a balance object was the same as a key for the account we're importing it into,
      // we may end up with duplicate signatures, so remove those
      boost::erase(signed_tx.signatures, boost::unique<boost::return_found_end>(boost::sort(signed_tx.signatures)));
      result.push_back( signed_tx );
      if( broadcast )
         _remote_net_broadcast->broadcast_transaction(signed_tx);
   }

   return result;
} FC_CAPTURE_AND_RETHROW( (name_or_id) ) }

}

map<address, string> wallet_api::dump_private_keys()
{
   FC_ASSERT(!is_locked());
   return my->_keys;
}

map<string, crosschain_prkeys> wallet_api::dump_crosschain_private_keys()
{
	FC_ASSERT(!is_locked());
	return my->_crosschain_keys;
}

map<string, crosschain_prkeys> wallet_api::dump_crosschain_private_key(string pubkey)
{
	FC_ASSERT(!is_locked());
	map<string, crosschain_prkeys> result;
	auto iter = my->_crosschain_keys.find(pubkey);
	if (iter != my->_crosschain_keys.end())
		result[pubkey] = iter->second;
	return result;
}


map<address, string> wallet_api::dump_private_key(string account_name)
{
	FC_ASSERT(!is_locked());
	map<address, string> result;
	auto acct = my->get_account(account_name);
	auto iter = my->_keys.find(acct.addr);
	if (iter != my->_keys.end())
		result[acct.addr] = iter->second;
	return result;
}

full_transaction wallet_api::upgrade_account( string name, bool broadcast )
{
   return my->upgrade_account(name,broadcast);
}
full_transaction wallet_api::create_guarantee_order(const string& account, const string& asset_orign, const string& asset_target, const string& symbol,bool broadcast)
{
	return my->create_guarantee_order(account,asset_orign,asset_target,symbol, broadcast);
}
full_transaction wallet_api::cancel_guarantee_order(const guarantee_object_id_type id, bool broadcast)
{
	return my->cancel_guarantee_order(id,broadcast);
}
vector<optional<guarantee_object>> wallet_api::get_my_guarantee_order(const string& account, bool all)
{
	return my->get_my_guarantee_order(account,all);
}
vector<optional<guarantee_object>> wallet_api::list_guarantee_order(const string& symbol,bool all)
{
	return my->list_guarantee_order(symbol,all);
}
fc::variant wallet_api::get_transaction(transaction_id_type id) const
{
	return my->get_transaction(id);
}

vector<transaction_id_type> wallet_api::list_transactions(uint32_t blocknum , uint32_t nums) const
{
	return my->list_transactions(blocknum,nums);
}
optional<guarantee_object> wallet_api::get_guarantee_order(const guarantee_object_id_type id)
{
	return my->get_guarantee_order(id);
}

void wallet_api::set_guarantee_id(const guarantee_object_id_type id)
{
	return my->set_guarantee_id(id);
}
void wallet_api::remove_guarantee_id()
{
	return my->remove_guarantee_id();
}
full_transaction wallet_api::senator_appointed_publisher(const string& account, const account_id_type publisher, const string& symbol, int64_t expiration_time, bool broadcast)
{
	return my->guard_appointed_publisher(account,publisher,symbol, expiration_time,broadcast);
}
full_transaction wallet_api::senator_cancel_publisher(const string& account, const account_id_type publisher, const string& symbol, int64_t expiration_time, bool broadcast)
{
	return my->guard_cancel_publisher(account, publisher, symbol, expiration_time, broadcast);
}
full_transaction wallet_api::senator_appointed_crosschain_fee(const string& account, const share_type fee, const string& symbol, int64_t expiration_time, bool broadcast)
{
	return my->senator_appointed_crosschain_fee(account,fee,symbol, expiration_time,broadcast);
}
full_transaction wallet_api::senator_change_eth_gas_price(const string& account, const string& gas_price, const string& symbol, int64_t expiration_time, bool broadcast)
{
	return my->senator_change_eth_gas_price(account, gas_price, symbol, expiration_time, broadcast);
}

full_transaction wallet_api::senator_appointed_lockbalance_senator(const string& account, const std::map<string, asset>& lockbalance, int64_t expiration_time, bool broadcast)
{
	return my->senator_appointed_lockbalance_senator(account, lockbalance, expiration_time, broadcast);
}
full_transaction wallet_api::senator_determine_block_payment(const string& account, const std::map<uint32_t, uint32_t>& blocks_pays, int64_t expiration_time, bool broadcast)
{
	return my->senator_determine_block_payment(account, blocks_pays,expiration_time,broadcast);
}

full_transaction wallet_api::senator_determine_withdraw_deposit(const string& account, bool can,const string& symbol, int64_t expiration_time, bool broadcast)
{
	return my->senator_determine_withdraw_deposit(account, can, symbol,expiration_time, broadcast);
}
address wallet_api::create_multisignature_address(const string& account, const fc::flat_set<public_key_type>& pubs, int required, bool broadcast)
{
	return my->create_multisignature_address(account,pubs,required,broadcast);
}
map<account_id_type, vector<asset>> wallet_api::get_citizen_lockbalance_info(const string& account)
{
	return my->get_citizen_lockbalance_info(account);
}
vector<optional< eth_multi_account_trx_object>> wallet_api::get_eth_multi_account_trx(const int & mul_acc_tx_state) {
	return my->get_eth_multi_account_trx(mul_acc_tx_state);
}

/*full_transaction wallet_api::sell_asset(string seller_account,
                                          string amount_to_sell,
                                          string symbol_to_sell,
                                          string min_to_receive,
                                          string symbol_to_receive,
                                          uint32_t expiration,
                                          bool   fill_or_kill,
                                          bool   broadcast)
{
   return my->sell_asset(seller_account, amount_to_sell, symbol_to_sell, min_to_receive,
                         symbol_to_receive, expiration, fill_or_kill, broadcast);
}*/

full_transaction wallet_api::sell( string seller_account,
                                     string base,
                                     string quote,
                                     double rate,
                                     double amount,
                                     bool broadcast )
{
   return my->sell_asset( seller_account, std::to_string( amount ), base,
                          std::to_string( rate * amount ), quote, 0, false, broadcast );
}

/*full_transaction wallet_api::buy( string buyer_account,
                                    string base,
                                    string quote,
                                    double rate,
                                    double amount,
                                    bool broadcast )
{
   return my->sell_asset( buyer_account, std::to_string( rate * amount ), quote,
                          std::to_string( amount ), base, 0, false, broadcast );
}*/
vector<optional<account_binding_object>> wallet_api::get_binding_account(const string& account, const string& symbol) const
{
	return my->get_binding_account(account,symbol);
}
vector<optional<multisig_account_pair_object>> wallet_api::get_multisig_account_pair(const string& symbol) const
{
	return my->get_multisig_account_pair(symbol);
}
optional<multisig_account_pair_object> wallet_api::get_multisig_account_pair_by_id(const multisig_account_pair_id_type& id) const
{
	return my->get_multisig_account_pair(id);
}
full_transaction wallet_api::borrow_asset(string seller_name, string amount_to_sell,
                                                string asset_symbol, string amount_of_collateral, bool broadcast)
{
   FC_ASSERT(!is_locked());
   return my->borrow_asset(seller_name, amount_to_sell, asset_symbol, amount_of_collateral, broadcast);
}

full_transaction wallet_api::cancel_order(object_id_type order_id, bool broadcast)
{
   FC_ASSERT(!is_locked());
   return my->cancel_order(order_id, broadcast);
}

string wallet_api::get_key_label( public_key_type key )const
{
   auto key_itr   = my->_wallet.labeled_keys.get<by_key>().find(key);
   if( key_itr != my->_wallet.labeled_keys.get<by_key>().end() )
      return key_itr->label;
   return string();
}

string wallet_api::get_private_key( public_key_type pubkey )const
{
   return key_to_wif( my->get_private_key( pubkey ) );
}

public_key_type  wallet_api::get_public_key( string label )const
{
   try { return fc::variant(label).as<public_key_type>(); } catch ( ... ){}

   auto key_itr   = my->_wallet.labeled_keys.get<by_label>().find(label);
   if( key_itr != my->_wallet.labeled_keys.get<by_label>().end() )
      return key_itr->key;
   return public_key_type();
}

bool               wallet_api::set_key_label( public_key_type key, string label )
{
   auto result = my->_wallet.labeled_keys.insert( key_label{label,key} );
   if( result.second  ) return true;

   auto key_itr   = my->_wallet.labeled_keys.get<by_key>().find(key);
   auto label_itr = my->_wallet.labeled_keys.get<by_label>().find(label);
   if( label_itr == my->_wallet.labeled_keys.get<by_label>().end() )
   {
      if( key_itr != my->_wallet.labeled_keys.get<by_key>().end() )
         return my->_wallet.labeled_keys.get<by_key>().modify( key_itr, [&]( key_label& obj ){ obj.label = label; } );
   }
   return false;
}
map<string,public_key_type> wallet_api::get_blind_accounts()const
{
   map<string,public_key_type> result;
   for( const auto& item : my->_wallet.labeled_keys )
      result[item.label] = item.key;
   return result;
}
map<string,public_key_type> wallet_api::get_my_blind_accounts()const
{
   FC_ASSERT( !is_locked() );
   map<string,public_key_type> result;
   for( const auto& item : my->_wallet.labeled_keys )
   {
      if( my->_keys.find(item.key) != my->_keys.end() )
         result[item.label] = item.key;
   }
   return result;
}

public_key_type    wallet_api::create_blind_account( string label, string brain_key  )
{
   FC_ASSERT( !is_locked() );

   auto label_itr = my->_wallet.labeled_keys.get<by_label>().find(label);
   if( label_itr !=  my->_wallet.labeled_keys.get<by_label>().end() )
      FC_ASSERT( !"Key with label already exists" );
   brain_key = fc::trim_and_normalize_spaces( brain_key );
   auto secret = fc::sha256::hash( brain_key.c_str(), brain_key.size() );
   auto priv_key = fc::ecc::private_key::regenerate( secret );
   public_key_type pub_key  = priv_key.get_public_key();

   FC_ASSERT( set_key_label( pub_key, label ) );

   my->_keys[pub_key] = graphene::utilities::key_to_wif( priv_key );

   save_wallet_file();
   return pub_key;
}

vector<asset>   wallet_api::get_blind_balances( string key_or_label )
{
   vector<asset> result;
   map<asset_id_type, share_type> balances;

   vector<commitment_type> used;

   auto pub_key = get_public_key( key_or_label );
   auto& to_asset_used_idx = my->_wallet.blind_receipts.get<by_to_asset_used>();
   auto start = to_asset_used_idx.lower_bound( std::make_tuple(pub_key,asset_id_type(0),false)  );
   auto end = to_asset_used_idx.lower_bound( std::make_tuple(pub_key,asset_id_type(uint32_t(0xffffffff)),true)  );
   while( start != end )
   {
      if( !start->used  )
      {
         auto answer = my->_remote_db->get_blinded_balances( {start->commitment()} );
         if( answer.size() )
            balances[start->amount.asset_id] += start->amount.amount;
         else
            used.push_back( start->commitment() );
      }
      ++start;
   }
   for( const auto& u : used )
   {
      auto itr = my->_wallet.blind_receipts.get<by_commitment>().find( u );
      my->_wallet.blind_receipts.modify( itr, []( blind_receipt& r ){ r.used = true; } );
   }
   for( auto item : balances )
      result.push_back( asset( item.second, item.first ) );
   return result;
}

blind_confirmation wallet_api::transfer_from_blind( string from_blind_account_key_or_label,
                                                    string to_account_id_or_name,
                                                    string amount_in,
                                                    string symbol,
                                                    bool broadcast )
{ try {
   transfer_from_blind_operation from_blind;


   auto fees  = my->_remote_db->get_global_properties().parameters.current_fees;
   fc::optional<asset_object> asset_obj = get_asset(symbol);
   FC_ASSERT(asset_obj.valid(), "Could not find asset matching ${asset}", ("asset", symbol));
   auto amount = asset_obj->amount_from_string(amount_in);

   from_blind.fee  = fees->calculate_fee( from_blind, asset_obj->options.core_exchange_rate );

   auto blind_in = asset_obj->amount_to_string( from_blind.fee + amount );


   auto conf = blind_transfer_help( from_blind_account_key_or_label,
                               from_blind_account_key_or_label,
                               blind_in, symbol, false, true/*to_temp*/ );
   FC_ASSERT( conf.outputs.size() > 0 );

   auto to_account = my->get_account( to_account_id_or_name );
   from_blind.to = to_account.id;
   from_blind.amount = amount;
   from_blind.blinding_factor = conf.outputs.back().decrypted_memo.blinding_factor;
   from_blind.inputs.push_back( {conf.outputs.back().decrypted_memo.commitment, authority() } );
   from_blind.fee  = fees->calculate_fee( from_blind, asset_obj->options.core_exchange_rate );

   idump( (from_blind) );
   conf.trx.operations.push_back(from_blind);
   ilog( "about to validate" );
   conf.trx.validate();

   if( broadcast && conf.outputs.size() == 2 ) {

       // Save the change
       blind_confirmation::output conf_output;
       blind_confirmation::output change_output = conf.outputs[0];

       // The wallet must have a private key for confirmation.to, this is used to decrypt the memo
       public_key_type from_key = get_public_key(from_blind_account_key_or_label);
       conf_output.confirmation.to = from_key;
       conf_output.confirmation.one_time_key = change_output.confirmation.one_time_key;
       conf_output.confirmation.encrypted_memo = change_output.confirmation.encrypted_memo;
       conf_output.confirmation_receipt = conf_output.confirmation;
       //try {
       receive_blind_transfer( conf_output.confirmation_receipt, from_blind_account_key_or_label, "@"+to_account.name );
       //} catch ( ... ){}
   }

   ilog( "about to broadcast" );
   conf.trx = sign_transaction( conf.trx, broadcast );

   return conf;
} FC_CAPTURE_AND_RETHROW( (from_blind_account_key_or_label)(to_account_id_or_name)(amount_in)(symbol) ) }

blind_confirmation wallet_api::blind_transfer( string from_key_or_label,
                                               string to_key_or_label,
                                               string amount_in,
                                               string symbol,
                                               bool broadcast )
{
   return blind_transfer_help( from_key_or_label, to_key_or_label, amount_in, symbol, broadcast, false );
}
blind_confirmation wallet_api::blind_transfer_help( string from_key_or_label,
                                               string to_key_or_label,
                                               string amount_in,
                                               string symbol,
                                               bool broadcast,
                                               bool to_temp )
{
   blind_confirmation confirm;
   try {

   FC_ASSERT( !is_locked() );
   public_key_type from_key = get_public_key(from_key_or_label);
   public_key_type to_key   = get_public_key(to_key_or_label);

   fc::optional<asset_object> asset_obj = get_asset(symbol);
   FC_ASSERT(asset_obj.valid(), "Could not find asset matching ${asset}", ("asset", symbol));

   blind_transfer_operation blind_tr;
   blind_tr.outputs.resize(2);

   auto fees  = my->_remote_db->get_global_properties().parameters.current_fees;

   auto amount = asset_obj->amount_from_string(amount_in);

   asset total_amount = asset_obj->amount(0);

   vector<fc::sha256> blinding_factors;

   //auto from_priv_key = my->get_private_key( from_key );

   blind_tr.fee  = fees->calculate_fee( blind_tr, asset_obj->options.core_exchange_rate );

   vector<commitment_type> used;

   auto& to_asset_used_idx = my->_wallet.blind_receipts.get<by_to_asset_used>();
   auto start = to_asset_used_idx.lower_bound( std::make_tuple(from_key,amount.asset_id,false)  );
   auto end = to_asset_used_idx.lower_bound( std::make_tuple(from_key,amount.asset_id,true)  );
   while( start != end )
   {
      auto result = my->_remote_db->get_blinded_balances( {start->commitment() } );
      if( result.size() == 0 )
      {
         used.push_back( start->commitment() );
      }
      else
      {
         blind_tr.inputs.push_back({start->commitment(), start->control_authority});
         blinding_factors.push_back( start->data.blinding_factor );
         total_amount += start->amount;

         if( total_amount >= amount + blind_tr.fee )
            break;
      }
      ++start;
   }
   for( const auto& u : used )
   {
      auto itr = my->_wallet.blind_receipts.get<by_commitment>().find( u );
      my->_wallet.blind_receipts.modify( itr, []( blind_receipt& r ){ r.used = true; } );
   }

   FC_ASSERT( total_amount >= amount+blind_tr.fee, "Insufficent Balance", ("available",total_amount)("amount",amount)("fee",blind_tr.fee) );

   auto one_time_key = fc::ecc::private_key::generate();
   auto secret       = one_time_key.get_shared_secret( to_key );
   auto child        = fc::sha256::hash( secret );
   auto nonce        = fc::sha256::hash( one_time_key.get_secret() );
   auto blind_factor = fc::sha256::hash( child );

   auto from_secret  = one_time_key.get_shared_secret( from_key );
   auto from_child   = fc::sha256::hash( from_secret );
   auto from_nonce   = fc::sha256::hash( nonce );

   auto change = total_amount - amount - blind_tr.fee;
   fc::sha256 change_blind_factor;
   fc::sha256 to_blind_factor;
   if( change.amount > 0 )
   {
      idump(("to_blind_factor")(blind_factor) );
      blinding_factors.push_back( blind_factor );
      change_blind_factor = fc::ecc::blind_sum( blinding_factors, blinding_factors.size() - 1 );
      wdump(("change_blind_factor")(change_blind_factor) );
   }
   else // change == 0
   {
      blind_tr.outputs.resize(1);
      blind_factor = fc::ecc::blind_sum( blinding_factors, blinding_factors.size() );
      idump(("to_sum_blind_factor")(blind_factor) );
      blinding_factors.push_back( blind_factor );
      idump(("nochange to_blind_factor")(blind_factor) );
   }
   fc::ecc::public_key from_pub_key = from_key;
   fc::ecc::public_key to_pub_key = to_key;

   blind_output to_out;
   to_out.owner       = to_temp ? authority() : authority( 1, public_key_type( to_pub_key.child( child ) ), 1 );
   to_out.commitment  = fc::ecc::blind( blind_factor, amount.amount.value );
   idump(("to_out.blind")(blind_factor)(to_out.commitment) );


   if( blind_tr.outputs.size() > 1 )
   {
      to_out.range_proof = fc::ecc::range_proof_sign( 0, to_out.commitment, blind_factor, nonce,  0, 0, amount.amount.value );

      blind_output change_out;
      change_out.owner       = authority( 1, public_key_type( from_pub_key.child( from_child ) ), 1 );
      change_out.commitment  = fc::ecc::blind( change_blind_factor, change.amount.value );
      change_out.range_proof = fc::ecc::range_proof_sign( 0, change_out.commitment, change_blind_factor, from_nonce,  0, 0, change.amount.value );
      blind_tr.outputs[1] = change_out;


      blind_confirmation::output conf_output;
      conf_output.label = from_key_or_label;
      conf_output.pub_key = from_key;
      conf_output.decrypted_memo.from = from_key;
      conf_output.decrypted_memo.amount = change;
      conf_output.decrypted_memo.blinding_factor = change_blind_factor;
      conf_output.decrypted_memo.commitment = change_out.commitment;
      conf_output.decrypted_memo.check   = from_secret._hash[0];
      conf_output.confirmation.one_time_key = one_time_key.get_public_key();
      conf_output.confirmation.to = from_key;
      conf_output.confirmation.encrypted_memo = fc::aes_encrypt( from_secret, fc::raw::pack( conf_output.decrypted_memo ) );
      conf_output.auth = change_out.owner;
      conf_output.confirmation_receipt = conf_output.confirmation;

      confirm.outputs.push_back( conf_output );
   }
   blind_tr.outputs[0] = to_out;

   blind_confirmation::output conf_output;
   conf_output.label = to_key_or_label;
   conf_output.pub_key = to_key;
   conf_output.decrypted_memo.from = from_key;
   conf_output.decrypted_memo.amount = amount;
   conf_output.decrypted_memo.blinding_factor = blind_factor;
   conf_output.decrypted_memo.commitment = to_out.commitment;
   conf_output.decrypted_memo.check   = secret._hash[0];
   conf_output.confirmation.one_time_key = one_time_key.get_public_key();
   conf_output.confirmation.to = to_key;
   conf_output.confirmation.encrypted_memo = fc::aes_encrypt( secret, fc::raw::pack( conf_output.decrypted_memo ) );
   conf_output.auth = to_out.owner;
   conf_output.confirmation_receipt = conf_output.confirmation;

   confirm.outputs.push_back( conf_output );

   /** commitments must be in sorted order */
   std::sort( blind_tr.outputs.begin(), blind_tr.outputs.end(),
              [&]( const blind_output& a, const blind_output& b ){ return a.commitment < b.commitment; } );
   std::sort( blind_tr.inputs.begin(), blind_tr.inputs.end(),
              [&]( const blind_input& a, const blind_input& b ){ return a.commitment < b.commitment; } );

   confirm.trx.operations.emplace_back( std::move(blind_tr) );
   ilog( "validate before" );
   confirm.trx.validate();
   confirm.trx = sign_transaction(confirm.trx, broadcast);

   if( broadcast )
   {
      for( const auto& out : confirm.outputs )
      {
         try { receive_blind_transfer( out.confirmation_receipt, from_key_or_label, "" ); } catch ( ... ){}
      }
   }

   return confirm;
} FC_CAPTURE_AND_RETHROW( (from_key_or_label)(to_key_or_label)(amount_in)(symbol)(broadcast)(confirm) ) }



/**
 *  Transfers a public balance from @from to one or more blinded balances using a
 *  stealth transfer.
 */
blind_confirmation wallet_api::transfer_to_blind( string from_account_id_or_name,
                                                  string asset_symbol,
                                                  /** map from key or label to amount */
                                                  vector<pair<string, string>> to_amounts,
                                                  bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
   idump((to_amounts));

   blind_confirmation confirm;
   account_object from_account = my->get_account(from_account_id_or_name);

   fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
   FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

   transfer_to_blind_operation bop;
   bop.from   = from_account.id;

   vector<fc::sha256> blinding_factors;

   asset total_amount = asset_obj->amount(0);

   for( auto item : to_amounts )
   {
      auto one_time_key = fc::ecc::private_key::generate();
      auto to_key       = get_public_key( item.first );
      auto secret       = one_time_key.get_shared_secret( to_key );
      auto child        = fc::sha256::hash( secret );
      auto nonce        = fc::sha256::hash( one_time_key.get_secret() );
      auto blind_factor = fc::sha256::hash( child );

      blinding_factors.push_back( blind_factor );

      auto amount = asset_obj->amount_from_string(item.second);
      total_amount += amount;


      fc::ecc::public_key to_pub_key = to_key;
      blind_output out;
      out.owner       = authority( 1, public_key_type( to_pub_key.child( child ) ), 1 );
      out.commitment  = fc::ecc::blind( blind_factor, amount.amount.value );
      if( to_amounts.size() > 1 )
         out.range_proof = fc::ecc::range_proof_sign( 0, out.commitment, blind_factor, nonce,  0, 0, amount.amount.value );


      blind_confirmation::output conf_output;
      conf_output.label = item.first;
      conf_output.pub_key = to_key;
      conf_output.decrypted_memo.amount = amount;
      conf_output.decrypted_memo.blinding_factor = blind_factor;
      conf_output.decrypted_memo.commitment = out.commitment;
      conf_output.decrypted_memo.check   = secret._hash[0];
      conf_output.confirmation.one_time_key = one_time_key.get_public_key();
      conf_output.confirmation.to = to_key;
      conf_output.confirmation.encrypted_memo = fc::aes_encrypt( secret, fc::raw::pack( conf_output.decrypted_memo ) );
      conf_output.confirmation_receipt = conf_output.confirmation;

      confirm.outputs.push_back( conf_output );

      bop.outputs.push_back(out);
   }
   bop.amount          = total_amount;
   bop.blinding_factor = fc::ecc::blind_sum( blinding_factors, blinding_factors.size() );

   /** commitments must be in sorted order */
   std::sort( bop.outputs.begin(), bop.outputs.end(),
              [&]( const blind_output& a, const blind_output& b ){ return a.commitment < b.commitment; } );

   confirm.trx.operations.push_back( bop );
   my->set_operation_fees( confirm.trx, my->_remote_db->get_global_properties().parameters.current_fees);
   confirm.trx.validate();
   confirm.trx = sign_transaction(confirm.trx, broadcast);

   if( broadcast )
   {
      for( const auto& out : confirm.outputs )
      {
         try { receive_blind_transfer( out.confirmation_receipt, "@"+from_account.name, "from @"+from_account.name ); } catch ( ... ){}
      }
   }

   return confirm;
} FC_CAPTURE_AND_RETHROW( (from_account_id_or_name)(asset_symbol)(to_amounts) ) }

blind_receipt wallet_api::receive_blind_transfer( string confirmation_receipt, string opt_from, string opt_memo )
{
   FC_ASSERT( !is_locked() );
   stealth_confirmation conf(confirmation_receipt);
   FC_ASSERT( conf.to );

   blind_receipt result;
   result.conf = conf;

   auto to_priv_key_itr = my->_keys.find( *conf.to );
   FC_ASSERT( to_priv_key_itr != my->_keys.end(), "No private key for receiver", ("conf",conf) );


   auto to_priv_key = wif_to_key( to_priv_key_itr->second );
   FC_ASSERT( to_priv_key );

   auto secret       = to_priv_key->get_shared_secret( conf.one_time_key );
   auto child        = fc::sha256::hash( secret );

   auto child_priv_key = to_priv_key->child( child );
   //auto blind_factor = fc::sha256::hash( child );

   auto plain_memo = fc::aes_decrypt( secret, conf.encrypted_memo );
   auto memo = fc::raw::unpack<stealth_confirmation::memo_data>( plain_memo );

   result.to_key   = *conf.to;
   result.to_label = get_key_label( result.to_key );
   if( memo.from )
   {
      result.from_key = *memo.from;
      result.from_label = get_key_label( result.from_key );
      if( result.from_label == string() )
      {
         result.from_label = opt_from;
         set_key_label( result.from_key, result.from_label );
      }
   }
   else
   {
      result.from_label = opt_from;
   }
   result.amount = memo.amount;
   result.memo = opt_memo;

   // confirm the amount matches the commitment (verify the blinding factor)
   auto commtiment_test = fc::ecc::blind( memo.blinding_factor, memo.amount.amount.value );
   FC_ASSERT( fc::ecc::verify_sum( {commtiment_test}, {memo.commitment}, 0 ) );

   blind_balance bal;
   bal.amount = memo.amount;
   bal.to     = *conf.to;
   if( memo.from ) bal.from   = *memo.from;
   bal.one_time_key = conf.one_time_key;
   bal.blinding_factor = memo.blinding_factor;
   bal.commitment = memo.commitment;
   bal.used = false;

   auto child_pubkey = child_priv_key.get_public_key();
   auto owner = authority(1, public_key_type(child_pubkey), 1);
   result.control_authority = owner;
   result.data = memo;

   auto child_key_itr = owner.key_auths.find( child_pubkey );
   if( child_key_itr != owner.key_auths.end() )
      my->_keys[child_key_itr->first] = key_to_wif( child_priv_key );

   // my->_wallet.blinded_balances[memo.amount.asset_id][bal.to].push_back( bal );

   result.date = fc::time_point::now();
   my->_wallet.blind_receipts.insert( result );
   my->_keys[child_pubkey] = key_to_wif( child_priv_key );

   save_wallet_file();

   return result;
}

vector<blind_receipt> wallet_api::blind_history( string key_or_account )
{
   vector<blind_receipt> result;
   auto pub_key = get_public_key( key_or_account );

   if( pub_key == public_key_type() )
      return vector<blind_receipt>();

   for( auto& r : my->_wallet.blind_receipts )
   {
      if( r.from_key == pub_key || r.to_key == pub_key )
         result.push_back( r );
   }
   std::sort( result.begin(), result.end(), [&]( const blind_receipt& a, const blind_receipt& b ){ return a.date > b.date; } );
   return result;
}

crosschain_prkeys wallet_api::create_crosschain_symbol(const string& symbol)
{
	return my->create_crosschain_symbol(symbol);
}
//crosschain_prkeys wallet_api::cupdate_asset_private_keysreate_crosschain_symbol_with_brainkey(const string& symbol)
//{
//	return my->create_crosschain_symbol(symbol);
//}

crosschain_prkeys wallet_api::wallet_create_crosschain_symbol(const string& symbol)
{
	return my->wallet_create_crosschain_symbol(symbol);
}
crosschain_prkeys wallet_api::wallet_create_crosschain_symbol_with_brain_key(const string& symbol)
{
	return my->wallet_create_crosschain_symbol(symbol,true);
}

order_book wallet_api::get_order_book( const string& base, const string& quote, unsigned limit )
{
   return( my->_remote_db->get_order_book( base, quote, limit ) );
}

signed_block_with_info::signed_block_with_info( const signed_block& block )
   : signed_block( block )
{
   block_id = id();
   number = block_num();
   signing_key = signee();
   transaction_ids.reserve( transactions.size() );
   for( const processed_transaction& tx : transactions )
	   transaction_ids.push_back( tx.id() );
}

map<string, crosschain_prkeys> wallet_api::decrypt_coldkeys(const string& key, const string& file)
{
	map<string, crosschain_prkeys> keys;
	std::ifstream in(file, std::ios::in | std::ios::binary);
	FC_ASSERT(in.is_open(),"key file open failed!\n");
	{

		std::vector<char> key_file_data((std::istreambuf_iterator<char>(in)),
			(std::istreambuf_iterator<char>()));
		in.close();
		if (key_file_data.size() > 0)
		{
			const auto plain_text = fc::aes_decrypt(fc::sha512(key.c_str(), key.length()), key_file_data);
			keys = fc::raw::unpack<map<string, crosschain_prkeys>>(plain_text);
		}
	}
	return keys;
}
void wallet_api::start_citizen(bool start)
{
    my->start_miner(start);
}
void wallet_api::start_mining(const vector<string>& accts)
{
	vector<address> addrs;
	map<miner_id_type, private_key> keys;
	auto& idx = my->_wallet.my_accounts.get<by_name>();
	for (auto acct : accts)
	{
		auto oac=idx.find(acct);
		FC_ASSERT(oac!= idx.end(), "account not found!");
		auto acc_obj = *(oac);
		fc::optional<miner_object> witness = my->_remote_db->get_miner_by_account(acc_obj.get_id());
		FC_ASSERT(witness.valid(),"only citizen can mine");
		FC_ASSERT(my->_keys.find(acc_obj.addr)!=my->_keys.end(),"my key is not in keys");
		fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(my->_keys[acc_obj.addr]);
		if (!optional_private_key)
			FC_THROW("Invalid private key");
		keys.insert(make_pair((*witness).id.as<miner_id_type>(),*optional_private_key));
	}
	my->start_mining(keys);
	my->_wallet.mining_accounts = accts;
}
std::map<std::string, fc::ntp_info> wallet_api::get_ntp_info()
{
	std::map<std::string, fc::ntp_info> res;
	res["witness_node"]= my->_remote_db->get_ntp_info();
	res["cli_wallet"]= fc::time_point::get_ntp_info();;
	return res;
}
void wallet_api::ntp_update_time()
{
	time_point::ntp_update_time();
	my->_remote_db->ntp_update_time();
}

bool wallet_api::set_brain_key(string  key, const int next)
{
	FC_ASSERT(!is_locked(),"");
	FC_ASSERT(!my->_current_brain_key.valid(), "brain key already set");
	FC_ASSERT(next>0, "next should bigger than 0");
	if (key == "")
	{
		key = suggest_brain_key().brain_priv_key;
	}
	auto pk = normalize_brain_key(key);
	brain_key_usage_info info;
	info.key = key;
	info.next = next;

	my->_current_brain_key = info;
	save_wallet_file();
	return true;
}

graphene::wallet::brain_key_usage_info wallet_api::dump_brain_key_usage_info(const string& password)
{
	FC_ASSERT(!is_locked(), "");
	FC_ASSERT(password.size() > 0);
	auto pw = fc::sha512::hash(password.c_str(), password.size());
	FC_ASSERT(pw==my->_checksum);
	FC_ASSERT(my->_current_brain_key.valid());
	return *my->_current_brain_key;
}

void wallet_api::witness_node_stop()
{
    my->witness_node_stop();
}
} } // graphene::wallet

void fc::to_variant(const account_multi_index_type& accts, fc::variant& vo)
{
   vo = vector<account_object>(accts.begin(), accts.end());
}

void fc::from_variant(const fc::variant& var, account_multi_index_type& vo)
{
   const vector<account_object>& v = var.as<vector<account_object>>();
   vo = account_multi_index_type(v.begin(), v.end());
}
