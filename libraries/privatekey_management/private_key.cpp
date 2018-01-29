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

	fc::ecc::private_key  create_private_key()
	{
		return fc::ecc::private_key::generate();
	}

	std::string  get_btc_wif_key(fc::ecc::private_key& priv_key)
	{
		
		return  graphene::utilities::key_to_wif(priv_key);
	}

    std::string  get_btc_address(fc::ecc::private_key& priv_key)
    {
        //configure for bitcoin
        uint8_t version = 0;
        bool compress = false;


        fc::ecc::public_key  pub_key = priv_key.get_public_key();

        graphene::chain::pts_address btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
    }

	fc::optional<fc::ecc::private_key>  import_btc_private_key(std::string& wif_key)
	{
		return graphene::utilities::wif_to_key(wif_key);

	}

	std::string  get_ltc_wif_key(fc::ecc::private_key& priv_key)
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

		fc::sha256& secret = priv_key.get_secret();

		const size_t size_of_data_to_hash = sizeof(secret) + 1;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes];
		data[0] = (char)0xB0;
		memcpy(&data[1], (char*)&secret, sizeof(secret));
		fc::sha256 digest = fc::sha256::hash(data, size_of_data_to_hash);
		digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));


	}



	std::string  get_ltc_address(fc::ecc::private_key& priv_key)
	{
		//configure for bitcoin
		uint8_t version = 48;
		bool compress = false;


		fc::ecc::public_key  pub_key = priv_key.get_public_key();

		graphene::chain::pts_address btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
	}

	fc::optional<fc::ecc::private_key>  import_ltc_private_key(std::string& wif_key)
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


	bool store_crosschain_privatekey(std::map<std::string, std::string>& store_keys, fc::ecc::private_key& priv_key, std::string& key_type)
	{
		std::string addr, wif_key;

		if (key_type == "btc")
		{
			addr = get_btc_address(priv_key);
			wif_key = get_btc_wif_key(priv_key);

		}
		else if (key_type == "ltc")
		{
			addr = get_ltc_address(priv_key);
			wif_key = get_ltc_wif_key(priv_key);
		}

		// judge if the private key has been stored
		if (store_keys.find(addr) == store_keys.end())
		{
			store_keys[addr] = wif_key;
			return true;
		}
		else
		{

			printf("Attention: this private key has been stored!");
			return false;
			
		}
		
		

	}





} } // end namespace graphene::privatekey_management
