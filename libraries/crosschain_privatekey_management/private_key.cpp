/**
* Author: wengqiang (email: wens.wq@gmail.com  site: qiangweng.site)
*
* Copyright © 2015--2018 . All rights reserved.
*
* File: private_key.cpp
* Date: 2018-01-11
*/

#include <graphene/crosschain_privatekey_management/private_key.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/optional.hpp>
#include <graphene/chain/pts_address.hpp>
#include <bitcoin/bitcoin.hpp>
#include <graphene/crosschain_privatekey_management/util.hpp>
#include <assert.h>


namespace graphene { namespace privatekey_management {


	crosschain_privatekey_base::crosschain_privatekey_base()
	{
		_key = fc::ecc::private_key();
	}

	crosschain_privatekey_base::crosschain_privatekey_base(fc::ecc::private_key& priv_key)
	{
		_key = priv_key;
	}

	fc::ecc::private_key  crosschain_privatekey_base::get_private_key()
	{
		FC_ASSERT(this->_key != fc::ecc::private_key(), "private key is empty!");
		
		return _key;
	}

	std::string  crosschain_privatekey_base::sign_trx(const std::string& script, const std::string& raw_trx,int index)
	{
		//get endorsement
		libbitcoin::endorsement out;
		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
// 		libbitcoin::wallet::ec_private libbitcoin_priv("L5d83SNdFb6EvyZvMDY7zGAhpgZc8hhr57onBo2YxUbdja8PZ7WL");
		//libbitcoin::config::base16  base(script);
		libbitcoin::chain::script   libbitcoin_script;//(libbitcoin::data_chunk(base),true);
		libbitcoin_script.from_string(script);
	
		libbitcoin::chain::transaction  trx;
		trx.from_data(libbitcoin::config::base16(raw_trx));
		uint8_t hash_type = libbitcoin::machine::sighash_algorithm::all;

		auto result = libbitcoin::chain::script::create_endorsement(out, libbitcoin_priv.secret(), libbitcoin_script, trx, index, hash_type);
		assert( result == true);
// 		printf("endorsement is %s\n", libbitcoin::encode_base16(out).c_str());

		//get public hex
		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();
// 		printf("public hex is %s\n", pub_hex.c_str());

		//get signed raw-trx
		std::string endorsment_script = "[" + libbitcoin::encode_base16(out) + "]" + " [" + pub_hex + "] ";
// 		printf("endorsement script is %s\n", endorsment_script.c_str());
		libbitcoin_script.from_string(endorsment_script);

		//trx.from_data(libbitcoin::config::base16(raw_trx));
		trx.inputs()[index].set_script(libbitcoin_script);	    
		std::string signed_trx = libbitcoin::encode_base16(trx.to_data());

// 		printf("signed trx is %s\n", signed_trx.c_str());




		return signed_trx;
	}

	void crosschain_privatekey_base::generate()
	{
		_key = fc::ecc::private_key::generate();
	}

	std::string crosschain_privatekey_base::sign_message(const std::string& msg)
	{


		libbitcoin::wallet::message_signature sign;

		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
// 		libbitcoin::wallet::ec_private libbitcoin_priv("L13gvvM3TtL2EmfBdye8tp4tQhcbCG3xz3VPrBjSZL8MeJavLL8K");
		libbitcoin::data_chunk  data(msg.begin(), msg.end());

		libbitcoin::wallet::sign_message(sign, data, libbitcoin_priv.secret());

		auto result = libbitcoin::encode_base64(sign);
// 		printf("the signed message is %s\n", result.c_str());
		return result;

	}
	

	void btc_privatekey::init()
	{
		set_id(0);
		set_pubkey_prefix(0x0);
		set_privkey_prefix(0x80);
	}



