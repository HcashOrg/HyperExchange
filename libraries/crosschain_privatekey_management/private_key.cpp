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
#include <fc/crypto/hex.hpp>
#include <fc/optional.hpp>
#include <graphene/chain/pts_address.hpp>
#include <bitcoin/bitcoin.hpp>
#include <graphene/crosschain_privatekey_management/util.hpp>
#include <assert.h>
#include <graphene/utilities/hash.hpp>
#include <list>
#include <libdevcore/DevCoreCommonJS.h>
#include <libdevcore/RLP.h>
#include <libdevcore/FixedHash.h>
#include <libethcore/TransactionBase.h>
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

	void crosschain_privatekey_base::generate(fc::optional<fc::ecc::private_key> k)
	{
		if (!k.valid())
		{

			_key = fc::ecc::private_key::generate();
		}
		else
		{
			_key = *k;
		}
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
	bool crosschain_privatekey_base::validate_transaction(const std::string& addr,const std::string& redeemscript,const std::string& sig)
	{
		return graphene::utxo::validateUtxoTransaction(addr,redeemscript,sig);
	}
	fc::variant_object crosschain_privatekey_base::combine_trxs(const std::vector<std::string>& trxs)
	{
		auto trx = graphene::utxo::combine_trx(trxs);
		fc::mutable_variant_object result;
		result["trx"]=fc::json::from_string(graphene::utxo::decoderawtransaction(trx)).get_object();
		result["hex"] = trx;
		return result;
	}

	bool crosschain_privatekey_base::verify_message(const std::string addr, const std::string& content, const std::string& encript)
	{
		return true;
	}

	void btc_privatekey::init()
	{
		set_id(0);
		//set_pubkey_prefix(0x6F);
		//set_script_prefix(0xC4);
		//set_privkey_prefix(0xEF);
		set_pubkey_prefix(0x0);
		set_script_prefix(0x05);
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
		//set_pubkey_prefix(0x6F);
		//set_script_prefix(0x3A);
		//set_privkey_prefix(0xEF);
		set_pubkey_prefix(0x30);
		set_script_prefix(0x32);
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

	bool ltc_privatekey::verify_message(const std::string addr, const std::string& content, const std::string& encript)
	{
		return graphene::utxo::verify_message(addr, content, encript, "Litecoin Signed Message:\n");
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
		set_script_prefix(0x05);
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

	std::string inter_get_address_by_pubkey(const libbitcoin::ec_uncompressed& pub)
	{
		std::vector<uint8_t> data;
		libbitcoin::wallet::ec_public libbitcoin_pub(pub,true);
		FC_ASSERT(libbitcoin_pub != libbitcoin::wallet::ec_public(), "the pubkey hex str is in valid!");
		libbitcoin_pub.to_data(data);
		FC_ASSERT(data.size() != 0, "get hc address fail");
		hc_privatekey hc_manager;
		std::string addr = "";
		if (data.size() == 33)
		{
			bool compress = true;
			auto version = hc_manager.get_pubkey_prefix();
			fc::ecc::public_key_data pubkey_data;

			for (auto i = 0; i < data.size(); i++)
			{
				pubkey_data.at(i) = data[i];
			}
			fc::ecc::public_key pub_key(pubkey_data);
			graphene::chain::pts_address_extra btc_addr(pub_key, compress, version);
			addr = btc_addr.operator fc::string();
		}
		else if (data.size() == 65) {
			bool compress = false;
			auto version = hc_manager.get_pubkey_prefix();
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

	bool hc_privatekey::verify_message(const std::string& address,const std::string& msg,const std::string& signature)
	{
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
		libbitcoin::data_chunk sig_data;
		libbitcoin::decode_base64(sig_data,signature);
		uint8_t magic_id = sig_data[0];
		uint8_t out_recovery_id;
		bool is_compressed = true;
		libbitcoin::wallet::magic_to_recovery_id(out_recovery_id, is_compressed, magic_id);
		recoverable.recovery_id = out_recovery_id;
		for( int i = 0; i < 64; i++){
			recoverable.signature[i] = sig_data[1 + i];
		}
		libbitcoin::hash_digest higest_bitcoin;
	
		for (int i = 0; i < 32; i++)
		{
			higest_bitcoin.at(i) = *(messagehash.begin() + i);
		}
		libbitcoin::ec_uncompressed out_pubkey;
		if (!libbitcoin::recover_public(out_pubkey, recoverable, higest_bitcoin)) {
			return "";
		}
		if (address == inter_get_address_by_pubkey(out_pubkey)) {
			return true;
		}
		return false;

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

	fc::variant_object hc_privatekey::combine_trxs(const std::vector<std::string>& trxs)
	{
		auto hex = hc_combine_trx(trxs);
		fc::mutable_variant_object result;
		result["trx"] = fc::json::from_string(graphene::utxo::decoderawtransaction(hex)).get_object();
		result["hex"] = hex;
		return result;
	}
	


	std::string hc_privatekey::hc_combine_trx(const std::vector<std::string>& trxs)
	{
		if (trxs.size() < 2) {
			FC_ASSERT(false, "trx count is least than min required");
		}
		auto trx = trxs[0];
		std::map<int, std::vector<libbitcoin::chain::script::operation>> bak_signature;
		std::ostringstream obj;
		libbitcoin::data_chunk writ;
		libbitcoin::data_sink ostream(writ);
		libbitcoin::ostream_writer w(ostream);
		libbitcoin::data_chunk aa = libbitcoin::config::base16(trx);
		libbitcoin::data_source va(aa);
		libbitcoin::istream_reader source(va);
		//without witness 
		uint32_t versiona = source.read_4_bytes_little_endian();
		w.write_4_bytes_little_endian(versiona);
		input_todata(source, w);
		output_todata(source, w);
		uint32_t locktime = source.read_4_bytes_little_endian();
		uint32_t expir = source.read_4_bytes_little_endian();
		w.write_4_bytes_little_endian(locktime);
		w.write_4_bytes_little_endian(expir);
		
		for (int i = 0; i < trxs.size(); i++) {
			auto inter_trx = trxs[i];
			std::ostringstream inter_obj;
			libbitcoin::data_chunk inter_writ;
			libbitcoin::data_sink inter_ostream(inter_writ);
			libbitcoin::ostream_writer inter_w(inter_ostream);
			libbitcoin::data_chunk inter_aa = libbitcoin::config::base16(inter_trx);
			libbitcoin::data_source inter_va(inter_aa);
			libbitcoin::istream_reader inter_source(inter_va);
			//without witness 
			uint32_t inter_versiona = inter_source.read_4_bytes_little_endian();
			uint32_t inter_version_prefix = inter_versiona | (1 << 16);
			uint32_t inter_version_witness = inter_versiona | (3 << 16);
		
			input_todata(inter_source, inter_w);
			output_todata(inter_source, inter_w);
			uint32_t inter_locktime = inter_source.read_4_bytes_little_endian();
			uint32_t inter_expir = inter_source.read_4_bytes_little_endian();
			inter_ostream.flush();
			auto witness_count = inter_source.read_size_little_endian();

			//insert input:

			for (int i = 0; i < witness_count; i++) {
				auto ValueIn = inter_source.read_8_bytes_little_endian();
				auto BlockHeight = inter_source.read_4_bytes_little_endian();
				auto BlockIndex = inter_source.read_4_bytes_little_endian();
				auto signtureCount = inter_source.read_size_little_endian();

				auto SignatureScript = inter_source.read_bytes(signtureCount);
				libbitcoin::chain::script a(SignatureScript, false);

				if (a.size() >= 2) {
					bak_signature[i].push_back(a[0]);
				}
				


			}

		}
		auto witness_count = source.read_size_little_endian();
		w.write_size_little_endian(witness_count);
		//insert input:

		for (int i = 0; i < witness_count;i++) {
			auto ValueIn = source.read_8_bytes_little_endian();
			
			auto BlockHeight = source.read_4_bytes_little_endian();
			auto BlockIndex = source.read_4_bytes_little_endian();
			auto signtureCount = source.read_size_little_endian();
			w.write_8_bytes_little_endian(ValueIn);
			w.write_4_bytes_little_endian(BlockHeight);
			w.write_4_bytes_little_endian(BlockIndex);
			auto SignatureScript = source.read_bytes(signtureCount);
			libbitcoin::chain::script a(SignatureScript, false);
			libbitcoin::chain::script::operation::list new_op_list;
			
			for (const auto& one_data : bak_signature[i]) {	
				new_op_list.push_back(one_data);
			}
			new_op_list.push_back(a[1]);
			libbitcoin::chain::script new_script(new_op_list);

			w.write_size_little_endian(new_script.to_data(false).size());
			w.write_bytes(new_script.to_data(false));


			std::cout << "\"script\": \"" << new_script.to_string(0) << "\"," << std::endl;
			

		}

		ostream.flush();
		std::string signed_trx = libbitcoin::encode_base16(libbitcoin::data_slice(writ));
		return signed_trx;
		
		////insert output:
		//libbitcoin::data_source output_reader(output_writ);
		//libbitcoin::istream_reader output_source(output_reader);
		//auto OutputCount = output_source.read_size_little_endian();
		////std::cout << (uint64_t)OutputCount << std::endl;
		//for (uint64_t i = 0; i < OutputCount; ++i) {
		//	auto Value = output_source.read_8_bytes_little_endian();
		//	auto Version = output_source.read_2_bytes_little_endian();
		//	auto output_count = output_source.read_size_little_endian();
		//	auto PkScript = output_source.read_bytes(output_count);
		//	libbitcoin::chain::script a(PkScript, false);
		//	//std::cout << "out put script is " << a.to_string(libbitcoin::machine::all_rules) << std::endl;
		//	if (i > 0)
		//		obj << ",";
		//	obj << "\"output\": {";

		//	if (a.size() == 5)
		//	{

		//		if (a[0] == libbitcoin::machine::opcode::dup &&
		//			a[1] == libbitcoin::machine::opcode::hash160 &&
		//			a[3] == libbitcoin::machine::opcode::equalverify &&
		//			a[4] == libbitcoin::machine::opcode::checksig) {
		//			//auto pubkeyhash = a[2];
		//			fc::array<char, 26> addr;
		//			//test line
		//			//int version = 0x0f21;
		//			int version = 0x097f;
		//			addr.data[0] = char(version >> 8);
		//			addr.data[1] = (char)version;
		//			for (int i = 0; i < a[2].data().size(); i++)
		//			{
		//				addr.data[i + 2] = a[2].data()[i];
		//			}
		//			//memcpy(addr.data + 2, (char*)&(a[2].data()), a[2].data().size());
		//			auto check = HashR14(addr.data, addr.data + a[2].data().size() + 2);
		//			check = HashR14((char*)&check, (char*)&check + sizeof(check));
		//			memcpy(addr.data + 2 + a[2].data().size(), (char*)&check, 4);
		//			obj << "\"address\": \"" << fc::to_base58(addr.data, sizeof(addr)) << "\",";
		//			//std::cout << fc::to_base58(addr.data, sizeof(addr)) << std::endl;
		//		}
		//	}
		//	else if (a.size() == 3)
		//	{
		//		if (a[0] == libbitcoin::machine::opcode::hash160 &&
		//			a[2] == libbitcoin::machine::opcode::equal)
		//		{
		//			//auto pubkeyhash = a[1];
		//			fc::array<char, 26> addr;
		//			//test line
		//			//int version = 0x0efc;
		//			int version = 0x095a;
		//			addr.data[0] = char(version >> 8);
		//			addr.data[1] = (char)version;
		//			for (int i = 0; i < a[1].data().size(); i++)
		//			{
		//				addr.data[i + 2] = a[1].data()[i];
		//			}
		//			auto check = HashR14(addr.data, addr.data + a[1].data().size() + 2);
		//			check = HashR14((char*)&check, (char*)&check + sizeof(check));
		//			memcpy(addr.data + 2 + a[1].data().size(), (char*)&check, 4);
		//			//std::cout << fc::to_base58(addr.data, sizeof(addr)) << std::endl;
		//			obj << "\"address\": \"" << fc::to_base58(addr.data, sizeof(addr)) << "\",";
		//		}
		//	}
		//	obj << "\"script\": \"" << a.to_string(libbitcoin::machine::all_rules) << "\",";
		//	obj << "\"value\": " << Value << "}";
		//}
		//obj << "}}";

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
	std::string eth_privatekey::get_address() {
		FC_ASSERT(is_empty() == false, "private key is empty!");
		auto pubkey = get_private_key().get_public_key();
		auto dat = pubkey.serialize_ecc_point();
		Keccak tmp_addr;
		tmp_addr.add(dat.begin() + 1, dat.size() - 1);
		auto addr_str_keccaksha3 = tmp_addr.getHash();
		auto hex_str = addr_str_keccaksha3.substr(24, addr_str_keccaksha3.size());
		return  "0x" + fc::ripemd160(hex_str).str();
	}
	std::string eth_privatekey::get_public_key() {
		auto pubkey = get_private_key().get_public_key();
		auto dat = pubkey.serialize_ecc_point();
		return "0x" + fc::to_hex(dat.data,dat.size());
	}
	std::string eth_privatekey::get_address_by_pubkey(const std::string& pub) {
		FC_ASSERT(pub.size() > 2, "eth pubkey size error");
		const int size_of_data_to_hash = ((pub.size() - 2) / 2);
		//FC_ASSERT(((size_of_data_to_hash == 33) || (size_of_data_to_hash == 65)), "eth pubkey size error");
		FC_ASSERT(((pub.at(0) == '0') && (pub.at(1) == 'x')), "eth pubkey start error");
		std::string pubwithout(pub.begin() + 2, pub.end());
		char pubchar[1024];
		fc::from_hex(pubwithout, pubchar, size_of_data_to_hash);
		Keccak tmp_addr;
		tmp_addr.add(pubchar + 1, size_of_data_to_hash - 1);
		auto addr_str_keccaksha3 = tmp_addr.getHash();
		auto hex_str = addr_str_keccaksha3.substr(24, addr_str_keccaksha3.size());
		return  "0x" + fc::ripemd160(hex_str).str();
	}
	std::string  eth_privatekey::sign_message(const std::string& msg) {
		auto msgHash = msg.substr(2);
		dev::Secret sec(dev::jsToBytes(get_private_key().get_secret()));
		std::string prefix = "\x19";
		prefix += "Ethereum Signed Message:\n";
		std::vector<char> temp;
		unsigned int nDeplength = 0;
		bool b_converse = from_hex(msgHash.data(), temp, msgHash.size(), nDeplength);
		FC_ASSERT(b_converse);
		std::string msg_prefix = prefix +fc::to_string(temp.size())+std::string(temp.begin(),temp.end());//std::string(temp.begin(), temp.end());
		Keccak real_hash;
		real_hash.add(msg_prefix.data(), msg_prefix.size());
		dev::h256 hash;
		msgHash = real_hash.getHash();
		std::vector<char> temp1;
		nDeplength = 0;
		b_converse = from_hex(msgHash.data(), temp1, msgHash.size(), nDeplength);
		FC_ASSERT(b_converse);
		for (int i = 0; i < temp1.size(); ++i)
		{
			hash[i] = temp1[i];
		}
		auto sign_str = dev::sign(sec, hash);
		std::vector<char> sign_bin(sign_str.begin(), sign_str.end());
		std::string signs = BinToHex(sign_bin, false);
		if (signs.substr(signs.size() - 2) == "00")
		{
			signs[signs.size() - 2] = '1';
			signs[signs.size() - 1] = 'b';
		}
		else if (signs.substr(signs.size() - 2) == "01")
		{
			signs[signs.size() - 2] = '1';
			signs[signs.size() - 1] = 'c';
		}
		
		//formal eth sign v
		/*if (signs.substr(signs.size() - 2) == "00")
		{
			signs[signs.size() - 2] = '2';
			signs[signs.size() - 1] = '5';
		}
		else if (signs.substr(signs.size() - 2) == "01")
		{
			signs[signs.size() - 2] = '2';
			signs[signs.size() - 1] = '6';
		}*/
		
		return signs;
	}

	bool  eth_privatekey::verify_message(const std::string address,const std::string& msg,const std::string& signature) {
		auto msgHash = msg.substr(2);
		std::string prefix = "\x19";
		prefix += "Ethereum Signed Message:\n";
		std::vector<char> temp;
		unsigned int nDeplength = 0;
		bool b_converse = from_hex(msgHash.data(), temp, msgHash.size(), nDeplength);
		FC_ASSERT(b_converse);
		std::string msg_prefix = prefix + fc::to_string(temp.size()) + std::string(temp.begin(), temp.end());//std::string(temp.begin(), temp.end());
		Keccak real_hash;
		real_hash.add(msg_prefix.data(), msg_prefix.size());
		dev::h256 hash;
		msgHash = real_hash.getHash();
		std::vector<char> temp1;
		nDeplength = 0;
		b_converse = from_hex(msgHash.data(), temp1, msgHash.size(), nDeplength);
		FC_ASSERT(b_converse);
		for (int i = 0; i < temp1.size(); ++i)
		{
			hash[i] = temp1[i];
		}
		dev::Signature sig{ signature };
		sig[64] = sig[64] - 27;

		const auto& publickey = dev::recover(sig,hash);
		auto const& dev_addr = dev::toAddress(publickey);
		if (address == "0x"+dev_addr.hex())
			return true;
		return false;

		//formal eth sign v
		/*if (signs.substr(signs.size() - 2) == "00")
		{
			signs[signs.size() - 2] = '2';
			signs[signs.size() - 1] = '5';
		}
		else if (signs.substr(signs.size() - 2) == "01")
		{
			signs[signs.size() - 2] = '2';
			signs[signs.size() - 1] = '6';
		}*/
	}

	std::string  eth_privatekey::sign_trx(const std::string& raw_trx, int index) {
		auto eth_trx = raw_trx;

		std::vector<char> temp;
		unsigned int nDeplength = 0;
		bool b_converse = from_hex(eth_trx.data(), temp, eth_trx.size(), nDeplength);
		FC_ASSERT(b_converse);
		dev::bytes trx(temp.begin(), temp.end());
		dev::eth::TransactionBase trx_base(trx, dev::eth::CheckTransaction::None);
		dev::Secret sec(dev::jsToBytes(get_private_key().get_secret()));
		trx_base.sign(sec);
		auto signed_trx = trx_base.rlp(dev::eth::WithSignature);
		std::string signed_trx_str(signed_trx.begin(), signed_trx.end());
		std::vector<char> hex_trx(signed_trx.begin(), signed_trx.end());
		return BinToHex(hex_trx, false);
	}

	std::string eth_privatekey::get_wif_key() {
		/*char buf[1024];
		::memset(buf, 0, 1024);
		sprintf_s(buf, "%x", get_private_key().get_secret().data());
		std::string eth_pre_key(buf);*/
		return  get_private_key().get_secret().str();
	}
	fc::optional<fc::ecc::private_key>  eth_privatekey::import_private_key(const std::string& wif_key) {
		//char buf[1024];
		//::memset(buf, 0, 1024);
		//fc::from_hex(wif_key,buf,wif_key.size());
		fc::ecc::private_key_secret ad(wif_key);
		fc::ecc::private_key key = fc::variant(ad).as<fc::ecc::private_key>();
		set_key(key);
		return key;
	}
	fc::variant_object  eth_privatekey::decoderawtransaction(const std::string& trx)
	{
		std::vector<char> temp;
		unsigned int nDeplength = 0;
		bool b_converse = from_hex(trx.data(), temp, trx.size(), nDeplength);
		FC_ASSERT(b_converse);
		dev::bytes bintrx(temp.begin(), temp.end());
		dev::eth::TransactionBase trx_base(bintrx, dev::eth::CheckTransaction::Everything);
		fc::mutable_variant_object ret_obj;
		ret_obj.set("from","0x"+trx_base.from().hex());
		ret_obj.set("to", "0x"+trx_base.to().hex());
		ret_obj.set("nonce", trx_base.nonce().str());
		ret_obj.set("gasPrice", trx_base.gasPrice().str());
		ret_obj.set("gas", trx_base.gas().str());
		ret_obj.set("value", trx_base.value().str());
		auto bin = trx_base.sha3();
		std::vector<char> vectorBin(bin.begin(), bin.end());
		auto trx_id = "0x" + BinToHex(vectorBin, false);
		ret_obj.set("trxid", trx_id);
		std::vector<char> temp_trx(trx_base.data().begin(), trx_base.data().end());
		std::string hexinput = BinToHex(temp_trx, false);
		ret_obj.set("input", hexinput);
		return fc::variant_object(ret_obj);
	};
	std::string eth_privatekey::mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx) {
		if (raw_trx.contains("without_sign_trx_sign"))
		{
			auto eth_trx = raw_trx["without_sign_trx_sign"].as_string();
		
			std::vector<char> temp;
			unsigned int nDeplength = 0;
			bool b_converse = from_hex(eth_trx.data(), temp, eth_trx.size(), nDeplength);
			FC_ASSERT(b_converse);
			dev::bytes trx(temp.begin(), temp.end());
			dev::eth::TransactionBase trx_base(trx,dev::eth::CheckTransaction::None);

			dev::Secret sec(dev::jsToBytes(get_private_key().get_secret()));
			trx_base.sign(sec);
			auto signed_trx = trx_base.rlp(dev::eth::WithSignature);
			std::string signed_trx_str(signed_trx.begin(), signed_trx.end());
			std::vector<char> hex_trx(signed_trx.begin(), signed_trx.end());
			return BinToHex(hex_trx,false);
		}
		else if (raw_trx.contains("get_param_hash"))
		{
			//std::string cointype = raw_trx["get_param_hash"]["cointype"].as_string();
			std::string cointype = raw_trx["get_param_hash"]["cointype"].as_string();
			std::string msg_address = raw_trx["get_param_hash"]["msg_address"].as_string();
			std::string msg_amount = raw_trx["get_param_hash"]["msg_amount"].as_string();
			std::string msg_prefix = raw_trx["get_param_hash"]["msg_prefix"].as_string();
			std::string msg_to_hash = msg_address + msg_amount + cointype.substr(24);
			std::vector<char> temp;
			unsigned int nDeplength = 0;
			bool b_converse = from_hex(msg_to_hash.data(), temp, msg_to_hash.size(), nDeplength);
			FC_ASSERT(b_converse);
			Keccak messageHash;
			messageHash.add(msg_prefix.data(), msg_prefix.size());
			messageHash.add(temp.data(), temp.size());
			auto msgHash = messageHash.getHash();
			return sign_message("0x"+msgHash);
		}
		else {
			std::string msg_address = raw_trx["msg_address"].as_string();
			std::string msg_amount = raw_trx["msg_amount"].as_string();
			std::string msg_prefix = raw_trx["msg_prefix"].as_string();
			std::string sign_msg = msg_prefix + msg_address + msg_amount;
			return sign_message(sign_msg);
		}
		
	}
	bool eth_privatekey::validate_address(const std::string& addr) {
		FC_ASSERT(addr.size() == 42 && addr[0] == '0' && addr[1] == 'x');
		auto addr_str = addr.substr(2, addr.size());
		for (auto i = addr_str.begin(); i != addr_str.end(); ++i)
		{
			fc::from_hex(*i);
		}
		return true;
	}
	crosschain_management::crosschain_management()
	{
		crosschain_decode.insert(std::make_pair("BTC", &graphene::privatekey_management::btc_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("LTC", &graphene::privatekey_management::ltc_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("UB", &graphene::privatekey_management::ub_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("HC", &graphene::privatekey_management::hc_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("ETH", &graphene::privatekey_management::eth_privatekey::decoderawtransaction));
		crosschain_decode.insert(std::make_pair("ERC", &graphene::privatekey_management::eth_privatekey::decoderawtransaction));
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
		else if (name == "ETH") {
			auto itr = crosschain_prks.insert(std::make_pair(name, new eth_privatekey()));
			return itr.first->second;
		}
		else if (name.find("ERC") != name.npos){
			auto itr = crosschain_prks.insert(std::make_pair(name, new eth_privatekey()));
			return itr.first->second;
		}
		return nullptr;
	}

	fc::variant_object crosschain_management::decoderawtransaction(const std::string& raw_trx, const std::string& symbol)
	{
		try {
			std::string temp_symbol = symbol;
			if (symbol.find("ERC") != symbol.npos)
			{
				temp_symbol = "ERC";
			}
			auto iter = crosschain_decode.find(temp_symbol);
			FC_ASSERT(iter != crosschain_decode.end(), "plugin not exist.");
			return iter->second(raw_trx);
		}FC_CAPTURE_AND_RETHROW((raw_trx)(symbol))
	}


} } // end namespace graphene::privatekey_management
