/**
* Author: wengqiang (email: wens.wq@gmail.com  site: qiangweng.site)
*
* Copyright © 2015--2018 . All rights reserved.
*
* File: util.cpp
* Date: 2018-03-19
*/

#include <graphene/crosschain_privatekey_management/util.hpp>
#include <fc/variant.hpp>

namespace graphene {
    namespace privatekey_management {


        std::string get_address_by_pubkey(const std::string& pubkey_hex_str, uint8_t version)
        {
            //get public key
            libbitcoin::wallet::ec_public libbitcoin_pub(pubkey_hex_str);
            FC_ASSERT(libbitcoin_pub != libbitcoin::wallet::ec_public(), "the pubkey hex str is in valid!");

            auto addr = libbitcoin_pub.to_payment_address(version);

            return addr.encoded();

        }

		std::string create_endorsement(const std::string& signer_wif, const std::string& redeemscript_hex, const std::string& raw_trx, int vin_index)
		{
			libbitcoin::wallet::ec_private libbitcoin_priv(signer_wif);
			// 		libbitcoin::wallet::ec_private libbitcoin_priv("L5d83SNdFb6EvyZvMDY7zGAhpgZc8hhr57onBo2YxUbdja8PZ7WL");

			libbitcoin::chain::script   libbitcoin_script;
			libbitcoin_script.from_data(libbitcoin::config::base16(redeemscript_hex), false);
			//             libbitcoin_script.from_string(redeemscript);
			libbitcoin::chain::transaction  trx;
			trx.from_data(libbitcoin::config::base16(raw_trx));
			uint32_t index = vin_index;
			uint8_t hash_type = libbitcoin::machine::sighash_algorithm::all;

			libbitcoin::endorsement out;
			auto result = libbitcoin::chain::script::create_endorsement(out, libbitcoin_priv.secret(), libbitcoin_script, trx, index, hash_type);
			assert(result == true);
			return libbitcoin::encode_base16(out);

		}

		std::string mutisign_trx(const std::string& signer_wif, const std::string& redeemscript_hex, const std::string& raw_trx, int vin_index)
		{

			std::string  endorse = create_endorsement(signer_wif, redeemscript_hex, raw_trx, vin_index);

			//get signed raw-trx
			std::string endorsement_script = "zero ";
			endorsement_script += "[" + endorse + "] ";
			endorsement_script += "[" + redeemscript_hex + "] ";

			//             printf("endorsement script is %s\n", endorsement_script.c_str());

			libbitcoin::chain::script   libbitcoin_script;
		    libbitcoin_script.from_string(endorsement_script);

			libbitcoin::chain::transaction  trx;
			trx.from_data(libbitcoin::config::base16(raw_trx));
			uint32_t index = vin_index;
			trx.inputs()[index].set_script(libbitcoin_script);
			std::string signed_trx = libbitcoin::encode_base16(trx.to_data());

			//             printf("signed trx is %s\n", signed_trx.c_str());

			return signed_trx;



		}

    }
	namespace utxo {
		std::string decoderawtransaction(const std::string& trx)
		{
			std::vector<fc::variant_object> obj;
			libbitcoin::chain::transaction  tx;
		    //tx.from_data(libbitcoin::config::base16(trx));
			//obj.push_back(fc::variant_object("txid", tx.hash));
			//obj.push_back(fc::variant_object("hash",tx.witness_hash_););

			return "";

		}
	}
}

