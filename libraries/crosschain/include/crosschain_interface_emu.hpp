#include <vector>
#include "crosschain_impl.hpp"

namespace graphene {
	namespace crosschain {

		class crosschain_interface_emu : public abstract_crosschain_interface
		{
		public:
			virtual ~crosschain_interface_emu() {}

			virtual void initialize_config(fc::variant_object &json_config);
			virtual void create_wallet(std::string wallet_name, std::string wallet_passprase);
			virtual bool unlock_wallet(std::string wallet_name, std::string wallet_passprase, uint32_t duration);
			virtual void close_wallet();
			virtual std::vector<std::string> wallet_list();
			virtual std::string create_normal_account(std::string account_name);
			virtual std::string create_multi_sig_account(std::vector<std::string> addresses);
			virtual std::vector<fc::variant_object> deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit);
			virtual fc::variant_object transaction_query(std::string trx_id);
			virtual fc::variant_object transfer(std::string &from_account, std::string &to_account, std::string &amount, std::string &symbol, std::string &memo, bool broadcast = true);
			virtual fc::variant_object create_multisig_transaction(std::string &from_account, std::string &to_account, std::string &amount, std::string &symbol, std::string &memo, bool broadcast = true);
			virtual fc::variant_object sign_multisig_transaction(fc::variant_object trx, std::string &sign_account, bool broadcast = true);
			virtual fc::variant_object merge_multisig_transaction(fc::variant_object trx, std::vector<fc::variant_object> signatures);
			virtual bool validate_transaction(fc::variant_object trx);
			virtual void broadcast_transaction(fc::variant_object trx);
			virtual std::vector<fc::variant_object> query_account_balance(std::string &account);
			virtual std::vector<fc::variant_object> transaction_history(std::string &user_account, uint32_t start_block, uint32_t limit);
			virtual std::string export_private_key(std::string &account, std::string &encrypt_passprase);
			virtual std::string import_private_key(std::string &account, std::string &encrypt_passprase);
			virtual std::string backup_wallet(std::string &wallet_name, std::string &encrypt_passprase);
			virtual std::string recover_wallet(std::string &wallet_name, std::string &encrypt_passprase);

		private:
			fc::variant_object config;
			std::vector<std::string> wallets;
			fc::variant_object transactions;
			std::set<std::string> trx_ids;
		};
	}
}
