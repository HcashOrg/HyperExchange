/**
* Author: wengqiang (email: wens.wq@gmail.com  site: qiangweng.site)
*
* Copyright © 2015--2018 . All rights reserved.
*
* File: util.cpp
* Date: 2018-03-19
*/
#include <graphene/utilities/hash.hpp>
#include "Keccak.hpp"
#include <fc/crypto/base58.hpp>
#include <graphene/crosschain_privatekey_management/util.hpp>
#include <fc/variant.hpp>
#include <graphene/crosschain_privatekey_management/private_key.hpp>
#include <fc/io/json.hpp>
#include <graphene/utilities/string_escape.hpp>
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
			auto decode = graphene::utxo::decoderawtransaction_hc(trx);
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
		
		void input_todata(libbitcoin::istream_reader& source, libbitcoin::ostream_writer& write, libbitcoin::ostream_writer& write_cache) {
			auto InputCount = source.read_size_little_endian();
			write.write_size_little_endian(uint64_t(InputCount));
			write_cache.write_size_little_endian(uint64_t(InputCount));
			//std::cout << InputCount << std::endl;
			for (uint64_t i = 0; i < InputCount; ++i) {
				auto Hash = source.read_hash();
				auto Index = source.read_4_bytes_little_endian();
				auto Tree = source.read_size_little_endian();
				write.write_hash(Hash);
				write.write_4_bytes_little_endian(Index);
				write.write_size_little_endian(Tree);
				write_cache.write_hash(Hash);
				write_cache.write_4_bytes_little_endian(Index);
				write_cache.write_size_little_endian(Tree);
				auto Sequence = source.read_4_bytes_little_endian();
				write.write_4_bytes_little_endian(Sequence);
				write_cache.write_4_bytes_little_endian(Sequence);
			}
		}
		void output_todata(libbitcoin::istream_reader& source, libbitcoin::ostream_writer& write, libbitcoin::ostream_writer& write_cache) {
			auto OutputCount = source.read_size_little_endian();
			write.write_size_little_endian((uint64_t)OutputCount);
			write_cache.write_size_little_endian((uint64_t)OutputCount);
			//std::cout << (uint64_t)OutputCount << std::endl;
			for (uint64_t i = 0; i < OutputCount; ++i) {
				auto Value = source.read_8_bytes_little_endian();
				auto Version = source.read_2_bytes_little_endian();
				auto output_count = source.read_size_little_endian();
				auto PkScript = source.read_bytes(output_count);
				write.write_8_bytes_little_endian(Value);
				write.write_2_bytes_little_endian(Version);
				write.write_size_little_endian(output_count);
				write.write_bytes(PkScript);
				write_cache.write_8_bytes_little_endian(Value);
				write_cache.write_2_bytes_little_endian(Version);
				write_cache.write_size_little_endian(output_count);
				write_cache.write_bytes(PkScript);

				//std::cout << PkScript.size() << "-" << output_count<<std::endl;
			}
		}
		std::string decoderawtransaction_hc(const std::string& trx) {
			std::ostringstream obj;
			libbitcoin::data_chunk input_writ;
			libbitcoin::data_sink input_ostream(input_writ);
			libbitcoin::ostream_writer input_w(input_ostream);
			libbitcoin::data_chunk output_writ;
			libbitcoin::data_sink output_ostream(output_writ);
			libbitcoin::ostream_writer output_w(output_ostream);
			libbitcoin::data_chunk writ;
			libbitcoin::data_sink ostream(writ);
			libbitcoin::ostream_writer w(ostream);
			libbitcoin::data_chunk aa = libbitcoin::config::base16(trx);
			libbitcoin::data_source va(aa);
			libbitcoin::istream_reader source(va);
			//without witness 
			uint32_t versiona = source.read_4_bytes_little_endian();
			uint32_t version_prefix = versiona | (1 << 16);
			uint32_t version_witness = versiona | (3 << 16);
			w.write_4_bytes_little_endian(version_prefix);
			input_todata(source, w, input_w);
			output_todata(source, w, output_w);
			uint32_t locktime = source.read_4_bytes_little_endian();
			uint32_t expir = source.read_4_bytes_little_endian();
			w.write_4_bytes_little_endian(locktime);
			w.write_4_bytes_little_endian(expir);
			ostream.flush();
			input_ostream.flush();
			output_ostream.flush();
			uint256 higest1 = HashR14(writ.begin(), writ.end());
			libbitcoin::hash_digest higest_bitcoin1;
			for (int i = 0; i < higest_bitcoin1.size(); i++)
			{
				higest_bitcoin1.at(i) = *(higest1.begin() + i);
			}
			for(int i = 0; i < higest_bitcoin1.size() / 2; i++ ){
				auto temp = higest_bitcoin1[i];
				higest_bitcoin1[i] = higest_bitcoin1[higest_bitcoin1.size() - 1 - i];
				higest_bitcoin1[higest_bitcoin1.size() - 1 - i] = temp;
			}
			libbitcoin::chain::transaction  tx;
			tx.from_data(libbitcoin::config::base16(trx));
			auto hash = tx.hash(false);
			std::reverse(hash.begin(), hash.end());
			obj << "{\"hash\": \"" << libbitcoin::encode_base16(higest_bitcoin1) << "\",\"inputs\": {";
			
			
			//insert input:
			libbitcoin::data_source input_reader(input_writ);
			libbitcoin::istream_reader input_source(input_reader);
			auto InputCount = input_source.read_size_little_endian();
			auto witness_count = source.read_size_little_endian();
			//std::cout << InputCount << std::endl;
			for (uint64_t i = 0; i < InputCount; ++i) {
				if (i > 0)
					obj << ",";
				obj << "\"input\": {";
				auto Hash = input_source.read_hash();
				for (int i = 0; i < Hash.size() / 2; i++) {
					auto temp = Hash[i];
					Hash[i] = Hash[Hash.size() - 1 - i];
					Hash[Hash.size() - 1 - i] = temp;
				}
				auto Index = input_source.read_4_bytes_little_endian();
				auto Tree = input_source.read_size_little_endian();
				obj << "\"previous_output\": { \
                       \"hash\": \"" << libbitcoin::encode_base16(Hash);
				obj << "\",\"index\": " << Index;
				obj << "},";
				if (i < witness_count){
					auto ValueIn = source.read_8_bytes_little_endian();
					auto BlockHeight = source.read_4_bytes_little_endian();
					auto BlockIndex = source.read_4_bytes_little_endian();
					auto signtureCount = source.read_size_little_endian();
					
					auto SignatureScript = source.read_bytes(signtureCount);
					obj << "\"script\": \"" << libbitcoin::encode_base16(libbitcoin::data_slice(SignatureScript)) << "\",";
					obj << "\"ValueIn\": " << ValueIn << ",";
					
				}
				auto Sequence = input_source.read_4_bytes_little_endian();
				obj << "\"sequence\": " << Sequence << "}";
			}
			obj << "},\
                \"lock_time\": " << locktime << ",\"outputs\": {";
			//insert output:
			libbitcoin::data_source output_reader(output_writ);
			libbitcoin::istream_reader output_source(output_reader);
			auto OutputCount = output_source.read_size_little_endian();
			//std::cout << (uint64_t)OutputCount << std::endl;
			for (uint64_t i = 0; i < OutputCount; ++i) {
				auto Value = output_source.read_8_bytes_little_endian();
				auto Version = output_source.read_2_bytes_little_endian();
				auto output_count = output_source.read_size_little_endian();
				auto PkScript = output_source.read_bytes(output_count);
				libbitcoin::chain::script a(PkScript, false);
				//std::cout << "out put script is " << a.to_string(libbitcoin::machine::all_rules) << std::endl;
				if (i > 0)
					obj << ",";
				obj << "\"output\": {";
				
				if (a.size() == 5)
				{
					
					if (a[0] == libbitcoin::machine::opcode::dup && 
						a[1] == libbitcoin::machine::opcode::hash160 &&
						a[3] == libbitcoin::machine::opcode::equalverify && 
						a[4] == libbitcoin::machine::opcode::checksig) {
						//auto pubkeyhash = a[2];
						fc::array<char, 26> addr;
						//test line
						//int version = 0x0f21;
						int version = 0x097f;
						addr.data[0] = char(version >> 8);
						addr.data[1] = (char)version;
						for (int i = 0; i < a[2].data().size(); i++)
						{
							addr.data[i + 2] = a[2].data()[i];
						}
						//memcpy(addr.data + 2, (char*)&(a[2].data()), a[2].data().size());
						auto check = HashR14(addr.data, addr.data + a[2].data().size() + 2);
						check = HashR14((char*)&check, (char*)&check + sizeof(check));
						memcpy(addr.data + 2 + a[2].data().size(), (char*)&check, 4);
						obj << "\"address\": \"" << fc::to_base58(addr.data, sizeof(addr)) << "\",";
						//std::cout << fc::to_base58(addr.data, sizeof(addr)) << std::endl;
					}
				}
				else if (a.size() == 3)
				{
					if (a[0] == libbitcoin::machine::opcode::hash160 &&
						a[2] == libbitcoin::machine::opcode::equal)
					{
						//auto pubkeyhash = a[1];
						fc::array<char, 26> addr;
						//test line
						//int version = 0x0efc;
						int version = 0x095a;
						addr.data[0] = char(version >> 8);
						addr.data[1] = (char)version;
						for (int i = 0; i < a[1].data().size(); i++)
						{
							addr.data[i + 2] = a[1].data()[i];
						}
						auto check = HashR14(addr.data, addr.data + a[1].data().size() + 2);
						check = HashR14((char*)&check, (char*)&check + sizeof(check));
						memcpy(addr.data + 2 + a[1].data().size(), (char*)&check, 4);
						//std::cout << fc::to_base58(addr.data, sizeof(addr)) << std::endl;
						obj << "\"address\": \"" << fc::to_base58(addr.data, sizeof(addr)) << "\",";
					}
				}
				obj << "\"script\": \"" << a.to_string(libbitcoin::machine::all_rules) << "\",";
				obj << "\"value\": " << Value << "}";
			}
			obj << "}}";
			return obj.str();
		}
		std::string decoderawtransaction(const std::string& trx)
		{
			std::ostringstream obj;
			libbitcoin::chain::transaction  tx;
		    tx.from_data(libbitcoin::config::base16(trx));
			auto hash =tx.hash(true);
			std::reverse(hash.begin(),hash.end());
			obj << "{\"hash\": \"" << libbitcoin::encode_base16(hash) << "\",\"vin\": [";
			//insert input:
			auto ins = tx.inputs();
			auto int_size = ins.size();
			for (auto index = 0; index < int_size; index++)
			{ 
				if (index > 0)
					obj << ",";
				obj << "{";
				auto input = ins.at(index);
				auto previous_output = input.previous_output();
				hash = previous_output.hash();
				std::reverse(hash.begin(),hash.end());
				obj << "\"txid\": \"" << libbitcoin::encode_base16(hash);
				obj << "\",\"vout\": " << previous_output.index();

				obj <<"\"script\": \"" << input.script().to_string(libbitcoin::machine::all_rules) << "\",";
				obj << "\"sequence\": " << input.sequence() << "}";
			}
			obj << "],\
                \"lock_time\": " << tx.locktime() << ",\"vout\": [";

			auto ons = tx.outputs();
			auto out_size = ons.size();
			for (auto index = 0; index < out_size; index++)
			{
				if (index > 0)
					obj << ",";
				auto output = ons.at(index);
				obj << "{";
				obj << "\"scriptPubkey\": {";
				obj << "\"address\": [\"" << output.address() << "\"],";
				obj << "\"script\": \"" << output.script().to_string(libbitcoin::machine::all_rules) <<"\"},";
				obj << "\"value\": " << graphene::utilities::amount_to_string(output.value(),8) << "}";
			}
			obj << "]}";
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
			auto t_signature =libbitcoin::to_array<libbitcoin::wallet::message_signature_size>(out);
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

		std::string combine_trx(const std::vector<std::string>& trxs)
		{
			std::map<int,fc::flat_set<std::string>> signatures;
			libbitcoin::chain::transaction  tx;
			int ins_size;
			std::string redeemscript;
			for (const auto trx : trxs)
			{
				tx.from_data(libbitcoin::config::base16(trx));
				auto ins = tx.inputs();
				ins_size = ins.size();
				int vin_index = ins_size - 1;
				for (; vin_index >= 0; vin_index--)
				{
					auto input = tx.inputs().at(vin_index);
					std::string script_str = input.script().to_string(libbitcoin::machine::all_rules);
					auto pos_first = script_str.find('[');
					FC_ASSERT(pos_first != std::string::npos);
					auto pos_end = script_str.find(']');
					FC_ASSERT(pos_end != std::string::npos);
					
					std::string hex;
					hex.assign(script_str, pos_first + 1, pos_end - pos_first - 1);
					redeemscript.assign(script_str.begin()+pos_end+1,script_str.end());
					signatures[vin_index].insert(hex);
				}
			}

			for (auto index = 0; index < ins_size; index++)
			{
				std::string endorsement_script = "zero ";
				for (const auto& sig : signatures[index])
				{
					endorsement_script += "[" + sig + "] ";
				}
				endorsement_script += redeemscript ;
				libbitcoin::chain::script   libbitcoin_script;
				libbitcoin_script.from_string(endorsement_script);
				tx.inputs()[index].set_script(libbitcoin_script);
			}
			return libbitcoin::encode_base16(tx.to_data());
			
		}



	}
}

