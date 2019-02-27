#include <graphene/crosschain/crosschain_interface_emu.hpp>
#include <fc/filesystem.hpp>
#include <iostream>
#include <boost/filesystem.hpp>
#include <fc/smart_ref_impl.hpp>
namespace graphene {
	namespace crosschain {


		bool crosschain_interface_emu::valid_config()
		{
			return true;
		}
		void crosschain_interface_emu::initialize_config(fc::variant_object &json_config)
		{
			config = json_config;
		}

		bool crosschain_interface_emu::create_wallet(std::string wallet_name, std::string wallet_passprase) {
			if (_plugin_wallet_filepath == "")
			{
				_plugin_wallet_filepath = boost::filesystem::initial_path<boost::filesystem::path>().string();
				_plugin_wallet_filepath += "/";
			}
			if (fc::exists(_plugin_wallet_filepath + wallet_name))
				return false;
			_wallet_name = wallet_name;
			_wallet = *make_shared<graphene::wallet::wallet_data>();
			_checksum = fc::sha512::hash(wallet_passprase.c_str(), wallet_passprase.size());

			lock();
			save_wallet_file(_plugin_wallet_filepath + wallet_name);

		}
		crosschain_trx crosschain_interface_emu::turn_trxs(const fc::variant_object & trx)
		{
			crosschain_trx hdtxs;
			
			return hdtxs;
		}
		hd_trx crosschain_interface_emu::turn_trx(const fc::variant_object & trx) {
			return hd_trx();
		}
		bool crosschain_interface_emu::unlock_wallet(std::string wallet_name, std::string wallet_passprase, uint32_t duration)
		{
			FC_ASSERT(wallet_passprase.size() > 0);
			auto pw = fc::sha512::hash(wallet_passprase.c_str(), wallet_passprase.size());
			vector<char> decrypted = fc::aes_decrypt(pw, _wallet.cipher_keys);
			auto pk = fc::raw::unpack<graphene::wallet::plain_keys>(decrypted);
			FC_ASSERT(pk.checksum == pw);
			_keys = std::move(pk.keys);
			_checksum = pk.checksum;
			return true;
		}

		bool crosschain_interface_emu::open_wallet(string wallet_name)
		{
			if (_plugin_wallet_filepath == "")
			{
				_plugin_wallet_filepath = boost::filesystem::initial_path<boost::filesystem::path>().string();
				_plugin_wallet_filepath += "/";
			}
			_wallet_name = wallet_name;
			load_wallet_file(_plugin_wallet_filepath + wallet_name);
			return true;
		}


		void crosschain_interface_emu::close_wallet() {
			if (!is_locked())
			{
				encrypt_keys();
				for (auto iter = _keys.begin(); iter != _keys.end(); iter++)
					iter->second = key_to_wif(fc::ecc::private_key());
				_keys.clear();
				_checksum = fc::sha512();
			}
			save_wallet_file(_plugin_wallet_filepath + _wallet_name);
		}

		std::vector<std::string> crosschain_interface_emu::wallet_list()
		{
			vector<string> wallets;
			if (!fc::is_directory(_plugin_wallet_filepath))
				return wallets;

			auto path = _plugin_wallet_filepath;
			fc::directory_iterator end_itr; // constructs terminator
			for (fc::directory_iterator itr(path); itr != end_itr; ++itr)
			{
				if (!itr->stem().string().empty() && !fc::is_directory(*itr))
				{
					wallets.push_back((*itr).stem().string());
				}
			}

			std::sort(wallets.begin(), wallets.end());
			return wallets;
		}

		std::string crosschain_interface_emu::create_normal_account(std::string account_name, const fc::optional<fc::ecc::private_key> key/*=fc::optional<fc::ecc::private_key>()*/)
		{
			auto& idx = _wallet.my_accounts.get<by_name>();
			auto itr = idx.find(account_name);
			FC_ASSERT(itr == idx.end());


			auto user_key = key.valid()?*key: fc::ecc::extended_private_key::generate();
			auto owner_key = derive_private_key(key_to_wif(user_key), 0);
			account_object new_account;
			new_account.addr = address(owner_key.get_public_key());
			new_account.name = account_name;
			_balances.insert(std::make_pair("account_name", 10000));
			_wallet.my_accounts.emplace(new_account);
			_keys.emplace(new_account.addr, key_to_wif(owner_key));
			save_wallet_file(_wallet_name);
			return string(new_account.addr);
		}

