/*
* Copyright (c) 2015 Cryptonomex, Inc., and contributors.
*
* The MIT License
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#pragma once

#include <graphene/app/application.hpp>

#include <boost/program_options.hpp>
#include <fc/io/json.hpp>
#include <graphene/app/plugin.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <fc/variant_object.hpp>
namespace graphene {
	namespace app {

		class abstract_cross_chain_plugin :public abstract_plugin
		{
		public:
			virtual ~abstract_cross_chain_plugin() {}

			virtual void initialize_config(fc::variant_object json_config) = 0;
			virtual bool create_wallet(string wallet_name, string wallet_passprase) =0;
			virtual bool unlock_wallet(string wallet_name, string wallet_passprase,uint32_t duration) = 0;
			virtual vector<string>  close_wallet() = 0;
			virtual void wallet_list() = 0;
			virtual std::string create_normal_account(string account_name) =0;
			virtual std::string create_multi_sig_account(std::string account_name, std::vector<std::string> addresses) = 0;
			virtual std::vector<fc::variant_object> deposit_transaction_query(string user_account,uint32_t from_block,uint32_t limit)=0;
			virtual fc::variant_object transaction_query(string trx_id) = 0;
			virtual fc::variant_object transfer(string from_account, string to_account, string amount, string symbol, string memo,bool broadcast=true) = 0;
			virtual fc::variant_object create_multisig_transaction(string from_account, string to_account, string amount, string symbol, string memo, bool broadcast = true) = 0;
			virtual fc::variant_object sign_multisig_transaction(fc::variant_object trx, string sign_account,bool broadcast=true) = 0;
			virtual fc::variant_object merge_multisig_transaction(fc::variant_object trx, std::vector<fc::variant_object> signatures) = 0;
			virtual bool validate_transaction(fc::variant_object trx) = 0;
			virtual void broadcast_transaction(fc::variant_object trx) = 0;
			virtual std::vector<fc::variant_object> query_account_balance(string account) = 0;
			virtual std::vector<fc::variant_object> transaction_history(string user_account, uint32_t start_block, uint32_t limit) = 0;
			virtual std::string export_private_key(string account,string encrypt_passprase) = 0;
			virtual std::string import_private_key(string account, string encrypt_passprase) = 0;
			virtual std::string backup_wallet(string wallet_name, string encrypt_passprase) = 0;
			virtual std::string recover_wallet(string wallet_name, string encrypt_passprase) = 0;


		};
	}
}