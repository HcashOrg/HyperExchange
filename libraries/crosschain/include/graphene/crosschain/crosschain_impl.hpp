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

namespace graphene {
	namespace crosschain {
		typedef struct handle_history_trx {
			std::string trx_id;
			std::vector<std::string> from_account;
			std::vector<std::string> to_account;
			std::string amount;
			std::string fee;
			std::string memo;
			std::string asset_sympol;
			int64_t block_num;
		}hd_trx;
		class abstract_crosschain_interface
		{
		public:
			virtual ~abstract_crosschain_interface() {}

			// Initialize with a JSON object.
			virtual void initialize_config(fc::variant_object &json_config) = 0;

			// Create a wallet with given name and optional protect-password.
			virtual bool create_wallet(std::string wallet_name, std::string wallet_passprase) =0;

			//´ò¿ªÇ®°ü
			virtual bool open_wallet(string wallet_name) = 0;

			// Unlock wallet before operating it.
			virtual bool unlock_wallet(std::string wallet_name, std::string wallet_passprase,uint32_t duration) = 0;

			// Close wallet.
			virtual void close_wallet() = 0;

			// List existed local wallets by name.
			virtual std::vector<std::string> wallet_list() = 0;

			// Create a new address.
			virtual std::string create_normal_account(std::string account_name) =0;

			// Create a multi-signed account.
			virtual std::string create_multi_sig_account(std::string account_name, std::vector<std::string> addresses, uint32_t nrequired) = 0;

			// Query transactions to given address.
			virtual std::vector<fc::variant_object> deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit) = 0;

			// Query transaction details by transaction id.
			virtual fc::variant_object transaction_query(std::string trx_id) = 0;

			// Transfer asset.
			virtual fc::variant_object transfer(std::string &from_account, std::string &to_account, std::string &amount, std::string &symbol, std::string &memo, bool broadcast = true) = 0;

			// Create transaction from multi-signed account.
			virtual fc::variant_object create_multisig_transaction(std::string &from_account, std::string &to_account, std::string &amount, std::string &symbol, std::string &memo, bool broadcast = true) = 0;

			// Get signature for a given transaction.
			virtual fc::variant_object sign_multisig_transaction(fc::variant_object trx, std::string &sign_account, bool broadcast = true) = 0;

			// Merge all signatures into on transaction.
			virtual fc::variant_object merge_multisig_transaction(fc::variant_object trx, std::vector<fc::variant_object> signatures) = 0;

			// Validate transaction.
			virtual bool validate_transaction(fc::variant_object trx) = 0;

			// Broadcast transaction.
			virtual void broadcast_transaction(fc::variant_object trx) = 0;

			// Query account balance.
			virtual std::vector<fc::variant_object> query_account_balance(std::string &account) = 0;

			// Query transactions of given account.
			virtual std::vector<fc::variant_object> transaction_history(std::string &user_account, uint32_t start_block, uint32_t limit) = 0;

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
FC_REFLECT(graphene::crosschain::handle_history_trx, (trx_id)(from_account)(to_account)(amount)(fee)(memo)(asset_sympol)(block_num))