#include <graphene/crosschain/crosschain_interface_emu.hpp>


namespace graphene {
	namespace crosschain {





		void crosschain_interface_emu::initialize_config(fc::variant_object &json_config)
		{
			config = json_config;
		}

		bool crosschain_interface_emu::create_wallet(std::string wallet_name, std::string wallet_passprase) {
			if (!fc::exists(_plugin_wallet_filepath + wallet_name))
				return false;
			_wallet_name = wallet_name;
			_wallet = *make_shared<graphene::wallet::wallet_data>();
			save_wallet_file(_plugin_wallet_filepath + wallet_name);
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

		std::string crosschain_interface_emu::create_normal_account(std::string account_name)
		{
			auto& idx = _wallet.my_accounts.get<by_name>();
			auto itr = idx.find(account_name);
			FC_ASSERT(itr == idx.end());


			auto user_key = fc::ecc::extended_private_key::generate();
			auto owner_key = derive_private_key(key_to_wif(user_key), 0);
			account_object new_account;
			new_account.addr = address(owner_key.get_public_key());
			new_account.name = account_name;
			_wallet.my_accounts.emplace(new_account);
			_keys.emplace(new_account.addr, key_to_wif(owner_key));
			save_wallet_file(_wallet_name);
			return string(new_account.addr);
		}

		std::string crosschain_interface_emu::create_multi_sig_account(std::string account_name, std::vector<std::string> addresses,uint32_t nrequired)
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
			return string(new_account.addr);
		}

		std::vector<fc::variant_object> crosschain_interface_emu::deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit)
		{
			std::vector<fc::variant_object> results;
			//TODo add rpc get function
			hd_trx a;
			a.trx_id = "trx-id-test";
			a.from_account.push_back(user_account);
			a.to_account.push_back("to_account");
			a.amount = "10";
			a.fee = "1";
			a.memo = "Falcom";
			a.asset_sympol = "mbtc";
			a.block_num = 1;
			fc::variant v;
			fc::to_variant(a, v);
			//
			results.emplace_back(v.get_object());
			return results;
			//return std::vector<fc::variant_object>();
		}

		fc::variant_object crosschain_interface_emu::transaction_query(std::string trx_id)
		{
			//TODo add rpc get function
			hd_trx a;
			a.trx_id = trx_id;
			a.from_account.push_back("from_account");
			a.to_account.push_back("to_account");
			a.amount = "10";
			a.fee = "1";
			a.memo = "Falcom";
			a.asset_sympol = "mbtc";
			a.block_num = 1;
			fc::variant v;
			fc::to_variant(a, v);
			return v.get_object();
		}

		fc::variant_object crosschain_interface_emu::transfer(std::string &from_account, std::string &to_account, std::string &amount, std::string &symbol, std::string &memo, bool broadcast)
		{
			//TODo add rpc get function
			hd_trx a;
			a.trx_id = "trx-id-test";
			a.from_account.push_back(from_account);
			a.to_account.push_back(to_account);
			a.amount = amount;
			a.fee = "1";
			a.memo = "Falcom";
			a.asset_sympol = memo;
			a.block_num = 1;
			fc::variant v;
			fc::to_variant(a, v);
			return v.get_object();
		}

		fc::variant_object crosschain_interface_emu::create_multisig_transaction(std::string &from_account, std::string &to_account, std::string &amount, std::string &symbol, std::string &memo, bool broadcast)
		{
			hd_trx a;
			a.trx_id = "trx-id-test";
			a.from_account.push_back(from_account);
			a.to_account.push_back(to_account);
			a.amount = amount;
			a.fee = "0";
			a.memo = "Falcom";
			a.asset_sympol = "mbtc";
			a.block_num = 1;
			fc::variant v;
			fc::to_variant(a, v);
			return v.get_object();
		}

		fc::variant_object crosschain_interface_emu::sign_multisig_transaction(fc::variant_object trx, std::string &sign_account, bool broadcast)
		{
			hd_trx a;
			fc::variant v;
			fc::to_variant(a, v);
			return v.get_object();
		}

		fc::variant_object crosschain_interface_emu::merge_multisig_transaction(fc::variant_object trx, std::vector<fc::variant_object> signatures)
		{
			hd_trx a;
			fc::variant v;
			fc::to_variant(a, v);
			return v.get_object();
		}

		bool crosschain_interface_emu::validate_transaction(fc::variant_object trx)
		{
			return false;
		}

		void crosschain_interface_emu::broadcast_transaction(fc::variant_object trx)
		{
			auto &idx_by_id = transactions.get<1>();
			if (idx_by_id.find(trx["trx_id"].as_string()) == idx_by_id.end())
			{
				transactions.insert(transaction_emu{ trx["trx_id"].as_string(), trx["from_addr"].as_string(), trx["to_addr"].as_string(), trx["block_num"].as_int64(), trx["amount"].as_int64() });
			}
		}

		std::vector<fc::variant_object> crosschain_interface_emu::query_account_balance(std::string &account)
		{
			std::vector<fc::variant_object> ret;
			if (!account.empty())
			{
				for (auto &b : _balances)
				{
					ret.push_back(variant(b.second).get_object());
				}
			}
			else
			{
				auto b = _balances.find(address(account));
				if (b != _balances.end())
				{
					ret.push_back(variant(b->second).get_object());
				}
			}
			return ret;
		}

		std::vector<fc::variant_object> crosschain_interface_emu::transaction_history(std::string &user_account, uint32_t start_block, uint32_t limit)
		{

			return std::vector<fc::variant_object>();
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