		std::map<std::string, std::string> crosschain_interface_emu::create_multi_sig_account(std::string account_name, std::vector<std::string> addresses, uint32_t nrequired)
		{
			fc::sha256::encoder endcoder;
			for (int i = 0; i < addresses.size(); ++i)
			{
				fc::raw::pack(endcoder, addresses[i]);
			}
			fc::sha256 secret = endcoder.result();
			auto user_key = private_key::regenerate(secret);
			auto owner_key = derive_private_key(key_to_wif(user_key), 0);
			account_object new_account;
			new_account.addr = address(owner_key.get_public_key());
			new_account.name = account_name;
			_wallet.my_accounts.emplace(new_account);
			_keys.emplace(new_account.addr, key_to_wif(owner_key));
			save_wallet_file(_wallet_name);
			return std::map<std::string,std::string>();
		}

		std::vector<hd_trx> crosschain_interface_emu::deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit)
		{
			std::vector<hd_trx> results;
			//TODo add rpc get function
			hd_trx a;
			a.trx_id = "trx-id-test";
			a.from_account = user_account;
			a.to_account = "to_account";
			a.amount = 10;
			a.asset_symbol = "mbtc";
			a.block_num = 1;
			//
			results.emplace_back(a);
			return results;
			//return std::vector<fc::variant_object>();
		}

		fc::variant_object crosschain_interface_emu::transaction_query(std::string trx_id)
		{
			//TODo add rpc get function
			hd_trx a;
			a.trx_id = trx_id;
			a.from_account = "from_account";
			a.to_account = "to_account";
			a.amount = 10;
			a.asset_symbol = "mbtc";
			a.block_num = 1;
			fc::variant v;
			fc::to_variant(a, v);
			return v.get_object();
		}

		fc::variant_object crosschain_interface_emu::transfer(const std::string &from_account, const std::string &to_account, uint64_t amount, const std::string &symbol, const std::string &memo, bool broadcast)
		{
			//TODo add rpc get function
			hd_trx a;
			a.trx_id = "trx-id-test";
			a.from_account = from_account;
			a.to_account = to_account;
			a.amount = amount;
			a.asset_symbol = memo;
			a.block_num = 1;
			fc::variant v;
			fc::to_variant(a, v);
			return v.get_object();
		}

		fc::variant_object crosschain_interface_emu::create_multisig_transaction(const std::string &from_account,const std::map<std::string, std::string> dest_info, const std::string &symbol, const std::string &memo)
		{
			return fc::variant_object();
		}
		fc::variant_object crosschain_interface_emu::create_multisig_transaction(std::string &from_account, std::string &to_account, const std::string& amount, std::string &symbol, std::string &memo, bool broadcast)
		{
			hd_trx a;
			a.trx_id = "trx-id-test";
			a.from_account = from_account;
			a.to_account = to_account;
			a.amount = amount;
			a.asset_symbol = "mbtc";
			a.block_num = 1;
			fc::variant v;
			fc::to_variant(a, v);
			return v.get_object();
		}

		fc::string crosschain_interface_emu::sign_multisig_transaction(fc::variant_object trx, graphene::privatekey_management::crosschain_privatekey_base*& sign_key, const std::string& redeemScript,bool broadcast)

		{
			string a;
			fc::variant v;
			fc::to_variant(a, v);
			return v.as_string();
		}

		fc::variant_object crosschain_interface_emu::merge_multisig_transaction(fc::variant_object &trx, std::vector<std::string> signatures)
		{
			hd_trx a;
			fc::variant v;
			fc::to_variant(a, v);
			return v.get_object();
		}

		bool crosschain_interface_emu::validate_link_trx(const hd_trx &trx)
		{
			return false;
		}

		bool crosschain_interface_emu::validate_link_trx(const std::vector<hd_trx> &trx)
		{
			return false;
		}

		bool crosschain_interface_emu::validate_other_trx(const fc::variant_object &trx)
		{
			return true;
		}
		bool crosschain_interface_emu::validate_address(const std::string& addr)
		{
			return false;
		}
		bool crosschain_interface_emu::validate_signature(const std::string &account, const std::string &content, const std::string &signature)
		{
			return true;
		}

		bool crosschain_interface_emu::create_signature(graphene::privatekey_management::crosschain_privatekey_base*& sign_key, const std::string &content, std::string &signature)
		{
			return false;
		}

		void crosschain_interface_emu::broadcast_transaction(const fc::variant_object& trx)
		{
			auto &idx_by_id = _transactions.get<trx_id>();
			if (idx_by_id.find(trx["trx_id"].as_string()) == idx_by_id.end())
			{
				_transactions.insert(hd_trx{
					trx["trx_id"].as_string(),
					trx["from_account"].as_string(),
					trx["to_account"].as_string(),
					trx["amount"].as_string(),
					trx["asset_symbol"].as_string() });
			}

			_balances[trx["from_account"].as_string()] -= trx["amount"].as_uint64();
			_balances[trx["to_account"].as_string()] += trx["amount"].as_uint64();
		}

