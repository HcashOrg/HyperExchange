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
#include <fc/crypto/ripemd160.hpp>
#include <fc/optional.hpp>
#include <graphene/chain/pts_address.hpp>
#include <bitcoin/bitcoin.hpp>
#include <graphene/crosschain_privatekey_management/util.hpp>
#include <assert.h>
#include <graphene/utilities/hash.hpp>
#include <list>

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

	std::string  crosschain_privatekey_base::sign_trx(const std::string& raw_trx,int index)
	{
		//get endorsement
		libbitcoin::endorsement out;
		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
		auto prev_script = libbitcoin::chain::script::to_pay_key_hash_pattern(libbitcoin_priv.to_payment_address().hash());
		libbitcoin::chain::script   libbitcoin_script;//(libbitcoin::data_chunk(base),true);	
		libbitcoin::chain::transaction  trx;
		trx.from_data(libbitcoin::config::base16(raw_trx));
		uint8_t hash_type = libbitcoin::machine::sighash_algorithm::all;

		auto result = libbitcoin::chain::script::create_endorsement(out, libbitcoin_priv.secret(), prev_script, trx, index, hash_type);
		assert( result == true);
// 		printf("endorsement is %s\n", libbitcoin::encode_base16(out).c_str());

		//get public hex
		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();
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
	bool crosschain_privatekey_base::validate_address(const std::string& addr)
	{
		try {
			graphene::chain::pts_address pts(addr);
			return pts.is_valid() && (pts.version() == get_pubkey_prefix()|| pts.version() == get_script_prefix());
		}
		catch (fc::exception& e) {
			return false;
		}
		
	}


	void btc_privatekey::init()
	{
		set_id(0);
		set_pubkey_prefix(0x6F);
		set_script_prefix(0xC4);
		set_privkey_prefix(0xEF);
		//set_pubkey_prefix(0x0);
		//set_script_prefix(0x05);
		//set_privkey_prefix(0x80);
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
	std::string btc_privatekey::sign_trx(const std::string& raw_trx,int index)
	{
		return this->crosschain_privatekey_base::sign_trx(raw_trx,index);
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
				auto endorse = graphene::privatekey_management::create_endorsement(get_wif_key(), redeemscript,trx,index);
				trx = graphene::privatekey_management::mutisign_trx(endorse,redeemscript,trx,index);
			}
			return trx;
		}FC_CAPTURE_AND_RETHROW((redeemscript)(raw_trx));
	}



	void ltc_privatekey::init()
	{
		set_id(0);
		set_pubkey_prefix(0x6F);
		set_script_prefix(0x3A);
		set_privkey_prefix(0xEF);
		//set_pubkey_prefix(0x30);
		//set_script_prefix(0x32);
		//set_privkey_prefix(0xB0);
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
				auto endorse = graphene::privatekey_management::create_endorsement(get_wif_key(), redeemscript, trx, index);
				trx = graphene::privatekey_management::mutisign_trx(endorse, redeemscript, trx, index);
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
	std::string ltc_privatekey::sign_trx( const std::string& raw_trx,int index)
	{
		return this->crosschain_privatekey_base::sign_trx(raw_trx, index);
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
	static libbitcoin::chain::script strip_code_seperators(const libbitcoin::chain::script& script_code)
	{
		libbitcoin::machine::operation::list ops;

		for (auto op = script_code.begin(); op != script_code.end(); ++op)
			if (op->code() != libbitcoin::machine::opcode::codeseparator)
				ops.push_back(*op);

		return libbitcoin::chain::script(std::move(ops));
	}



	std::string create_endorsement_ub(const std::string& signer_wif, const std::string& redeemscript_hex, const std::string& raw_trx, int vin_index)
	{
		libbitcoin::wallet::ec_private libbitcoin_priv(signer_wif);
		libbitcoin::chain::script   libbitcoin_script;
		libbitcoin_script.from_data(libbitcoin::config::base16(redeemscript_hex), false);
		const auto stripped = strip_code_seperators(libbitcoin_script);
		libbitcoin::chain::transaction  trx;
		trx.from_data(libbitcoin::config::base16(raw_trx));
		uint32_t index = vin_index;
		uint32_t sighash_type = libbitcoin::machine::sighash_algorithm::all|0x8;
		libbitcoin::chain::input::list ins;
		const auto& inputs = trx.inputs();
		const auto any = (sighash_type & libbitcoin::machine::sighash_algorithm::anyone_can_pay) != 0;
		ins.reserve(any ? 1 : inputs.size());

		BITCOIN_ASSERT(vin_index < inputs.size());
		const auto& self = inputs[vin_index];

		if (any)
		{
			// Retain only self.
			ins.emplace_back(self.previous_output(), stripped, self.sequence());
		}
		else
		{
			// Erase all input scripts.
			for (const auto& input : inputs)
				ins.emplace_back(input.previous_output(), libbitcoin::chain::script{},
					input.sequence());

			// Replace self that is lost in the loop.
			ins[vin_index].set_script(stripped);
			////ins[input_index].set_sequence(self.sequence());
		}

		// Move new inputs and copy outputs to new transaction.
		libbitcoin::chain::transaction out(trx.version(), trx.locktime(), libbitcoin::chain::input::list{}, trx.outputs());
		out.set_inputs(std::move(ins));
		auto serialized = out.to_data(true, false);
		libbitcoin::extend_data(serialized, libbitcoin::to_little_endian(sighash_type));
		serialized.push_back('\x02');
		serialized.push_back('u');
		serialized.push_back('b');
		
		auto higest =libbitcoin::bitcoin_hash(serialized);

		libbitcoin::ec_signature signature;
		libbitcoin::endorsement endorse;
		if (!libbitcoin::sign(signature, libbitcoin_priv.secret(), higest) || !libbitcoin::encode_signature(endorse, signature))
			return "";
		endorse.push_back(sighash_type);
		endorse.shrink_to_fit();
		return libbitcoin::encode_base16(endorse);
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
				auto endorse = create_endorsement_ub(get_wif_key(), redeemscript, trx, index);
				trx = graphene::privatekey_management::mutisign_trx(endorse, redeemscript, trx, index);
			}
			return trx;
		}FC_CAPTURE_AND_RETHROW((redeemscript)(raw_trx));
	}
	std::string ub_privatekey::sign_trx(const std::string& raw_trx, int index)
	{
		libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
		libbitcoin::chain::script script = libbitcoin::chain::script::to_pay_key_hash_pattern(libbitcoin_priv.to_payment_address().hash());
		auto prev = libbitcoin::encode_base16(script.to_data(false));
		auto out = create_endorsement_ub(get_wif_key(), prev, raw_trx, index);
		FC_ASSERT(out != "");
		
		libbitcoin::chain::script   libbitcoin_script;//(libbitcoin::data_chunk(base),true);
		libbitcoin::chain::transaction  trx;
		trx.from_data(libbitcoin::config::base16(raw_trx));
		uint8_t hash_type = libbitcoin::machine::sighash_algorithm::all;
		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();
		std::string endorsment_script = "[" + out + "]" + " [" + pub_hex + "] ";
		libbitcoin_script.from_string(endorsment_script);
		trx.inputs()[index].set_script(libbitcoin_script);
		std::string signed_trx = libbitcoin::encode_base16(trx.to_data());
		return signed_trx;
	}
	void hc_privatekey::init() {
		set_id(0);
		set_pubkey_prefix(0x0f21);
		set_privkey_prefix(0x230e);
		//set_pubkey_prefix(0x097f);
		//set_privkey_prefix(0x19ab);
	}
	std::string hc_privatekey::get_wif_key() {
		FC_ASSERT(is_empty() == false, "private key is empty!");

		fc::sha256 secret = get_private_key().get_secret();
		//one byte for prefix, one byte for compressed sentinel
		const size_t size_of_data_to_hash = sizeof(secret) + 3;
		const size_t size_of_hash_bytes = 4;
		char data[size_of_data_to_hash + size_of_hash_bytes];
		int prikey_prefix = get_privkey_prefix();
		data[0] = (char)(prikey_prefix >> 8);
		data[1] = (char)prikey_prefix;
		data[2] = (char)(0);
		memcpy(&data[3], (char*)&secret, sizeof(secret));
		//data[size_of_data_to_hash] = (char)0x01;
		uint256 digest = HashR14(data, data+size_of_data_to_hash);
		//digest = fc::sha256::hash(digest);
		memcpy(data + size_of_data_to_hash, (char*)&digest, size_of_hash_bytes);
		return fc::to_base58(data, sizeof(data));
	}
	void input_todata(libbitcoin::istream_reader& source, libbitcoin::ostream_writer& write) {
		auto InputCount = source.read_size_little_endian();
		write.write_size_little_endian(uint64_t(InputCount));
		//std::cout << InputCount << std::endl;
		for (uint64_t i = 0; i < InputCount; ++i) {
			auto Hash = source.read_hash();
			auto Index = source.read_4_bytes_little_endian();
			auto Tree = source.read_size_little_endian();
			write.write_hash(Hash);
			write.write_4_bytes_little_endian(Index);
			write.write_size_little_endian(Tree);
			auto Sequence = source.read_4_bytes_little_endian();
			write.write_4_bytes_little_endian(Sequence);
		}
	}
	void output_todata(libbitcoin::istream_reader& source, libbitcoin::ostream_writer& write) {
		auto OutputCount = source.read_size_little_endian();
		write.write_size_little_endian((uint64_t)OutputCount);
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
			//std::cout << PkScript.size() << "-" << output_count<<std::endl;
		}
	}
	void witness_todata(libbitcoin::istream_reader& source, libbitcoin::ostream_writer& write, libbitcoin::chain::script libbitcoin_script,int vin_index,bool bHashMode = false) {
		auto witness_count = source.read_size_little_endian();
		write.write_size_little_endian((uint64_t)witness_count);
		for (uint64_t i = 0; i < (int)witness_count; ++i) {
			auto ValueIn = source.read_8_bytes_little_endian();
			auto BlockHeight = source.read_4_bytes_little_endian();
			auto BlockIndex = source.read_4_bytes_little_endian();
			auto signtureCount = source.read_size_little_endian();
			//std::cout << signtureCount << std::endl;
			auto SignatureScript = source.read_bytes(signtureCount);
			if (!bHashMode){
				write.write_8_bytes_little_endian(ValueIn);
				write.write_4_bytes_little_endian(BlockHeight);
				write.write_4_bytes_little_endian(BlockIndex);
			}
			//std::cout << SignatureScript.size() << "-" << signtureCount << std::endl;
			if (i == vin_index) {
				write.write_size_little_endian(libbitcoin_script.to_data(false).size());
				write.write_bytes(libbitcoin_script.to_data(false));
			}
			else {
				if (bHashMode){
					write.write_size_little_endian((uint8_t)0);
				}
				else {
					write.write_size_little_endian(signtureCount);
					write.write_bytes(SignatureScript);
				}
				
			}

		}
	}
	std::string create_endorsement_hc(const fc::sha256& secret,const int& version, const std::string& redeemscript_hex, const std::string& raw_trx, int vin_index)
	{
		const size_t size_of_data_to_hash = sizeof(secret);
		char data[size_of_data_to_hash];
		memcpy(&data[0], (char*)&secret, sizeof(secret));
		libbitcoin::ec_secret ecs;
		for (int i = 0; i < size_of_data_to_hash; i++)
		{
			ecs.at(i) = data[i];
		}
		libbitcoin::data_chunk writ;
		libbitcoin::data_sink ostream(writ);
		libbitcoin::ostream_writer w(ostream);
		
		libbitcoin::wallet::ec_private libbitcoin_priv(ecs, version, true);
		//libbitcoin::wallet::ec_private libbitcoin_priv(signer_wif);
		libbitcoin::chain::script   libbitcoin_script;
		libbitcoin_script.from_data(libbitcoin::config::base16(redeemscript_hex), false);
		//const auto stripped = strip_code_seperators(libbitcoin_script);
		libbitcoin::chain::transaction  trx;
		libbitcoin::data_chunk aa = libbitcoin::config::base16(raw_trx);
		libbitcoin::data_source va(aa);
		libbitcoin::istream_reader source(va);
		//拆分现有交易
		uint32_t versiona = source.read_4_bytes_little_endian();
		uint32_t version_prefix = versiona | (1 << 16);
		uint32_t version_witness = versiona | (3 << 16);
		w.write_4_bytes_little_endian(version_prefix);
		input_todata(source, w);
		output_todata(source, w);
		uint32_t locktime = source.read_4_bytes_little_endian();
		uint32_t expir = source.read_4_bytes_little_endian();
		w.write_4_bytes_little_endian(locktime);
		w.write_4_bytes_little_endian(expir);
		ostream.flush();
		//std::cout << "data size is " << libbitcoin::encode_base16(libbitcoin::data_slice(writ)) << std::endl;
		//std::cout << "Hash 1 size is " << writ.size() << std::endl;
		libbitcoin::data_chunk writ2;
		libbitcoin::data_sink ostream2(writ2);
		libbitcoin::ostream_writer w2(ostream2);
		w2.write_4_bytes_little_endian(version_witness);
		witness_todata(source, w2, libbitcoin_script, vin_index, true);
		FC_ASSERT((!source) == 0, "hc source not handle");
		//std::cout << !source << std::endl;
		ostream2.flush();
		//std::cout << "data2 size is " << libbitcoin::encode_base16(libbitcoin::data_slice(writ2)) << std::endl;
		//std::cout << "Hash 2 size is " << writ2.size() << std::endl;
		libbitcoin::data_chunk writ3;
		libbitcoin::data_sink ostream3(writ3);
		libbitcoin::ostream_writer w3(ostream3);
		w3.write_4_bytes_little_endian(libbitcoin::machine::sighash_algorithm::all);
		ostream3.flush();
		uint256 higest1 = HashR14(writ.begin(), writ.end());
		//auto higest = libbitcoin::bitcoin_hash(serialized);
		libbitcoin::hash_digest higest_bitcoin;
		libbitcoin::hash_digest higest_bitcoin1;
		libbitcoin::hash_digest higest_bitcoin2;
		for (int i =0;i < higest_bitcoin.size();i++)
		{
			higest_bitcoin1.at(i) = *(higest1.begin()+i);
			writ3.push_back(*(higest1.begin() + i));
		}
		//std::cout << libbitcoin::encode_base16(higest_bitcoin1) << std::endl;
		uint256 higest2 = HashR14(writ2.begin(), writ2.end());
		for (int i = 0; i < higest_bitcoin.size(); i++)
		{
			higest_bitcoin2.at(i) = *(higest2.begin() + i);
			writ3.push_back(*(higest2.begin() + i));
		}
		//std::cout << libbitcoin::encode_base16(higest_bitcoin2) << std::endl;
		//std::cout << "Hash 3 size is" << writ3.size() << std::endl;
		uint256 higest3= HashR14(writ3.begin(), writ3.end());
		for (int i = 0; i < higest_bitcoin.size(); i++)
		{
			higest_bitcoin.at(i) = *(higest3.begin() + i);
		}
		//std::cout << libbitcoin::encode_base16(higest_bitcoin) << std::endl;
		libbitcoin::ec_signature signature;
		libbitcoin::endorsement endorse;
		/*
		bool end1 = !libbitcoin::sign(signature, libbitcoin_priv.secret(), higest_bitcoin);
		bool end2 = !libbitcoin::encode_signature(endorse, signature);
		std::cout << end1<<"-" << end2 << std::endl;*/
		if (!libbitcoin::sign(signature, libbitcoin_priv.secret(), higest_bitcoin) || !libbitcoin::encode_signature(endorse, signature))
			return "";
 		endorse.push_back(libbitcoin::machine::sighash_algorithm::all);
 		endorse.shrink_to_fit();
		//std::cout << libbitcoin::encode_base16(endorse) << std::endl;
		//FC_ASSERT(0, "test");
		return libbitcoin::encode_base16(endorse);
	}
	std::string hc_privatekey::get_address() {
		FC_ASSERT(is_empty() == false, "private key is empty!");

		//configure for hc
		int version = get_pubkey_prefix();
		bool compress = true;

		const fc::ecc::private_key& priv_key = get_private_key();
		fc::ecc::public_key  pub_key = priv_key.get_public_key();

		graphene::chain::pts_address_extra btc_addr(pub_key, compress, version);
		std::string  addr = btc_addr.operator fc::string();

		return addr;
	}
	std::string hc_privatekey::get_address_by_pubkey(const std::string& pub)
	{
		std::vector<uint8_t> data;
		libbitcoin::wallet::ec_public libbitcoin_pub(pub);
		FC_ASSERT(libbitcoin_pub != libbitcoin::wallet::ec_public(), "the pubkey hex str is in valid!");
		libbitcoin_pub.to_data(data);
		FC_ASSERT(data.size() != 0, "get hc address fail");
		std::string addr = "";
		if (data.size() == 33)
		{
			bool compress = true;
			auto version = get_pubkey_prefix();
			fc::ecc::public_key_data pubkey_data;

			for (auto i = 0; i < data.size(); i++)
			{
				pubkey_data.at(i) = data[i];
			}
			fc::ecc::public_key pub_key(pubkey_data);
			graphene::chain::pts_address_extra btc_addr(pub_key, compress, version);
			addr = btc_addr.operator fc::string();
		}
		else if(data.size() == 65){
			bool compress = false;
			auto version = get_pubkey_prefix();
			fc::ecc::public_key_point_data pubkey_data;

			for (auto i = 0; i < data.size(); i++)
			{
				pubkey_data.at(i) = data[i];
			}
			fc::ecc::public_key pub_key(pubkey_data);
			graphene::chain::pts_address_extra btc_addr(pub_key, compress, version);
			addr = btc_addr.operator fc::string();
		}
		return addr;
		//return graphene::privatekey_management::get_address_by_pubkey(pub, get_pubkey_prefix());
	}
	std::string hc_privatekey::get_public_key()
	{
		fc::sha256 secret = get_private_key().get_secret();
		int version = get_pubkey_prefix();
		const size_t size_of_data_to_hash = sizeof(secret);
		char data[size_of_data_to_hash];
		memcpy(&data[0], (char*)&secret, sizeof(secret));
		
		libbitcoin::ec_secret ecs;
		for (int i = 0;i < size_of_data_to_hash;i++)
		{
			ecs.at(i) = data[i];
		}
		libbitcoin::wallet::ec_private libbitcoin_priv(ecs, version,true);

		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();

		return pub_hex;

	}

	std::string hc_privatekey::sign_message(const std::string& msg)
	{
		//libbitcoin::wallet::message_signature sign;

		fc::sha256 secret = get_private_key().get_secret();
		int version = get_pubkey_prefix();
		const size_t size_of_data_to_hash = sizeof(secret);
		char data_a[size_of_data_to_hash];
		memcpy(&data_a[0], (char*)&secret, sizeof(secret));

		libbitcoin::ec_secret ecs;
		for (int i = 0; i < size_of_data_to_hash; i++)
		{
			ecs.at(i) = data_a[i];
		}
		libbitcoin::wallet::ec_private libbitcoin_priv(ecs, version, true);
		// 		libbitcoin::wallet::ec_private libbitcoin_priv("L13gvvM3TtL2EmfBdye8tp4tQhcbCG3xz3VPrBjSZL8MeJavLL8K");
		//libbitcoin::data_chunk  data(msg.begin(), msg.end());
		libbitcoin::recoverable_signature recoverable;
		std::string prefix = "Hc Signed Message:\n";
		libbitcoin::data_chunk  msg_data(msg.begin(), msg.end());
		libbitcoin::data_slice msg_slice_data(msg_data);
		libbitcoin::data_chunk data;
		libbitcoin::data_sink ostream(data);
		libbitcoin::ostream_writer sink(ostream);
		sink.write_string(prefix);
		sink.write_variable_little_endian(msg_slice_data.size());
		sink.write_bytes(msg_slice_data.begin(), msg_slice_data.size());
		ostream.flush();
		uint256 messagehash = HashR14(data.begin(), data.end());
		libbitcoin::hash_digest higest_bitcoin;
		for (int i = 0; i < higest_bitcoin.size(); i++)
		{
			higest_bitcoin.at(i) = *(messagehash.begin() + i);
		}
		if (!libbitcoin::sign_recoverable(recoverable, libbitcoin_priv.secret(), higest_bitcoin))
			return "";

		uint8_t magic;
		if (!libbitcoin::wallet::recovery_id_to_magic(magic, recoverable.recovery_id, true))
			return "";

		auto signature = libbitcoin::splice(libbitcoin::to_array(magic), recoverable.signature);

		auto result = libbitcoin::encode_base64(signature);
		return result;
	}

	fc::optional<fc::ecc::private_key> hc_wif_to_key(const std::string& wif_key) {
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
		std::vector<char> key_bytes(wif_bytes.begin() + 3, wif_bytes.end() - 4);
		fc::ecc::private_key key = fc::variant(key_bytes).as<fc::ecc::private_key>();
		uint256 check = HashR14(wif_bytes.data(), wif_bytes.data()+wif_bytes.size() - 4);
		if (memcmp((char*)&check, wif_bytes.data() + wif_bytes.size() - 4, 4) == 0)
			return key;

		return fc::optional<fc::ecc::private_key>();
	}
	fc::optional<fc::ecc::private_key>   hc_privatekey::import_private_key(const std::string& wif_key)
	{
		auto key = hc_wif_to_key(wif_key);
		set_key(*key);
		return key;

	}
	std::string mutisign_trx_hc(const std::string& endorse, const std::string& redeemscript_hex, const std::string& raw_trx, int vin_index) {
		FC_ASSERT(endorse != "");
		//std::string  endorse = create_endorsement(signer_wif, redeemscript_hex, raw_trx, vin_index);

		//get signed raw-trx
		std::string endorsement_script = "";
		endorsement_script += "[" + endorse + "] ";
		endorsement_script += "[" + redeemscript_hex + "] ";

		//             printf("endorsement script is %s\n", endorsement_script.c_str());

		libbitcoin::chain::script   libbitcoin_script;
		libbitcoin_script.from_string(endorsement_script);
		libbitcoin::data_chunk writ;
		libbitcoin::data_sink ostream(writ);
		libbitcoin::ostream_writer w(ostream);
		//const auto stripped = strip_code_seperators(libbitcoin_script);
		libbitcoin::chain::transaction  trx;
		libbitcoin::data_chunk aa = libbitcoin::config::base16(raw_trx);
		libbitcoin::data_source va(aa);
		libbitcoin::istream_reader source(va);

		uint32_t versiona = source.read_4_bytes_little_endian();
		w.write_4_bytes_little_endian(versiona);
		input_todata(source, w);
		output_todata(source, w);
		
		uint32_t locktime = source.read_4_bytes_little_endian();
		uint32_t expir = source.read_4_bytes_little_endian();
		w.write_4_bytes_little_endian(locktime);
		w.write_4_bytes_little_endian(expir);
		witness_todata(source, w, libbitcoin_script, vin_index);

		FC_ASSERT((!source) == 0, "hc source not handle");
		//std::cout << !source << std::endl;
		ostream.flush();
		std::string signed_trx = libbitcoin::encode_base16(libbitcoin::data_slice(writ));
		//std::cout << signed_trx << std::endl;
		//             printf("signed trx is %s\n", signed_trx.c_str());
		//FC_ASSERT(0, "test");
		return signed_trx;
	}
	std::string hc_privatekey::mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx)
	{
		try {
			FC_ASSERT(raw_trx.contains("hex"));
			FC_ASSERT(raw_trx.contains("trx"));
			fc::sha256 secret = get_private_key().get_secret();
			int version = get_pubkey_prefix();
			auto tx = raw_trx["trx"].get_object();
			auto size = tx["vin"].get_array().size();
			std::string trx = raw_trx["hex"].as_string();
			for (int index = 0; index < size; index++)
			{
				auto endorse = create_endorsement_hc(secret, version,redeemscript, trx, index);
				trx = mutisign_trx_hc(endorse, redeemscript, trx, index);
			}
			return trx;
		}FC_CAPTURE_AND_RETHROW((redeemscript)(raw_trx));
	}

	libbitcoin::chain::script stript_code_create_hc(const fc::ripemd160 rep) {
		//FC_ASSERT(pkscript.size() != 20, "hc pkscript size error");
		libbitcoin::machine::operation::list ops;
		//ops.push_back(libbitcoin::machine::opcode::codeseparator);
		ops.push_back(libbitcoin::machine::opcode::dup);
		ops.push_back(libbitcoin::machine::opcode::hash160);
		//ops.push_back(libbitcoin::machine::opcode::push_size_20);
		libbitcoin::data_chunk pk;
		for (int i =0;i < rep.data_size();i++){
			pk.push_back(*(rep.data()+i));
		}
		libbitcoin::machine::operation op(pk);
		ops.push_back(op);
		ops.push_back(libbitcoin::machine::opcode::equalverify);
		ops.push_back(libbitcoin::machine::opcode::checksig);
		return libbitcoin::chain::script(std::move(ops));
	}

	std::string hc_privatekey::sign_trx(const std::string& raw_trx, int index)
	{
		//libbitcoin::wallet::ec_private libbitcoin_priv(get_wif_key());
		fc::sha256 secret = get_private_key().get_secret();
		int version = get_pubkey_prefix();
		const size_t size_of_data_to_hash = sizeof(secret);
		char data[size_of_data_to_hash];
		memcpy(&data[0], (char*)&secret, sizeof(secret));
		libbitcoin::ec_secret ecs;
		for (int i = 0; i < size_of_data_to_hash; i++)
		{
			ecs.at(i) = data[i];
		}
		libbitcoin::wallet::ec_private libbitcoin_priv(ecs, version, true);
		uint256 digest;
		
		auto dat = get_private_key().get_public_key().serialize();

		digest = HashR14(dat.data, dat.data + sizeof(dat));
		//auto dat = pub.serialize_ecc_point();
		//digest = HashR14(dat.data, dat.data + sizeof(dat));
		auto rep = fc::ripemd160::hash((char*)&digest, sizeof(digest));
		libbitcoin::chain::script libbitcoin_script = stript_code_create_hc(rep);

		//libbitcoin::chain::script script = libbitcoin::chain::script::to_pay_key_hash_pattern(libbitcoin_priv.to_payment_address().hash());
		auto prev = libbitcoin::encode_base16(libbitcoin_script.to_data(false));
		auto out = create_endorsement_hc(secret, version, prev, raw_trx, index);
		FC_ASSERT(out != "");
		libbitcoin::data_chunk writ;
		libbitcoin::data_sink ostream(writ);
		libbitcoin::ostream_writer w(ostream);
		libbitcoin::chain::transaction  trx;
		libbitcoin::data_chunk aa = libbitcoin::config::base16(raw_trx);
		libbitcoin::data_source va(aa);
		libbitcoin::istream_reader source(va);
		libbitcoin::wallet::ec_public libbitcoin_pub = libbitcoin_priv.to_public();
		std::string pub_hex = libbitcoin_pub.encoded();
		std::string endorsment_script = "[" + out + "]" + " [" + pub_hex + "] ";
		libbitcoin_script.from_string(endorsment_script);
		uint32_t versiona = source.read_4_bytes_little_endian();
		w.write_4_bytes_little_endian(versiona);
		input_todata(source, w);
		output_todata(source, w);
		uint32_t locktime = source.read_4_bytes_little_endian();
		uint32_t expir = source.read_4_bytes_little_endian();
		w.write_4_bytes_little_endian(locktime);
		w.write_4_bytes_little_endian(expir);
		witness_todata(source, w, libbitcoin_script, index);
		FC_ASSERT((!source) == 0, "hc source not handle");
		ostream.flush();
		std::string signed_trx = libbitcoin::encode_base16(libbitcoin::data_slice(writ));
		//std::cout << signed_trx << std::endl;

		/*
		libbitcoin::chain::script   libbitcoin_script;//(libbitcoin::data_chunk(base),true);
		libbitcoin::chain::transaction  trx;
		trx.from_data(libbitcoin::config::base16(raw_trx));
		uint8_t hash_type = libbitcoin::machine::sighash_algorithm::all;
		
		trx.inputs()[index].set_script(libbitcoin_script);
		std::string signed_trx = libbitcoin::encode_base16(trx.to_data());*/
		return signed_trx;
	}
	bool hc_privatekey::validate_address(const std::string& addr) {
		try {
			graphene::chain::pts_address_extra pts(addr);
			//return pts.is_valid() && pts.version() == get_pubkey_prefix();
			return pts.is_valid();
		}
		catch (fc::exception& e) {
			return false;
		}
	}
	crosschain_management::crosschain_management()
	{
		crosschain_decode.insert(std::make_pair("BTC", &graphene::privatekey_management::btc_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("LTC", &graphene::privatekey_management::ltc_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("UB", &graphene::privatekey_management::ub_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("HC", &graphene::privatekey_management::hc_privatekey::decoderawtransaction));
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
		else if (name == "HC") {
			auto itr = crosschain_prks.insert(std::make_pair(name, new hc_privatekey()));
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
