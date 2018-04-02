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

#include <string>
#include <vector>
#include <fc/variant_object.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/crosschain_privatekey_management/private_key.hpp>
#include <graphene/utilities/string_escape.hpp>
namespace graphene {
	namespace crosschain {
		typedef struct handle_history_trx {
			std::string trx_id;
			std::string from_account;
			std::string to_account;
			std::string amount;
			std::string asset_symbol;
			int64_t block_num;
		}hd_trx;
		
		typedef struct crosschain_balance {
			std::string account;
			graphene::chain::asset balance;
		};

		class abstract_crosschain_interface
		{
		public:
			virtual ~abstract_crosschain_interface() {}

			// Initialize with a JSON object.
			virtual void initialize_config(fc::variant_object &json_config) = 0;
			virtual bool valid_config() = 0;
			// Create a wallet with given name and optional protect-password.
			virtual bool create_wallet(std::string wallet_name, std::string wallet_passprase) =0;

			//´ò¿ªÇ®°ü
			virtual bool open_wallet(std::string wallet_name) = 0;

			// Unlock wallet before operating it.
			virtual bool unlock_wallet(std::string wallet_name, std::string wallet_passprase,uint32_t duration) = 0;

			// Close wallet.
			virtual void close_wallet() = 0;

			// List existed local wallets by name.
			virtual std::vector<std::string> wallet_list() = 0;

			// Create a new address.
			virtual std::string create_normal_account(std::string account_name) =0;

			// Create a multi-signed account.
			virtual std::map<std::string,std::string> create_multi_sig_account(std::string account_name, std::vector<std::string> addresses, uint32_t nrequired) = 0;

			// Query transactions to given address.
			virtual std::vector<hd_trx> deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit) = 0;

			// Query transaction details by transaction id.
			virtual fc::variant_object transaction_query(std::string trx_id) = 0;

			// Transfer asset.
			virtual fc::variant_object transfer(const std::string &from_account, const std::string &to_account, uint64_t amount, const std::string &symbol, const std::string &memo, bool broadcast = true) = 0;

			// Create transaction from multi-signed account.
			virtual fc::variant_object create_multisig_transaction(std::string &from_account, std::string &to_account, const std::string& amount, std::string &symbol, std::string &memo, bool broadcast = true) = 0;
			// Create transaction from multi-signed account.
			virtual fc::variant_object create_multisig_transaction(const std::string &from_account, const std::map<std::string,std::string> dest_info, const std::string &symbol, const std::string &memo) = 0;

			// Get signature for a given transaction.
			virtual std::string sign_multisig_transaction(fc::variant_object trx, graphene::privatekey_management::crosschain_privatekey_base*& sign_key, const std::string& redeemScript, bool broadcast = true) = 0;

			// Merge all signatures into on transaction.
			virtual fc::variant_object merge_multisig_transaction(fc::variant_object &trx, std::vector<std::string> signatures) = 0;

			// Validate transaction.
			virtual bool validate_link_trx(const hd_trx &trx) = 0;
			virtual bool validate_link_trx(const std::vector<hd_trx> &trx) = 0;
			virtual bool validate_other_trx(const fc::variant_object &trx) = 0;
			virtual bool validate_address(const std::string& addr) = 0;
			//Turn plugin transaction to handle transaction
			virtual hd_trx turn_trx(const fc::variant_object & trx) = 0;
			virtual std::map<std::string, graphene::crosschain::hd_trx> turn_trxs(const fc::variant_object & trx) = 0;
			// Validate signature. Now it is used to validate signature of multi-signed addresses.
			virtual bool validate_signature(const std::string &account, const std::string &content, const std::string &signature) = 0;

			// Sign for the content.
			virtual bool create_signature(graphene::privatekey_management::crosschain_privatekey_base*& sign_key, const std::string &content, std::string &signature) = 0;

			// Broadcast transaction.
			virtual void broadcast_transaction(const fc::variant_object &trx) = 0;

			// Query account balance.
			virtual std::vector<fc::variant_object> query_account_balance(const std::string &account) = 0;

			// Query transactions of given account.
			virtual std::vector<fc::variant_object> transaction_history(std::string symbol, const std::string &user_account, uint32_t start_block, uint32_t limit,uint32_t& end_block_num) = 0;

			// Export private key.
			virtual std::string export_private_key(std::string &account, std::string &encrypt_passprase) = 0;

			// Import private key.
			virtual std::string import_private_key(std::string &account, std::string &encrypt_passprase) = 0;

			// Backup wallet (include all privte keys).
			virtual std::string backup_wallet(std::string &wallet_name, std::string &encrypt_passprase) = 0;

			// Recover wallet.
			virtual std::string recover_wallet(std::string &wallet_name, std::string &encrypt_passprase) = 0;

		};
	}
}
FC_REFLECT(graphene::crosschain::handle_history_trx, (trx_id)(from_account)(to_account)(amount)(asset_symbol)(block_num))