		std::vector<fc::variant_object> crosschain_interface_emu::query_account_balance(const std::string &account)
		{
			std::vector<fc::variant_object> ret;
			if (account.empty())
			{
				for (auto &b : _balances)
				{

					ret.push_back(variant_object(b.first, variant(b.second)));
					//ret.push_back(variant(b.second).get_object());
				}
			}
			else
			{
				auto b = _balances.find(account);
				if (b != _balances.end())
				{
					ret.push_back(variant_object(b->first, variant(b->second)));
				}
			}
			return ret;
		}

		std::vector<fc::variant_object> crosschain_interface_emu::transaction_history(std::string symbol,const std::string &user_account, uint32_t start_block, uint32_t limit, uint32_t& end_block_num)
		{
			struct comp_block_num
			{
				// compare an ID and an employee
				bool operator()(int x, const hd_trx& t)const{ return x < t.block_num; }

				// compare an employee and an ID
				bool operator()(const hd_trx& t, int x)const{ return t.block_num < x; }
				// compare an ID and an ID
				bool operator()(const int& t, int x)const{ return t < x; }

			};

			auto &blocks = _transactions.get<block_num>();
			std::cout << "block size is " << blocks.size() << std::endl;
			auto itr = blocks.lower_bound(start_block, comp_block_num());
			std::vector<fc::variant_object> ret;
			for (; itr != blocks.end(); ++itr)
			{
				if (itr->from_account == user_account)
				{
					ret.push_back(fc::variant(*itr).get_object());
				}
			}
			return ret;
		}

		std::string crosschain_interface_emu::export_private_key(std::string &account, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_emu::import_private_key(std::string &account, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_emu::backup_wallet(std::string &wallet_name, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_emu::recover_wallet(std::string &wallet_name, std::string &encrypt_passprase)
		{
			return std::string();
		}



		fc::ecc::private_key crosschain_interface_emu::derive_private_key(const std::string& prefix_string,
			int sequence_number)
		{
			std::string sequence_string = std::to_string(sequence_number);
			fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
			fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
			return derived_key;
		}

		bool crosschain_interface_emu::load_wallet_file(string wallet_filename)
		{
			// TODO:  Merge imported wallet with existing wallet,
			//        instead of replacing it
			if (wallet_filename == "")
				wallet_filename = _plugin_wallet_filepath + _wallet_name;

			if (!fc::exists(wallet_filename))
				return false;

			_wallet = fc::json::from_file(wallet_filename).as< graphene::wallet::wallet_data >();
			if (_wallet.chain_id != _chain_id)
				FC_THROW("Wallet chain ID does not match",
				("wallet.chain_id", _wallet.chain_id)
				("chain_id", _chain_id));

			return true;
		}

		void crosschain_interface_emu::lock()
		{
			try {
				FC_ASSERT(!is_locked());
				encrypt_keys();
				for (auto iter = _keys.begin(); iter != _keys.end(); iter++)
					iter->second = key_to_wif(fc::ecc::private_key());
				_keys.clear();
				_checksum = fc::sha512();
			} FC_CAPTURE_AND_RETHROW()
		}



		bool crosschain_interface_emu::is_locked()
		{
			return _checksum == fc::sha512();
		}
		void crosschain_interface_emu::save_wallet_file(string wallet_filename)
		{
			//
			// Serialize in memory, then save to disk
			//
			// This approach lessens the risk of a partially written wallet
			// if exceptions are thrown in serialization
			//

			encrypt_keys();

			if (wallet_filename == "")
				wallet_filename = _plugin_wallet_filepath + _wallet_name;

			wlog("saving wallet to file ${fn}", ("fn", wallet_filename));

			string data = fc::json::to_pretty_string(_wallet);
			try
			{
				enable_umask_protection();
				//
				// Parentheses on the following declaration fails to compile,
				// due to the Most Vexing Parse.  Thanks, C++
				//
				// http://en.wikipedia.org/wiki/Most_vexing_parse
				//

				std::string test_path = fc::path(wallet_filename).preferred_string();
				fc::ofstream outfile{ fc::path(wallet_filename) };
				outfile.write(data.c_str(), data.length());
				outfile.flush();
				outfile.close();
				disable_umask_protection();
			}
			catch (...)
			{
				disable_umask_protection();
				throw;
			}
		}


		void crosschain_interface_emu::encrypt_keys()
		{
			if (!is_locked())
			{
				graphene::wallet::plain_keys data;
				data.keys = _keys;
				data.checksum = _checksum;
				auto plain_txt = fc::raw::pack(data);
				_wallet.cipher_keys = fc::aes_encrypt(data.checksum, plain_txt);
			}
		}
		void crosschain_interface_emu::enable_umask_protection()
		{
#ifdef __unix__
			_old_umask = umask(S_IRWXG | S_IRWXO);
#endif
		}

		void crosschain_interface_emu::disable_umask_protection()
		{
#ifdef __unix__
			umask(_old_umask);
#endif
		}
		}
			}
