/**
* Author: wengqiang (email: wens.wq@gmail.com  site: qiangweng.site)
*
* Copyright © 2015--2018 . All rights reserved.
*
* File: private_key.cpp
* Date: 2018-01-11
*/

#include <graphene/privatekey_management/private_key.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/optional.hpp>
#include <graphene/chain/pts_address.hpp>

#include <assert.h>


namespace graphene { namespace privatekey_management {


	fc::ecc::private_key  crosschain_privatekey_base::get_private_key()
	{
		if (this->_key != fc::ecc::private_key())
			return _key;

		_key =  fc::ecc::private_key::generate();
		return _key;
	}

	std::string crosschain_privatekey_base::exec(const char* cmd)
	{
		std::array<char, COMMAND_BUF> buffer;
		std::string result;
	#if defined(_WIN32)
		std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
	#elif defined(__linux__)
		std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
	#endif
		if (!pipe) throw std::runtime_error("popen() failed!");
		while (!feof(pipe.get())) 
		{
			if (fgets(buffer.data(), COMMAND_BUF, pipe.get()) != nullptr)
				result += buffer.data();
		}

		result.erase(result.end() - 1);
		return result;
	}


	std::string  crosschain_privatekey_base::sign_trx(const std::string& script, const std::string& raw_trx)
	{
		//GET private key by address, todo
		std::string priv_hex = _key.get_secret().str().c_str();

		//get endorsement
		std::string cmd = "\input-sign";
		cmd += " " + priv_hex + " " + script + " " + raw_trx;
		auto endorsement = exec(cmd.c_str());
// 		printf("endorsement: %s\n", endorsement.c_str());

		//get public hex
		cmd = "E:\\blocklink_project\\blocklink-core\\libraries\\privatekey_management\\pm.exe ec-to-public";
		cmd += " " + priv_hex;
		auto pub_hex = exec(cmd.c_str());
// 		printf("pub_hex: %s\n", pub_hex.c_str());

		//get signed raw-trx
		cmd = "E:\\blocklink_project\\blocklink-core\\libraries\\privatekey_management\\pm.exe input-set ";
		cmd += "\"[" + endorsement + "]" + " [" + pub_hex + "]\" " + raw_trx;
// 		printf("command str: %s\n", cmd.c_str());
		auto signed_raw_trx = exec(cmd.c_str());

		printf("signed_raw_trx str: %s\n", signed_raw_trx.c_str());
		return signed_raw_trx;
	}

	std::string crosschain_privatekey_base::sign_message(const std::string& msg)
	{
		auto wif = get_wif_key(_key);

		//sign msg
		std::string cmd = "E:\\blocklink_project\\blocklink-core\\libraries\\privatekey_management\\pm.exe message-sign";
		cmd += " " + wif + " " + "\"" + msg + "\"";
// 		printf("cmd string is %s\n", cmd.c_str());
		auto signedmsg = exec(cmd.c_str());
		printf("signed message: %s\n", signedmsg.c_str());

		return signedmsg;

	}


	btc_privatekey::btc_privatekey()
	{
		set_id(0);
		set_pubkey_prefix(0x0);
		set_privkey_prefix(0x80);
	}


	std::string  btc_privatekey::get_wif_key(fc::ecc::private_key& priv_key)
	{	
		FC_ASSERT( is_empty() == false, "private key is empty!" );
		return  graphene::utilities::key_to_wif(priv_key);
	}

    std::string   btc_privatekey::get_address(fc::ecc::private_key& priv_key)
    {
		FC_ASSERT(is_empty() == false, "private key is empty!");

        //configure for bitcoin
        uint8_t version = get_pubkey_prefix();
        bool compress = false;


        fc::ecc::public_key  pub_key = priv_key.get_public_key();

        graphene::chain::pts_address btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
    }

	fc::optional<fc::ecc::private_key>   btc_privatekey::import_private_key(std::string& wif_key)
	{
		return graphene::utilities::wif_to_key(wif_key);

	}

	

	ltc_privatekey::ltc_privatekey()
	{
		set_id(0);
		set_pubkey_prefix(0x30);
		set_privkey_prefix(0xB0);
	}

	std::string  ltc_privatekey::get_wif_key(fc::ecc::private_key& priv_key)
	{
		/*fc::sha256& secret = priv_key.get_secret();

		const size_t size_of_data_to_hash = sizeof(secret) + 1 ;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes + 1];
		data[0] = (char)0xB0;
		memcpy(&data[1], (char*)&secret, sizeof(secret));

		// add compressed byte
		char value = (char)0x01;
		memcpy(data + size_of_data_to_hash, (char *)&value, 1);
		fc::sha256 digest = fc::sha256::hash(data, size_of_data_to_hash);
		digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash + 1, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));*/

		FC_ASSERT(is_empty() == false, "private key is empty!");

		const fc::sha256& secret = priv_key.get_secret();

		const size_t size_of_data_to_hash = sizeof(secret) + 1;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes];
		data[0] = (char)get_privkey_prefix();
		memcpy(&data[1], (char*)&secret, sizeof(secret));
		fc::sha256 digest = fc::sha256::hash(data, size_of_data_to_hash);
		digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));


	}





	std::string ltc_privatekey::get_address(fc::ecc::private_key& priv_key)
	{
		FC_ASSERT(is_empty() == false, "private key is empty!");

		//configure for bitcoin
		uint8_t version = get_pubkey_prefix();
		bool compress = false;


		fc::ecc::public_key  pub_key = priv_key.get_public_key();

		graphene::chain::pts_address btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
	}

	fc::optional<fc::ecc::private_key> ltc_privatekey::import_private_key(std::string& wif_key)
	{
/*
		std::vector<char> wif_bytes;
		try
		{
			wif_bytes = fc::from_base58(wif_key);
		}
		catch (const fc::parse_error_exception&)
		{
			return fc::optional<fc::ecc::private_key>();
		}
		if (wif_bytes.size() < 5)
			return fc::optional<fc::ecc::private_key>();

		printf("the size is  %d\n", wif_bytes.size());

		std::vector<char> key_bytes(wif_bytes.begin() + 1, wif_bytes.end() - 5);

		fc::ecc::private_key key = fc::variant(key_bytes).as<fc::ecc::private_key>();
		fc::sha256 check = fc::sha256::hash(wif_bytes.data(), wif_bytes.size() - 5);
		fc::sha256 check2 = fc::sha256::hash(check);

		if (memcmp((char*)&check, wif_bytes.data() + wif_bytes.size() - 4, 4) == 0 ||
			memcmp((char*)&check2, wif_bytes.data() + wif_bytes.size() - 4, 4) == 0)
			return key;

		return fc::optional<fc::ecc::private_key>();*/

		return graphene::utilities::wif_to_key(wif_key);

	}

	crosschain_privatekey_base * crosschain_management::get_crosschain_prk(const std::string& name)
	{
		auto itr = crosschain_prks.find(name);
		if (itr != crosschain_prks.end())
		{
			return itr->second;
		}

		if (name == "BTC")
		{
			auto itr = crosschain_prks.insert(std::make_pair(name, new btc_privatekey()));
			return itr.first->second;
		}
		else if (name == "LTC")
		{
			auto itr = crosschain_prks.insert(std::make_pair(name, new ltc_privatekey()));
			return itr.first->second;
		}
	}


} } // end namespace graphene::privatekey_management
