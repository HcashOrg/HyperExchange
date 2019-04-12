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
#include "bytomlib.hpp"
#include <SHA3IUF/sha3.h>
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
			auto decode = graphene::utxo::decoderawtransaction(trx, btc_pubkey, btc_script);
			return fc::json::from_string(decode).get_object();
		}
		fc::variant_object hc_privatekey::decoderawtransaction(const std::string& trx)
		{
			auto decode = graphene::utxo::decoderawtransaction_hc(trx);
			return fc::json::from_string(decode).get_object();
		}
		fc::variant_object ltc_privatekey::decoderawtransaction(const std::string& trx)
		{
			auto decode = graphene::utxo::decoderawtransaction(trx,ltc_pubkey,ltc_script);
			return fc::json::from_string(decode).get_object();
		}
		fc::variant_object ub_privatekey::decoderawtransaction(const std::string& trx)
		{
			auto decode = graphene::utxo::decoderawtransaction(trx,btc_pubkey,btc_script);
			return fc::json::from_string(decode).get_object();
		}
		fc::variant_object btm_privatekey::decoderawtransaction(const std::string& trx)
		{
			auto decode = graphene::utxo::decoderawtransaction_btm(trx);
			return fc::json::from_string(decode).get_object();
		}
    }
	namespace util_for_turn {
		std::string StringToHex(const std::string& data)
		{
			const std::string hex = "0123456789abcdef";
			std::stringstream ss;
			for (std::string::size_type i = 0; i < data.size(); ++i)
				ss << hex[(unsigned char)data[i] >> 4] << hex[(unsigned char)data[i] & 0xf];
			//std::cout << ss.str() << std::endl;
			return ss.str();
		}
		std::string HexToStr(const std::string& str)
		{
			std::string result;
			for (size_t i = 0; i < str.length(); i += 2)
			{
				std::string byte = str.substr(i, 2);
				char chr = (char)(int)strtol(byte.c_str(), NULL, 16);
				result.push_back(chr);
			}
			return result;
		}
		bool  from_hex(const char *pSrc, std::vector<char> &pDst, unsigned int nSrcLength, unsigned int &nDstLength)
		{
			if (pSrc == 0)
			{
				return false;
			}

			nDstLength = 0;

			if (pSrc[0] == 0) // nothing to convert  
				return 0;

			// 计算需要转换的字节数  
			for (int j = 0; pSrc[j]; j++)
			{
				if (isxdigit(pSrc[j]))
					nDstLength++;
			}

			// 判断待转换字节数是否为奇数，然后加一  
			if (nDstLength & 0x01) nDstLength++;
			nDstLength /= 2;

			if (nDstLength > nSrcLength)
				return false;

			nDstLength = 0;

			int phase = 0;
			char temp_char;

			for (int i = 0; pSrc[i]; i++)
			{
				if (!isxdigit(pSrc[i]))
					continue;

				unsigned char val = pSrc[i] - (isdigit(pSrc[i]) ? 0x30 : (isupper(pSrc[i]) ? 0x37 : 0x57));

				if (phase == 0)
				{
					temp_char = val << 4;
					phase++;
				}
				else
				{
					temp_char |= val;
					phase = 0;
					pDst.push_back(temp_char);
					nDstLength++;
				}
			}

			return true;
		}
		std::string BinToHex(const std::vector<char> &strBin, bool bIsUpper)
		{
			std::string strHex;
			strHex.resize(strBin.size() * 2);
			for (size_t i = 0; i < strBin.size(); i++)
			{
				uint8_t cTemp = strBin[i];
				for (size_t j = 0; j < 2; j++)
				{
					uint8_t cCur = (cTemp & 0x0f);
					if (cCur < 10)
					{
						cCur += '0';
					}
					else
					{
						cCur += ((bIsUpper ? 'A' : 'a') - 10);
					}
					strHex[2 * i + 1 - j] = cCur;
					cTemp >>= 4;
				}
			}

			return strHex;
		}
		uint64_t ReadUvarint(libbitcoin::istream_reader& r) {
			uint64_t x = 0;
			uint32_t s = 0;
			for (int i = 0; ; i++) {
				auto b = r.read_byte();
				if (b < (uint8_t)0x80) {
					if (i > 9 || i == 9 && b > 1) {
						return x;
					}
					return x | uint64_t(b) << s;
				}
				x |= uint64_t(b & 0x7f) << s;
				s += 7;
			}
		}
		std::vector<char> CalcSha3Hash(std::string type, libbitcoin::data_chunk hash_msg) {
			//std::cout << "Sha3 Source is " << BinToHex(std::vector<char>(hash_msg.begin(), hash_msg.end()), false) << std::endl;
			sha3_context c_msg_hash;
			sha3_SetFlags(&c_msg_hash, SHA3_FLAGS_KECCAK);
			sha3_Init256(&c_msg_hash);
			sha3_Update(&c_msg_hash, hash_msg.data(), hash_msg.size());
			auto msg_hash = sha3_Finalize(&c_msg_hash);
			std::vector<char> msg_hash_for_show;
			for (int i = 0; i < 32; i++)
			{
				msg_hash_for_show.push_back(((uint8_t*)msg_hash)[i]);
			}
			//std::cout << "Sha3 Msg is " << BinToHex(msg_hash_for_show, false) << std::endl;
			std::vector<char> msg_hash_vector;
			unsigned int nDeplength = 0;
			bool b_converse = from_hex(StringToHex("entryid:" + type + ":").data(), msg_hash_vector, StringToHex("entryid:" + type + ":").size(), nDeplength);
			sha3_context c_final_hash;
			sha3_SetFlags(&c_final_hash, SHA3_FLAGS_KECCAK);
			sha3_Init256(&c_final_hash);
			sha3_Update(&c_final_hash, msg_hash_vector.data(), msg_hash_vector.size());
			sha3_Update(&c_final_hash, msg_hash_for_show.data(), msg_hash_for_show.size());
			auto id_hash = sha3_Finalize(&c_final_hash);
			std::vector<char> ret;
			for (int i = 0; i < 32; i++)
			{
				ret.push_back(((uint8_t*)id_hash)[i]);
			}
			//std::cout << "Sha3 ID is " << BinToHex(ret, false) << std::endl;
			return ret;
		}
		void WriteUvarint(libbitcoin::ostream_writer& w, uint64_t x) {
			uint8_t buf[9];
			int length_real = 0;
			for (; x >= 0x80;) {
				buf[length_real] = uint8_t(x) | 0x80;
				x >>= 7;
				length_real++;
			}
			buf[length_real] = uint8_t(x);
			for (int i = 0; i < length_real + 1; i++) {
				w.write_byte(buf[i]);
			}
		}
	}
	namespace utxo {
		
		void input_todata(libbitcoin::istream_reader& source, libbitcoin::ostream_writer& write, libbitcoin::ostream_writer& write_cache) {
			auto InputCount = source.read_size_little_endian();
			write.write_size_little_endian(uint64_t(InputCount));
			write_cache.write_size_little_endian(uint64_t(InputCount));
			////std::cout << InputCount << std::endl;
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
			obj << "{\"txid\": \"" << libbitcoin::encode_base16(higest_bitcoin1) << "\",\"vin\": [";
			
			
			//insert input:
			libbitcoin::data_source input_reader(input_writ);
			libbitcoin::istream_reader input_source(input_reader);
			auto InputCount = input_source.read_size_little_endian();
			auto witness_count = source.read_size_little_endian();
			//std::cout << InputCount << std::endl;
			for (uint64_t i = 0; i < InputCount; ++i) {
				if (i > 0)
					obj << ",";
				obj << "{";
				auto Hash = input_source.read_hash();
				for (int i = 0; i < Hash.size() / 2; i++) {
					auto temp = Hash[i];
					Hash[i] = Hash[Hash.size() - 1 - i];
					Hash[Hash.size() - 1 - i] = temp;
				}
				auto Index = input_source.read_4_bytes_little_endian();
				auto Tree = input_source.read_size_little_endian();
				obj <<"\"txid\": \"" << libbitcoin::encode_base16(Hash);
				obj << "\",\"vout\": " << Index <<",";
				if (i < witness_count){
					auto ValueIn = source.read_8_bytes_little_endian();
					auto BlockHeight = source.read_4_bytes_little_endian();
					auto BlockIndex = source.read_4_bytes_little_endian();
					auto signtureCount = source.read_size_little_endian();
					
					auto SignatureScript = source.read_bytes(signtureCount);
					obj << "\"scriptSig\": \"" << libbitcoin::encode_base16(libbitcoin::data_slice(SignatureScript)) << "\",";
					obj << "\"amountin\": " << ValueIn << ",";
					
				}
				auto Sequence = input_source.read_4_bytes_little_endian();
				obj << "\"sequence\": " << Sequence << "}";
			}
			obj << "],\
                \"lock_time\": " << locktime << ",\"vout\": [";
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
				obj << "{";
				obj << "\"scriptPubKey\": {"<<"\"script\": "<<"\"" << a.to_string(libbitcoin::machine::all_rules) << "\",";
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
						int version = hc_pubkey;
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
						obj << "\"addresses\": [\"" << fc::to_base58(addr.data, sizeof(addr)) << "\"]";
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
						int version = hc_script;
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
						obj << "\"addresses\": [\"" << fc::to_base58(addr.data, sizeof(addr)) << "\"]";
					}
				}
				obj << "},";
				obj << "\"value\": " << Value << "}";
			}
			obj << "]}";
			return obj.str();
		}
		std::string decoderawtransaction_btm(const std::string& trx) {
			std::ostringstream obj;
			uint64_t all_input = 0;
			uint64_t all_output = 0;
			libbitcoin::data_chunk writ;
			libbitcoin::data_sink ostream(writ);
			libbitcoin::ostream_writer w(ostream);
			std::vector<char> temp;
			unsigned int nDeplength = 0;
			bool b_converse = util_for_turn::from_hex(trx.data(), temp, trx.size(), nDeplength);
			libbitcoin::data_chunk aa(temp.begin(), temp.end());
			libbitcoin::data_source va(aa);
			libbitcoin::istream_reader source(va);
			auto serflags = source.read_byte();
			auto version = util_for_turn::ReadUvarint(source);
			auto TimeRange = util_for_turn::ReadUvarint(source);
			w.write_8_bytes_little_endian(version);
			w.write_8_bytes_little_endian(TimeRange);
			auto InputCount = util_for_turn::ReadUvarint(source);
			libbitcoin::data_chunk writ_mux;
			libbitcoin::data_sink ostream_mux(writ_mux);
			libbitcoin::ostream_writer w_mux(ostream_mux);
			uint64_t mux_size = 0;
			std::vector<libbitcoin::hash_digest> vec_spend_ids;
			obj << "{";
			obj << "\"version\":" << version << ",";
			obj << "\"time_range\":" << TimeRange << ",";
			
			for (int i = 0; i < InputCount; i++)
			{
				if (i == 0){
					obj << "\"inputs\":[{";
				}
				libbitcoin::hash_digest spend_id_bytearr;
				auto AssetVersion = util_for_turn::ReadUvarint(source);
				//CommitmentSuffix ReadExtensibleString
				auto CommitmentSuffix_length = util_for_turn::ReadUvarint(source);
				auto CommitmentSuffix_msg = source.read_bytes(CommitmentSuffix_length);
				libbitcoin::data_chunk vaa(CommitmentSuffix_msg.begin(), CommitmentSuffix_msg.end());
				libbitcoin::data_source va(vaa);
				libbitcoin::istream_reader commit_suffix_source(va);
				auto icType = commit_suffix_source.read_byte();
				if (icType == (uint8_t)1)
				{
					obj << "\"type\":\"" << "spend" << "\",";
					auto solo_input_length = util_for_turn::ReadUvarint(commit_suffix_source);
					libbitcoin::data_chunk writ_input;
					libbitcoin::data_sink ostream_input(writ_input);
					libbitcoin::ostream_writer w_input(ostream_input);
					auto sourceId = commit_suffix_source.read_hash();
					w_input.write_hash(sourceId);
					auto assetID = commit_suffix_source.read_hash();
					obj << "\"assetID\":\"" << util_for_turn::BinToHex(std::vector<char>(assetID.begin(), assetID.end()),false) << "\",";
					w_input.write_hash(assetID);
					auto amount = util_for_turn::ReadUvarint(commit_suffix_source);
					all_input += amount;
					obj << "\"amount\":" << amount << ",";
					w_input.write_8_bytes_little_endian(amount);
					auto SourcePosition = util_for_turn::ReadUvarint(commit_suffix_source);
					w_input.write_8_bytes_little_endian(SourcePosition);
					auto VMVersion = util_for_turn::ReadUvarint(commit_suffix_source);
					w_input.write_8_bytes_little_endian(VMVersion);
					auto a = commit_suffix_source.read_bytes();
					libbitcoin::data_chunk ctrol_prog(a.begin(), a.end());
					libbitcoin::data_source ctrol_prog_source(ctrol_prog);
					libbitcoin::istream_reader ctrol_prog_reader(ctrol_prog_source);
					auto ctrl_prog_length = util_for_turn::ReadUvarint(ctrol_prog_reader);
					auto ctrl_prog = ctrol_prog_reader.read_bytes(ctrl_prog_length);
					obj << "\"control_program\":\"" << util_for_turn::BinToHex(std::vector<char>(ctrl_prog.begin(), ctrl_prog.end()), false) << "\",";
					std::string btm_prifix = "mainnet";
					if (btm_script == 0x00) {
						btm_prifix = "mainnet";
					}
					else if (btm_script == 0x01) {
						btm_prifix = "wisdom";
					}
					else if (btm_script == 0x02) {
						btm_prifix = "solonet";
					}
					obj << "\"address\":\"" << bytom::GetBytomAddressFromHash(util_for_turn::BinToHex(std::vector<char>(ctrl_prog.begin()+2, ctrl_prog.end()), false), btm_prifix) << "\",";
					w_input.write_bytes(a);
					ostream_input.flush();
					auto prev_out_id = util_for_turn::CalcSha3Hash("output1", writ_input);
					auto spend_id = util_for_turn::CalcSha3Hash("spend1", libbitcoin::data_chunk(prev_out_id.begin(), prev_out_id.end()));
					obj << "\"spent_output_id\":\"" << util_for_turn::BinToHex(prev_out_id, false) << "\",";
					obj << "\"input_id\":\"" << util_for_turn::BinToHex(spend_id, false) << "\",";
					for (int i = 0; i < spend_id.size(); i++)
					{
						spend_id_bytearr[i] = spend_id[i];
					}
					vec_spend_ids.push_back(spend_id_bytearr);
					w_mux.write_hash(spend_id_bytearr);
					w_mux.write_hash(assetID);
					w_mux.write_8_bytes_little_endian(amount);
					w_mux.write_8_bytes_little_endian(uint64_t(0));
					mux_size += 1;
				}
				obj << "\"witness_arguments\":[" ;
				//WitnessSuffix ReadExtensibleString
				auto WitnessSuffix_length = util_for_turn::ReadUvarint(source);
				auto WitnessSuffix_msg = source.read_bytes(WitnessSuffix_length);
				libbitcoin::data_chunk sign_msg_chunk;
				libbitcoin::data_sink sign_msg_sink(sign_msg_chunk);
				libbitcoin::ostream_writer write_sign_msg(sign_msg_sink);
				if (WitnessSuffix_length != 1)
				{
					obj << "\"";
					libbitcoin::data_chunk witness_data_chunk(WitnessSuffix_msg.begin(), WitnessSuffix_msg.end());
					libbitcoin::data_source witness_data_source(witness_data_chunk);
					libbitcoin::istream_reader witnessreader_source(witness_data_source);
					libbitcoin::data_chunk script;
					auto length_witness_array = util_for_turn::ReadUvarint(witnessreader_source);
					for (int i = 0; i < length_witness_array; i++)
					{
						auto length = util_for_turn::ReadUvarint(witnessreader_source);
						auto str = witnessreader_source.read_bytes(length);
						obj << util_for_turn::BinToHex(std::vector<char>(str.begin(), str.end()), false);
						
						if (i == length_witness_array - 1) {
							script = str;
							obj << "\"]";
						}
						else {
							obj << "\",\"";
						}
					}
				}
				else {
					obj << "]";
				}
				obj << "}";
				if (i != InputCount -1){
					obj << ",{";
				}
				else {
					obj << "]";
				}
			}
			
			ostream_mux.flush();
			libbitcoin::data_chunk writ_mux_end;
			libbitcoin::data_sink ostream_mux_end(writ_mux_end);
			libbitcoin::ostream_writer w_mux_end(ostream_mux_end);
			util_for_turn::WriteUvarint(w_mux_end, mux_size);
			w_mux_end.write_bytes(writ_mux);
			w_mux_end.write_8_bytes_little_endian(uint64_t(1));
			std::vector<char> vec_muxprog;
			util_for_turn::from_hex(std::string("51").data(), vec_muxprog, std::string("51").size(), nDeplength);
			util_for_turn::WriteUvarint(w_mux_end, uint64_t(vec_muxprog.size()));
			w_mux_end.write_bytes(libbitcoin::data_chunk(vec_muxprog.begin(), vec_muxprog.end()));
			ostream_mux_end.flush();
			auto mux_id = util_for_turn::CalcSha3Hash("mux1", libbitcoin::data_chunk(writ_mux_end.begin(), writ_mux_end.end()));
			std::cout << "mux id is " << util_for_turn::BinToHex(mux_id, false) << std::endl;
			libbitcoin::hash_digest muxid;
			for (int i = 0; i < mux_id.size(); i++)
			{
				muxid[i] = mux_id[i];
			}
			auto OutputCount = util_for_turn::ReadUvarint(source);
			util_for_turn::WriteUvarint(w, OutputCount);
			for (int i = 0; i < OutputCount; i++)
			{
				if (i == 0){
					obj << ",\"outputs\":[{";
				}
				obj << "\"type\":\"" << "control" << "\",";
				libbitcoin::data_chunk writ_output;
				libbitcoin::data_sink ostream_output(writ_output);
				libbitcoin::ostream_writer w_output(ostream_output);
				auto AssetVersion = util_for_turn::ReadUvarint(source);
				std::cout << "AssetVersion " << AssetVersion << std::endl;
				//CommitmentSuffix ReadExtensibleString
				auto CommitmentSuffix_length = util_for_turn::ReadUvarint(source);
				std::cout << "CommitmentSuffix_length " << CommitmentSuffix_length << std::endl;
				auto CommitmentSuffix_msg = source.read_bytes(CommitmentSuffix_length);
				std::cout << "CommitmentSuffix_msg " << util_for_turn::BinToHex(std::vector<char>(CommitmentSuffix_msg.begin(), CommitmentSuffix_msg.end()), false) << std::endl;
				libbitcoin::data_chunk vaa(CommitmentSuffix_msg.begin(), CommitmentSuffix_msg.end());
				libbitcoin::data_source va(vaa);
				libbitcoin::istream_reader commit_suffix_source(va);
				auto assetID = commit_suffix_source.read_hash();
				obj << "\"assetID\":\"" << util_for_turn::BinToHex(std::vector<char>(assetID.begin(), assetID.end()), false) << "\",";
				std::cout << "Output Asset ID " << util_for_turn::BinToHex(std::vector<char>(assetID.begin(), assetID.end()), false) << std::endl;
				auto amount = util_for_turn::ReadUvarint(commit_suffix_source);
				all_output += amount;
				obj << "\"amount\":" << amount << ",";
				std::cout << "Output Asset amount " << amount << std::endl;;
				auto VMVersion = util_for_turn::ReadUvarint(commit_suffix_source);
				std::cout << "Output Asset VMVersion " << VMVersion << std::endl;
				auto CtrlProgLen = util_for_turn::ReadUvarint(commit_suffix_source);
				auto CtrlProg = commit_suffix_source.read_bytes(CtrlProgLen);
				obj << "\"control_program\":\"" << util_for_turn::BinToHex(std::vector<char>(CtrlProg.begin(), CtrlProg.end()), false) << "\",";
				std::string btm_prifix = "mainnet";
				if (btm_script == 0x00) {
					btm_prifix = "mainnet";
				}
				else if (btm_script == 0x01) {
					btm_prifix = "wisdom";
				}
				else if (btm_script == 0x02) {
					btm_prifix = "solonet";
				}
				obj << "\"address\":\"" << bytom::GetBytomAddressFromHash(util_for_turn::BinToHex(std::vector<char>(CtrlProg.begin()+2, CtrlProg.end()), false), btm_prifix) << "\",";
				obj << "\"position\":" << i;
				std::vector<char> vec_ctrlprog;
				util_for_turn::from_hex(std::string("6a").data(), vec_ctrlprog, std::string("6a").size(), nDeplength);
				libbitcoin::hash_digest result_id_hash_digest;
				if (CtrlProgLen > 0 && CtrlProg[0] == vec_ctrlprog[0]) {
					std::cout << " IN R" << std::endl;
					w_output.write_hash(muxid);
					w_output.write_hash(assetID);
					w_output.write_8_bytes_little_endian(amount);
					w_output.write_8_bytes_little_endian((uint64_t)i);
					ostream_output.flush();
					auto result_id = util_for_turn::CalcSha3Hash("retirement1", writ_output);
					std::cout << "result_id id is " << util_for_turn::BinToHex(result_id, false) << std::endl;
					for (int i = 0; i < result_id.size(); i++)
					{
						result_id_hash_digest[i] = result_id[i];
					}

				}
				else {
					std::cout << " IN NON-R" << std::endl;
					w_output.write_hash(muxid);
					w_output.write_hash(assetID);
					w_output.write_8_bytes_little_endian(amount);
					w_output.write_8_bytes_little_endian((uint64_t)i);
					w_output.write_8_bytes_little_endian(VMVersion);
					util_for_turn::WriteUvarint(w_output, CtrlProgLen);
					w_output.write_bytes(CtrlProg);
					ostream_output.flush();
					auto result_id = util_for_turn::CalcSha3Hash("output1", writ_output);
					std::cout << "result_id id is " << util_for_turn::BinToHex(result_id, false) << std::endl;
					for (int i = 0; i < result_id.size(); i++)
					{
						result_id_hash_digest[i] = result_id[i];
					}
				}
				obj << "}";
				if (i != OutputCount - 1) {
					obj << ",{";
				}
				else {
					obj << "]";
				}
				
				w.write_hash(result_id_hash_digest);
				auto output_witness_length = util_for_turn::ReadUvarint(source);
				std::cout << "output_witness_length is " << output_witness_length;
				source.read_bytes(output_witness_length);
			}
			ostream.flush();
			auto head_id = util_for_turn::CalcSha3Hash("txheader", writ);
			obj << ",\"fee\":" << all_input - all_output;
			obj << ",\"tx_id\":\"" << util_for_turn::BinToHex(std::vector<char>(head_id.begin(), head_id.end()), false) << "\"}";
			return obj.str();
		}
		std::string decoderawtransaction(const std::string& trx,uint8_t kh,uint8_t sh)
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
				obj << "\"scriptPubKey\": {";
				obj << "\"addresses\": [\"" << output.address(kh,sh) << "\"],";
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
			std::map<int,std::vector<std::string>> signatures;
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
					signatures[vin_index].push_back(hex);
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

