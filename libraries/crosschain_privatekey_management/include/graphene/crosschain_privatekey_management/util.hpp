/**
* Author: wengqiang (email: wens.wq@gmail.com  site: qiangweng.site)
*
* Copyright © 2015--2018 . All rights reserved.
*
* File: util.hpp
* Date: 2018-03-19
*/

#pragma once

#include <graphene/crosschain_privatekey_management/private_key.hpp>
#include <bitcoin/bitcoin.hpp>

#include <assert.h>

namespace graphene {
	namespace privatekey_management {

		std::string get_address_by_pubkey(const std::string& pubkey_hex_str, uint8_t version);

		std::string create_endorsement(const std::string& signer_wif, const std::string& redeemscript, const std::string& raw_trx, int vin_index = 0);

		std::string mutisign_trx(const std::string& endorse, const std::string& redeemscript, const std::string& raw_trx, int vin_index = 0);

	}


}

namespace graphene {
	namespace utxo {
		libbitcoin::hash_digest create_digest(const std::string& redeemscript, libbitcoin::chain::transaction& trx,int index);
		std::string decoderawtransaction(const std::string& trx);
		bool validateUtxoTransaction(const std::string& addr, const std::string& redeemscript, const std::string& sig);
		bool verify_message(const std::string addr, const std::string& content, const std::string& encript,const std::string& prefix);
		std::string combine_trx(const std::vector<std::string>& trxs);
	}
}


