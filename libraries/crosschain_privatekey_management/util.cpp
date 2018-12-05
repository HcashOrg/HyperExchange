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
#include <graphene/crosschain_privatekey_management/private_key.hpp>
#include <fc/io/json.hpp>
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

		std::string mutisign_trx(const std::string& endorse, const std::string& redeemscript_hex, const std::string& raw_trx, int vin_index)
		{
			FC_ASSERT(endorse!="");
			//std::string  endorse = create_endorsement(signer_wif, redeemscript_hex, raw_trx, vin_index);

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

		fc::variant_object btc_privatekey::decoderawtransaction(const std::string& trx)
		{
			auto decode = graphene::utxo::decoderawtransaction(trx);
			return fc::json::from_string(decode).get_object();
		}
		fc::variant_object hc_privatekey::decoderawtransaction(const std::string& trx)
		{
			auto decode = graphene::utxo::decoderawtransaction(trx);
			return fc::json::from_string(decode).get_object();
		}
		fc::variant_object ltc_privatekey::decoderawtransaction(const std::string& trx)
		{
			auto decode = graphene::utxo::decoderawtransaction(trx);
			return fc::json::from_string(decode).get_object();
		}
		fc::variant_object ub_privatekey::decoderawtransaction(const std::string& trx)
		{
			auto decode = graphene::utxo::decoderawtransaction(trx);
			return fc::json::from_string(decode).get_object();
		}
    }
	namespace utxo {
		std::string decoderawtransaction(const std::string& trx)
		{
			std::ostringstream obj;
			libbitcoin::chain::transaction  tx;
		    tx.from_data(libbitcoin::config::base16(trx));
			auto hash =tx.hash(true);
			std::reverse(hash.begin(),hash.end());
			obj << "{\"hash\": \"" << libbitcoin::encode_base16(hash) << "\",\"inputs\": {";
			//insert input:
			auto ins = tx.inputs();
			auto int_size = ins.size();
			for (auto index = 0; index < int_size; index++)
			{ 
				if (index > 0)
					obj << ",";
				obj << "\"input\": {";
				auto input = ins.at(index);
				auto previous_output = input.previous_output();
				hash = previous_output.hash();
				std::reverse(hash.begin(),hash.end());
				obj << "\"previous_output\": { \
                       \"hash\": \"" << libbitcoin::encode_base16(hash);
				obj << "\",\"index\": " << previous_output.index();
				obj << "},";

				obj <<"\"script\": \"" << input.script().to_string(libbitcoin::machine::all_rules) << "\",";
				obj << "\"sequence\": " << input.sequence() << "}";
			}
			obj << "},\
                \"lock_time\": " << tx.locktime() << ",\"outputs\": {";

			auto ons = tx.outputs();
			auto out_size = ons.size();
			for (auto index = 0; index < out_size; index++)
			{
				if (index > 0)
					obj << ",";
				auto output = ons.at(index);
				obj << "\"output\": {";
				obj << "\"address\": \"" << output.address() << "\",";
				obj << "\"script\": \"" << output.script().to_string(libbitcoin::machine::all_rules) <<"\",";
				obj << "\"value\": " << output.value() << "}";
			}
			obj << "}}";
			return obj.str();

		}

		static bool recover(libbitcoin::short_hash& out_hash, bool compressed,
			const libbitcoin::ec_signature& compact, uint8_t recovery_id,
			const libbitcoin::hash_digest& message_digest)
		{
			const libbitcoin::recoverable_signature recoverable
			{
				compact,
				recovery_id
			};

			if (compressed)
			{
				libbitcoin::ec_compressed point;
				if (!libbitcoin::recover_public(point, recoverable, message_digest))
					return false;

				out_hash = libbitcoin::bitcoin_short_hash(point);
				return true;
			}

			libbitcoin::ec_uncompressed point;
			if (!recover_public(point, recoverable, message_digest))
				return false;

			out_hash = libbitcoin::bitcoin_short_hash(point);
			return true;
		}

		
		bool verify_message(const std::string addr, const std::string& content, const std::string& encript, const std::string& prefix="Bitcoin Signed Message:\n")
		{
			libbitcoin::wallet::payment_address address(addr);
			libbitcoin::data_chunk out;
			FC_ASSERT( libbitcoin::decode_base64(out, encript) );
			//libbitcoin::wallet::message_signature t_signature;
			auto& t_signature =libbitcoin::to_array<libbitcoin::wallet::message_signature_size>(out);
			const auto magic = t_signature.front();
			const auto compact = libbitcoin::slice<1, libbitcoin::wallet::message_signature_size>(t_signature);

			bool compressed;
			uint8_t recovery_id;
			if (!libbitcoin::wallet::magic_to_recovery_id(recovery_id, compressed, magic))
				return false;

			libbitcoin::short_hash hash;

			libbitcoin::data_chunk msg(content.begin(),content.end());
			const auto message_digest = libbitcoin::wallet::hash_message(msg,prefix);
			return recover(hash, compressed, compact, recovery_id, message_digest) &&
				(hash == address.hash());


		}


		bool validateUtxoTransaction(const std::string& pubkey,const std::string& redeemscript,const std::string& sig)
		{
			libbitcoin::chain::transaction  tx;
			tx.from_data(libbitcoin::config::base16(sig));
			libbitcoin::wallet::ec_public libbitcoin_pub(pubkey);
			FC_ASSERT(libbitcoin_pub != libbitcoin::wallet::ec_public(), "the pubkey hex str is in valid!");
			libbitcoin::data_chunk pubkey_out;
			FC_ASSERT(libbitcoin_pub.to_data(pubkey_out));
			auto ins = tx.inputs();
			auto int_size = ins.size();
			uint8_t hash_type = libbitcoin::machine::sighash_algorithm::all;
			int vin_index = int_size -1;

			for (; vin_index >= 0; vin_index--)
			{
				auto input = tx.inputs().at(vin_index);
				std::string script_str = input.script().to_string(libbitcoin::machine::all_rules);
				auto pos_first = script_str.find('[');
				FC_ASSERT(pos_first != std::string::npos);
				auto pos_end = script_str.find(']');
				FC_ASSERT(pos_end != std::string::npos);
				std::string hex = script_str.assign(script_str, pos_first + 1, pos_end - pos_first-1);
				libbitcoin::endorsement out;
				FC_ASSERT(libbitcoin::decode_base16(out, hex));
				libbitcoin::der_signature der_sig;
				FC_ASSERT(libbitcoin::parse_endorsement(hash_type, der_sig, std::move(out)));
				libbitcoin::ec_signature ec_sig;
				FC_ASSERT(libbitcoin::parse_signature(ec_sig, der_sig, false));
				auto sigest = create_digest(redeemscript, tx, vin_index);
				if (false == libbitcoin::verify_signature(pubkey_out, sigest, ec_sig))
					return false;
			}
			return true;
		}

		libbitcoin::hash_digest create_digest(const std::string& redeemscript, libbitcoin::chain::transaction& trx, int index)
		{
			libbitcoin::chain::script   libbitcoin_script;
			libbitcoin_script.from_data(libbitcoin::config::base16(redeemscript), false);
			uint8_t hash_type = libbitcoin::machine::sighash_algorithm::all;
			return libbitcoin::chain::script::generate_signature_hash(trx, index, libbitcoin_script, hash_type);
		}
	}
}