	std::string  btc_privatekey::get_wif_key()
	{	
		FC_ASSERT( is_empty() == false, "private key is empty!" );

		fc::sha256 secret = get_private_key().get_secret();
		//one byte for prefix, one byte for compressed sentinel
		const size_t size_of_data_to_hash = sizeof(secret) + 2;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes];
		data[0] = (char)get_privkey_prefix();
		memcpy(&data[1], (char*)&secret, sizeof(secret));
		data[size_of_data_to_hash - 1] = (char)0x01;
		fc::sha256 digest = fc::sha256::hash(data, size_of_data_to_hash);
		digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));
	
	}

    std::string   btc_privatekey::get_address()
    {
		FC_ASSERT(is_empty() == false, "private key is empty!");

        //configure for bitcoin
        uint8_t version = get_pubkey_prefix();
        bool compress = true;

		const fc::ecc::private_key& priv_key = get_private_key();
        fc::ecc::public_key  pub_key = priv_key.get_public_key();

        graphene::chain::pts_address btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
    }
	std::string btc_privatekey::get_address_by_pubkey(const std::string& pub)
	{
		return graphene::privatekey_management::get_address_by_pubkey(pub, get_pubkey_prefix());
	}
	std::string btc_privatekey::get_public_key()
	{
		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());

		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();

		return pub_hex;

	}

	std::string btc_privatekey::sign_message(const std::string& msg)
	{
		return this->crosschain_privatekey_base::sign_message(msg);
	}
	std::string btc_privatekey::sign_trx(const std::string& script, const std::string& raw_trx,int index)
	{
		return this->crosschain_privatekey_base::sign_trx(script,raw_trx,index);
	}

	fc::optional<fc::ecc::private_key>   btc_privatekey::import_private_key(const std::string& wif_key)
	{
		auto key = graphene::utilities::wif_to_key(wif_key);
		set_key(*key);
		return key;

	}
	std::string btc_privatekey::mutisign_trx( const std::string& redeemscript, const fc::variant_object& raw_trx)
	{
		try {
			FC_ASSERT(raw_trx.contains("hex"));
			FC_ASSERT(raw_trx.contains("trx"));
			auto tx = raw_trx["trx"].get_object();
			auto size = tx["vin"].get_array().size();
			std::string trx = raw_trx["hex"].as_string();
			for (int index = 0; index < size; index++)
			{
				trx = graphene::privatekey_management::mutisign_trx(get_wif_key(),redeemscript,trx,index);
			}
			return trx;
		}FC_CAPTURE_AND_RETHROW((redeemscript)(raw_trx));
		


	}



	void ltc_privatekey::init()
	{
		set_id(0);
		set_pubkey_prefix(0x30);
		set_privkey_prefix(0xB0);
	}

	std::string  ltc_privatekey::get_wif_key()
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

		const fc::ecc::private_key& priv_key = get_private_key();
		const fc::sha256& secret = priv_key.get_secret();

		const size_t size_of_data_to_hash = sizeof(secret) + 2;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes];
		data[0] = (char)get_privkey_prefix();
		memcpy(&data[1], (char*)&secret, sizeof(secret));
		data[size_of_data_to_hash - 1] = (char)0x01;
		fc::sha256 digest = fc::sha256::hash(data, size_of_data_to_hash);
		digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));


	}



	std::string ltc_privatekey::get_address()
	{
		FC_ASSERT(is_empty() == false, "private key is empty!");

		//configure for bitcoin
		uint8_t version = get_pubkey_prefix();
		bool compress = true;

		const fc::ecc::private_key& priv_key = get_private_key();
		fc::ecc::public_key  pub_key = priv_key.get_public_key();

		graphene::chain::pts_address btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
	}

	std::string ltc_privatekey::sign_message(const std::string& msg)
	{
		libbitcoin::wallet::message_signature sign;

		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
		// 		libbitcoin::wallet::ec_private libbitcoin_priv("L13gvvM3TtL2EmfBdye8tp4tQhcbCG3xz3VPrBjSZL8MeJavLL8K");
		libbitcoin::data_chunk  data(msg.begin(), msg.end());
		
		libbitcoin::wallet::sign_message(sign, data, libbitcoin_priv.secret(),true, std::string("Litecoin Signed Message:\n"));

		auto result = libbitcoin::encode_base64(sign);
		// 		printf("the signed message is %s\n", result.c_str());
		return result;
	}


	std::string ltc_privatekey::mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx)
	{
		try {
			FC_ASSERT(raw_trx.contains("hex"));
			FC_ASSERT(raw_trx.contains("trx"));
			auto tx = raw_trx["trx"].get_object();
			auto size = tx["vin"].get_array().size();
			std::string trx = raw_trx["hex"].as_string();
			for (int index = 0; index < size; index++)
			{
				trx = graphene::privatekey_management::mutisign_trx(get_wif_key(), redeemscript, trx, index);
			}
			return trx;
		}FC_CAPTURE_AND_RETHROW((redeemscript)(raw_trx));



	}

	std::string ltc_privatekey::get_public_key()
	{
		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());

		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();

		return pub_hex;

	}
	std::string ltc_privatekey::get_address_by_pubkey(const std::string& pub)
	{
		return graphene::privatekey_management::get_address_by_pubkey(pub, get_pubkey_prefix());
	}

	fc::optional<fc::ecc::private_key> ltc_privatekey::import_private_key(const std::string& wif_key)
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

		auto key = graphene::utilities::wif_to_key(wif_key);
		set_key(*key);
		return key;

	}
	std::string ltc_privatekey::sign_trx(const std::string& script, const std::string& raw_trx,int index)
	{
		return this->crosschain_privatekey_base::sign_trx(script, raw_trx, index);
	}

	void ub_privatekey::init()
	{
		set_id(0);
		set_pubkey_prefix(0x0);
		set_privkey_prefix(0x80);
	}



	std::string  ub_privatekey::get_wif_key()
	{
		FC_ASSERT(is_empty() == false, "private key is empty!");

		fc::sha256 secret = get_private_key().get_secret();
		//one byte for prefix, one byte for compressed sentinel
		const size_t size_of_data_to_hash = sizeof(secret) + 2;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes];
		data[0] = (char)get_privkey_prefix();
		memcpy(&data[1], (char*)&secret, sizeof(secret));
		data[size_of_data_to_hash - 1] = (char)0x01;
		fc::sha256 digest = fc::sha256::hash(data, size_of_data_to_hash);
		digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));

	}

	std::string   ub_privatekey::get_address()
	{
		FC_ASSERT(is_empty() == false, "private key is empty!");

		//configure for bitcoin
		uint8_t version = get_pubkey_prefix();
		bool compress = true;

		const fc::ecc::private_key& priv_key = get_private_key();
		fc::ecc::public_key  pub_key = priv_key.get_public_key();

		graphene::chain::pts_address btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
	}
	std::string ub_privatekey::get_address_by_pubkey(const std::string& pub)
	{
		return graphene::privatekey_management::get_address_by_pubkey(pub, get_pubkey_prefix());
	}
	std::string ub_privatekey::get_public_key()
	{
		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());

		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();

		return pub_hex;

	}

	std::string ub_privatekey::sign_message(const std::string& msg)
	{
		return this->crosschain_privatekey_base::sign_message(msg);
	}


	fc::optional<fc::ecc::private_key>   ub_privatekey::import_private_key(const std::string& wif_key)
	{
		auto key = graphene::utilities::wif_to_key(wif_key);
		set_key(*key);
		return key;

	}
	std::string ub_privatekey::mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx)
	{
		try {
			FC_ASSERT(raw_trx.contains("hex"));
			FC_ASSERT(raw_trx.contains("trx"));
			auto tx = raw_trx["trx"].get_object();
			auto size = tx["vin"].get_array().size();
			std::string trx = raw_trx["hex"].as_string();
			for (int index = 0; index < size; index++)
			{
				trx = graphene::privatekey_management::mutisign_trx(get_wif_key(), redeemscript, trx, index);
			}
			return trx;
		}FC_CAPTURE_AND_RETHROW((redeemscript)(raw_trx));
	}
	std::string ub_privatekey::sign_trx(const std::string& script, const std::string& raw_trx, int index)
	{
		return this->crosschain_privatekey_base::sign_trx(script, raw_trx,index);
	}

	crosschain_management::crosschain_management()
	{
		crosschain_decode.insert(std::make_pair("BTC", &graphene::privatekey_management::btc_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("LTC", &graphene::privatekey_management::ltc_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("UB", &graphene::privatekey_management::ub_privatekey::decoderawtransaction));
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
		else if (name == "UB")
		{
			auto itr = crosschain_prks.insert(std::make_pair(name, new ub_privatekey()));
			return itr.first->second;
		}
		return nullptr;
	}

	fc::variant_object crosschain_management::decoderawtransaction(const std::string& raw_trx, const std::string& symbol)
	{
		try {
			auto iter = crosschain_decode.find(symbol);
			FC_ASSERT(iter != crosschain_decode.end(), "plugin not exist.");
			return iter->second(raw_trx);
		}FC_CAPTURE_AND_RETHROW((raw_trx)(symbol))
	}


} } // end namespace graphene::privatekey_management
