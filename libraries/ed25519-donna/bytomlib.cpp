#include "bech32/segwit_addr.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include <iostream>
#include "ed25519.h"
#include "hmac/hmac_sha2.h"
#include <fc/crypto/ripemd160.hpp>
#include "bech32.h"
#include "bytomlib.hpp"
namespace bytom {
	namespace hexturn {
		bool  from_hex(const char *pSrc, std::vector<char> &pDst, unsigned int nSrcLength, unsigned int &nDstLength)
		{
			if (pSrc == 0)
			{
				return false;
			}

			nDstLength = 0;

			if (pSrc[0] == 0) // nothing to convert  
				return 0;

			for (int j = 0; pSrc[j]; j++)
			{
				if (isxdigit(pSrc[j]))
					nDstLength++;
			}

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
	}
	std::string GenerateBytomKey(std::string memstr) {
		/*
		std::string mmemkey = "";
		for (int i = 0;i < sizeof(messages_key);++i)
		{
			messages_key[i] = (unsigned char)mmemkey[i];
		}
		*/
		const unsigned char rot[4] = { 'R', 'o', 'o', 't' };
		ed25519_secret_key Xprev;
		/*std::vector<char> tempSeed;
		unsigned int nDeplength = 0;
		bool b_converse = hexturn::from_hex((const char *)messages_key, tempSeed, sizeof(messages_key), nDeplength);
		std::string seed_binstr(tempSeed.begin(), tempSeed.end());*/
		hmac_sha512_ctx ctx_temp;
		hmac_sha512_init(&ctx_temp, rot, sizeof(rot));
		hmac_sha512_update(&ctx_temp, (const unsigned char*)memstr.data(), memstr.size());
		hmac_sha512_final(&ctx_temp, Xprev, 64);
		Xprev[0] &= 248;
		Xprev[31] &= 31;
		Xprev[31] |= 64;
		std::vector<char> tempXprv;
		for (int i = 0; i < 64; i++)
		{
			tempXprv.push_back(Xprev[i]);
		}
		std::string prikey = hexturn::BinToHex(tempXprv, false);
		//std::cout << prikey << std::endl;
		//prikey = "";
		return prikey;
	}
	std::string GetBytomPubKeyFromPrv(std::string Xprev) {
		ed25519_public_key pk;
		std::vector<char> tempXprev;
		unsigned int nDeplength = 0;
		bool b_converse = hexturn::from_hex(Xprev.data(), tempXprev, Xprev.size(), nDeplength);
		ed25519_secret_key sk;
		for (int i = 0; i < tempXprev.size(); i++)
		{
			sk[i] = (unsigned char)tempXprev[i];
		}
		std::vector<char> tempXpub;
		ed25519_publickey(sk, pk);
		for (int i = 0; i < 32; i++)
		{
			tempXpub.push_back(pk[i]);
		}
		for (int i = 32; i < 64; i++)
		{
			pk[i] = sk[i];
			tempXpub.push_back(sk[i]);
		}
		std::string pubkey = hexturn::BinToHex(tempXpub, false);
		//std::cout << pubkey << std::endl;
		return pubkey;
	}
	std::string GetBytomAddressFromHash(std::string ctrlProgHash, std::string NetParam) {
		std::vector<char> binHash;
		unsigned int nDeplength = 0;
		std::cout << "source ctrl hash is " << ctrlProgHash << std::endl;
		bool b_converse = hexturn::from_hex(ctrlProgHash.data(), binHash, ctrlProgHash.size(), nDeplength);
		char bin_hash[64];
		::memset(bin_hash, 0, 64);
		for (int i = 0; i < binHash.size(); i++)
		{
			bin_hash[i] = binHash[i];
		}
		std::string prefix = "bm";
		if (NetParam == "mainnet") {

			prefix = "bm";
		}
		else if (NetParam == "wisdom") {
			prefix = "tm";
		}
		else if (NetParam == "solonet") {
			prefix = "sm";
		}
		std::string binhashStr(bin_hash);
		std::cout << "binHashstr size is " << binhashStr.size();
		char outp[1024];
		::memset(outp, 0, 1024);
		int a = segwit_addr_encode(outp, prefix.data(), 0, ((const uint8_t *)binhashStr.c_str()), binhashStr.size());
		//auto addr = bech32::Encode(prefix, temp2);
		std::cout << std::string(outp) << std::endl;
		return  std::string(outp);
	}
	std::string GetBytomAddressFromPub(std::string Pubkey, std::string NetParam) {
		std::vector<char> tempXpub;
		unsigned int nDeplength = 0;
		bool b_converse = hexturn::from_hex(Pubkey.data(), tempXpub, Pubkey.size(), nDeplength);
		char pubkey_c[32];
		for (int i = 0; i < 32; i++)
		{
			pubkey_c[i] = tempXpub[i];
		}
		auto pubHash = fc::ripemd160::hash((char*)&pubkey_c, sizeof(pubkey_c));
		std::string prefix = "bm";
		if (NetParam == "mainnet") {

			prefix = "bm";
		}
		else if (NetParam == "wisdom") {
			prefix = "tm";
		}
		else if (NetParam == "solonet") {
			prefix = "sm";
		}
		std::string pubHashStr = pubHash.str();
		std::vector<char> temp2;
		char outp[1024];
		::memset(outp, 0, 1024);
		hexturn::from_hex(pubHashStr.c_str(), temp2, pubHashStr.size(), nDeplength);
		std::string real_bin(temp2.begin(), temp2.end());
		int a = segwit_addr_encode(outp, prefix.data(), 0, ((const uint8_t *)real_bin.c_str()), real_bin.size());
		//auto addr = bech32::Encode(prefix, temp2);
		std::cout << std::string(outp) << std::endl;
		return  std::string(outp);
	}
	bool ValidateAddress(std::string address, std::string NetParam) {
		bool ret = false;
		do{
			std::string prefix = "bm";
			if (NetParam == "mainnet") {

				prefix = "bm";
			}
			else if (NetParam == "wisdom") {
				prefix = "tm";
			}
			else if (NetParam == "solonet") {
				prefix = "sm";
			}
			auto prefix_pos = address.find('1');
			if (prefix_pos == address.npos)	{
				break;
			}
			auto addr_prefix = address.substr(0, prefix_pos);
			if (addr_prefix != prefix){
				break;
			}
			int ver;
			char outp[80];
			::memset(outp, 0, 80);
			size_t prop_len;
			int decodea = segwit_addr_decode(&ver, (uint8_t *)outp, &prop_len, prefix.data(), address.data());
			if (decodea != 1)
			{
				break;
			}
			if (ver !=0){
				break;
			}
			if ((prop_len != 32) && (prop_len != 20)){
				break;
			}
			ret = true;
		} while (0);

		return ret;
	}
	std::string SignMessage(std::string hex_str, std::string Xprev) {
		std::vector<char> tempXprv;
		unsigned int nDeplength = 0;
		bool b_converse = hexturn::from_hex(Xprev.data(), tempXprv, Xprev.size(), nDeplength);
		ed25519_secret_key sk;
		for (int i = 0; i < tempXprv.size(); i++)
		{
			sk[i] = (unsigned char)tempXprv[i];
		}
		const unsigned char rot[6] = { 'E', 'x', 'p', 'a', 'n', 'd' };
		ed25519_secret_key Xprv;
		hmac_sha512_ctx ctx_temp;
		hmac_sha512_init(&ctx_temp, rot, sizeof(rot));
		hmac_sha512_update(&ctx_temp, (const unsigned char*)sk, sizeof(sk));
		hmac_sha512_final(&ctx_temp, Xprv, 64);
		//std::vector<char> temp1;
		for (int i = 0; i < 32; i++){
			Xprv[i] = sk[i];
			//temp1.push_back(sk[i]);
		}
		/*
		for (int i = 32; i < 64; i++)
		{
			temp1.push_back(Xprv[i]);
		}
		std::cout << all::BinToHex(temp1, false) << std::endl;
		*/
		
		ed25519_public_key pk;
		ed25519_publickey(Xprv, pk);
		//std::vector<char> temppk;
// 		for (int i = 0; i < 32; i++)
// 		{
// 			temppk.push_back(pk[i]);
// 		}
		for (int i = 32; i < 64; i++){
			pk[i] = Xprv[i];
			//temppk.push_back(Xprv[i]);
		}
		//std::cout << hexturn::BinToHex(temppk, false) << std::endl;
		ed25519_signature ts;
		std::vector<char> temp_message;
		//unsigned int nDeplength = 0;
		//auto as_str = char2hex(source_str);
		b_converse = hexturn::from_hex(hex_str.data(), temp_message, hex_str.size(), nDeplength);
		//std::cout << all::BinToHex(temp_message, false) <<std::endl;
		std::string temp_message_bin_str(temp_message.begin(), temp_message.end());
		ed25519_sign((const unsigned char *)temp_message_bin_str.data(), temp_message_bin_str.size(), Xprv, pk, ts);
		std::vector<char> tempSig;
		for (int i = 0; i < sizeof(ts); i++){
			tempSig.push_back(ts[i]);
		}
		return hexturn::BinToHex(tempSig, false);
	}
	bool VerifySignBytomMessage(std::string sign_str, std::string hex_source_msg, std::string Xpub) {
		std::vector<char> temp_XSig;
		std::vector<char> temp_XPub;
		unsigned int nDeplength = 0;
		ed25519_public_key pubk;
		ed25519_signature ts;
		bool b_converse = hexturn::from_hex(sign_str.data(), temp_XSig, sign_str.size(), nDeplength);
		b_converse = hexturn::from_hex(Xpub.data(), temp_XPub, Xpub.size(), nDeplength);
		for (int i = 0; i < 64; i++)
		{
			pubk[i] = temp_XPub[i];
			ts[i] = temp_XSig[i];
		}
		std::vector<char> temp_message;
		//unsigned int nDeplength = 0;
		b_converse = hexturn::from_hex(hex_source_msg.data(), temp_message, hex_source_msg.size(), nDeplength);
		std::string temp_message_bin_str(temp_message.begin(), temp_message.end());
		int breturn = ed25519_sign_open((const unsigned char *)temp_message_bin_str.data(), temp_message_bin_str.size(), pubk, ts);
		
		return (breturn == 0);
	}
}