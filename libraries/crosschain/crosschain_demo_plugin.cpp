
#include <graphene/crosschain/crosschain_demo_plugin.hpp>
#include <graphene/wallet/wallet.hpp>

#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/elliptic.hpp>

namespace graphene {
	namespace app {



		std::string _plugin_wallet_filepath;
		std::string _wallet_name;
		graphene::wallet::wallet_data _wallet;
		chain_id_type           _chain_id;
		fc::sha512              _checksum;
		map<address, string> _keys;


		bool crosschain_demo_plugin::create_wallet(string wallet_name, string wallet_passprase)
		{
			if (!fc::exists(_plugin_wallet_filepath+ wallet_name))
				return false;
			_wallet_name = wallet_name;
			_wallet = *make_shared<graphene::wallet::wallet_data>();
			save_wallet_file(_plugin_wallet_filepath + wallet_name);

			
		}
		bool crosschain_demo_plugin::unlock_wallet(string wallet_name, string wallet_passprase, uint32_t duration)
		{
			FC_ASSERT(wallet_passprase.size() > 0);
			auto pw = fc::sha512::hash(wallet_passprase.c_str(), wallet_passprase.size());
			vector<char> decrypted = fc::aes_decrypt(pw, _wallet.cipher_keys);
			auto pk = fc::raw::unpack<graphene::wallet::plain_keys>(decrypted);
			FC_ASSERT(pk.checksum == pw);
			_keys = std::move(pk.keys);
			_checksum = pk.checksum;

		}
		void crosschain_demo_plugin::close_wallet()
		{
			if (!is_locked())
			{
				encrypt_keys();
				for (auto iter = _keys.begin(); iter != _keys.end(); iter++)
					iter->second = key_to_wif(fc::ecc::private_key());
				_keys.clear();
				_checksum = fc::sha512();
			}
			save_wallet_file(_plugin_wallet_filepath+_wallet_name);
		}
		vector<string> crosschain_demo_plugin::wallet_list()
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
		fc::ecc::private_key derive_private_key(const std::string& prefix_string,
			int sequence_number)
		{
			std::string sequence_string = std::to_string(sequence_number);
			fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
			fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
			return derived_key;
		}
		std::string crosschain_demo_plugin::create_normal_account(string account_name)
		{
			auto& idx = _wallet.my_accounts.get<by_name>();
			auto itr = idx.find(account_name);
			FC_ASSERT(itr == idx.end());
			

			auto user_key =  fc::ecc::extended_private_key::generate();
			auto owner_key = derive_private_key(key_to_wif(user_key), 0);
			account_object new_account;
			new_account.addr = address(owner_key.get_public_key());
			new_account.name = account_name;
			_wallet.my_accounts.emplace(new_account);
			_keys.emplace(new_account.addr, key_to_wif(owner_key));
			save_wallet_file(_wallet_name);
			return string(new_account.addr);
		}
		std::string crosschain_demo_plugin::create_multi_sig_account(std::string account_name,std::vector<std::string> addresses)
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
		bool load_wallet_file(string wallet_filename = "")
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

		void lock()
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



		bool is_locked()
		{
			return _checksum == fc::sha512();
		}
		void save_wallet_file(string wallet_filename = "")
		{
			//
			// Serialize in memory, then save to disk
			//
			// This approach lessens the risk of a partially written wallet
			// if exceptions are thrown in serialization
			//

			encrypt_keys();

			if (wallet_filename == "")
				wallet_filename = _plugin_wallet_filepath+_wallet_name;

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


		void encrypt_keys()
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
		void enable_umask_protection()
		{
#ifdef __unix__
			_old_umask = umask(S_IRWXG | S_IRWXO);
#endif
		}

		void disable_umask_protection()
		{
#ifdef __unix__
			umask(_old_umask);
#endif
		}
	}
}