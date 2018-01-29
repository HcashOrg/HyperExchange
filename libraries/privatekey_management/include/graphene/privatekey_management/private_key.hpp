/**
* Author: wengqiang (email: wens.wq@gmail.com  site: qiangweng.site)
*
* Copyright © 2015--2018 . All rights reserved.
*
* File: private_key.hpp
* Date: 2018-01-11
*/

#pragma once


#include <fc/crypto/elliptic.hpp>
#include <string>

namespace graphene {
	namespace privatekey_management {


		class crosschain_privatekey
		{
		public:
			crosschain_privatekey() {}

		private:
			fc::ecc::private_key  key;

		};

	struct crosschain_privatekey
	{
		std::map<std::string, std::string> keys;
	};

	fc::ecc::private_key  create_private_key();

	std::string  get_btc_wif_key(fc::ecc::private_key& priv_key);
    std::string  get_btc_address(fc::ecc::private_key& priv_key);
	fc::optional<fc::ecc::private_key>  import_btc_private_key(std::string& wif_key);

	std::string  get_ltc_wif_key(fc::ecc::private_key& priv_key);
	std::string  get_ltc_address(fc::ecc::private_key& priv_key);
	fc::optional<fc::ecc::private_key>  import_ltc_private_key(std::string& wif_key);

	bool store_crosschain_privatekey(std::map<std::string, std::string>& store_keys, fc::ecc::private_key& priv_key, std::string& key_type);


	}
} // end namespace graphene::privatekey_management
