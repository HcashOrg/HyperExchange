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
#include <fc/variant_object.hpp>
namespace graphene {
	namespace privatekey_management {

		#define COMMAND_BUF  1024

		class crosschain_privatekey_base
		{
		public:

			crosschain_privatekey_base();
			crosschain_privatekey_base(fc::ecc::private_key& priv_key);
			virtual std::string  sign_trx(const std::string& script, const std::string& raw_trx);
			virtual std::string  sign_message(const std::string& msg);
			fc::ecc::private_key  get_private_key();

			virtual std::string get_wif_key() = 0;
			virtual std::string get_address() = 0;
			virtual fc::optional<fc::ecc::private_key>  import_private_key(const std::string& wif_key) = 0;
			void generate();
			bool is_empty() const {	return _key == fc::ecc::private_key(); }
			virtual std::string get_public_key() = 0;
			virtual std::string get_address_by_pubkey(const std::string& pub) = 0;
			int  get_pubkey_prefix() { return _pubkey_prefix; }
			bool set_pubkey_prefix(int prefix = 0) { _pubkey_prefix = prefix; return true; }
			virtual std::string mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx) =0;
			int get_privkey_prefix() { return _privkey_prefix; }
			bool set_privkey_prefix(int prefix = 0)	{ _privkey_prefix = prefix; return true; }
			void set_key(fc::ecc::private_key& key) { _key = key; }
			int		get_id() { return _id; }
			bool	set_id(int p_id) { _id = p_id; return true; }
		private:
			fc::ecc::private_key  _key;

			int                   _id;
			int                   _pubkey_prefix;
			int					  _privkey_prefix;


		};


		class btc_privatekey : public crosschain_privatekey_base
		{
		public:
			btc_privatekey() { init(); };
			btc_privatekey(fc::ecc::private_key& priv_key) : crosschain_privatekey_base(priv_key) { init(); };
				                                                     

			virtual std::string get_wif_key() ;
			virtual std::string get_address() ;
			virtual std::string get_public_key();
			virtual std::string  sign_message(const std::string& msg);
			virtual std::string get_address_by_pubkey(const std::string& pub);
			virtual std::string mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx);
			virtual fc::optional<fc::ecc::private_key>  import_private_key(const std::string& wif_key) ;
			static  fc::variant_object  decoderawtransaction(const std::string& trx);
		private:
			void init();

		};



		class ltc_privatekey : public crosschain_privatekey_base
		{
		public:
			ltc_privatekey() { init(); };
			ltc_privatekey(fc::ecc::private_key& priv_key) : crosschain_privatekey_base(priv_key) { init(); };

			virtual std::string get_wif_key();
			virtual std::string get_address();
			virtual std::string get_public_key();
			virtual std::string get_address_by_pubkey(const std::string& pub);
			virtual std::string  sign_message(const std::string& msg);
			virtual std::string mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx);
			virtual fc::optional<fc::ecc::private_key>  import_private_key(const std::string& wif_key);
		private:
			void init();

		};

		class ub_privatekey : public crosschain_privatekey_base
		{
		public:
			ub_privatekey() { init(); };
			ub_privatekey(fc::ecc::private_key& priv_key) : crosschain_privatekey_base(priv_key) { init(); };

			virtual std::string get_wif_key();
			virtual std::string get_address();
			virtual std::string get_public_key();
			virtual std::string get_address_by_pubkey(const std::string& pub);
			virtual std::string  sign_message(const std::string& msg);
			virtual std::string mutisign_trx(const std::string& redeemscript, const fc::variant_object& raw_trx);
			virtual fc::optional<fc::ecc::private_key>  import_private_key(const std::string& wif_key);
		private:
			void init();

		};


		class crosschain_management
		{
		public:
			crosschain_management() {}
			~crosschain_management() {}
			static crosschain_management& get_instance()
			{
				static crosschain_management mgr;
				return mgr;
			}
			crosschain_privatekey_base * get_crosschain_prk(const std::string& name);
		private:
			std::map<std::string, crosschain_privatekey_base *> crosschain_prks;
		};
	}
} // end namespace graphene::privatekey_management


FC_REFLECT(graphene::privatekey_management::crosschain_privatekey_base,
	       (_key)
		   (_id)
	       (_pubkey_prefix)
	       (_privkey_prefix)
		  )