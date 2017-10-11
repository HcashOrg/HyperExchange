#include "crosschain_interface_emu.hpp"


namespace graphene {
	namespace crosschain {


		void crosschain_interface_emu::initialize_config(fc::variant_object &json_config)
		{

		}

		void crosschain_interface_emu::create_wallet(std::string wallet_name, std::string wallet_passprase) {}

		bool crosschain_interface_emu::unlock_wallet(std::string wallet_name, std::string wallet_passprase, uint32_t duration)
		{
			return false;
		}

		void crosschain_interface_emu::close_wallet() {}

		std::vector<std::string> crosschain_interface_emu::wallet_list()
		{
			return std::vector<std::string>();
		}

		std::string crosschain_interface_emu::create_normal_account(std::string account_name)
		{
			return std::string();
		}

		std::string crosschain_interface_emu::create_multi_sig_account(std::vector<std::string> addresses)
		{
			return std::string();
		}

		std::vector<fc::variant_object> crosschain_interface_emu::deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit)
		{
			return std::vector<fc::variant_object>();
		}

		fc::variant_object crosschain_interface_emu::transaction_query(std::string trx_id)
		{
			return fc::variant_object();
		}

		fc::variant_object crosschain_interface_emu::transfer(std::string &from_account, std::string &to_account, std::string &amount, std::string &symbol, std::string &memo, bool broadcast)
		{
			return fc::variant_object();
		}

		fc::variant_object crosschain_interface_emu::create_multisig_transaction(std::string &from_account, std::string &to_account, std::string &amount, std::string &symbol, std::string &memo, bool broadcast)
		{
			return fc::variant_object();
		}

		fc::variant_object crosschain_interface_emu::sign_multisig_transaction(fc::variant_object trx, std::string &sign_account, bool broadcast)
		{
			return fc::variant_object();
		}

		fc::variant_object crosschain_interface_emu::merge_multisig_transaction(fc::variant_object trx, std::vector<fc::variant_object> signatures)
		{
			return fc::variant_object();
		}

		bool crosschain_interface_emu::validate_transaction(fc::variant_object trx)
		{
			return false;
		}

		void crosschain_interface_emu::broadcast_transaction(fc::variant_object trx)
		{

		}

		std::vector<fc::variant_object> crosschain_interface_emu::query_account_balance(std::string &account)
		{
			return std::vector<fc::variant_object>();
		}

		std::vector<fc::variant_object> crosschain_interface_emu::transaction_history(std::string &user_account, uint32_t start_block, uint32_t limit)
		{
			return std::vector<fc::variant_object>();
		}

		std::string crosschain_interface_emu::export_private_key(std::string &account, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_emu::import_private_key(std::string &account, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_emu::backup_wallet(std::string &wallet_name, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_emu::recover_wallet(std::string &wallet_name, std::string &encrypt_passprase)
		{
			return std::string();
		}

	}
}
