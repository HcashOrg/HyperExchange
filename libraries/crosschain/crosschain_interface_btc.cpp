#include <graphene/crosschain/crosschain_interface_btc.hpp>


namespace graphene {
	namespace crosschain {


		void crosschain_interface_btc::initialize_config(fc::variant_object &json_config)
		{

		}

		bool crosschain_interface_btc::unlock_wallet(std::string wallet_name, std::string wallet_passprase, uint32_t duration)
		{
			return false;
		}

		bool crosschain_interface_btc::open_wallet(string wallet_name)
		{
			return false;
		}

		void crosschain_interface_btc::close_wallet()
		{
			
		}

		std::vector<std::string> crosschain_interface_btc::wallet_list()
		{
			return std::vector<std::string>();
		}

		bool graphene::crosschain::crosschain_interface_btc::create_wallet(std::string wallet_name, std::string wallet_passprase)
		{
			return false;
		}

		std::string crosschain_interface_btc::create_normal_account(std::string account_name)
		{
			return std::string();
		}
		
		std::string crosschain_interface_btc::create_multi_sig_account(std::string account_name, std::vector<std::string> addresses, uint32_t nrequired)
		{
			return std::string();
		}

		std::vector<graphene::crosschain::hd_trx> crosschain_interface_btc::deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit)
		{
			return std::vector<graphene::crosschain::hd_trx>();
		}

		fc::variant_object crosschain_interface_btc::transaction_query(std::string trx_id)
		{
			return fc::variant_object();
		}

		fc::variant_object crosschain_interface_btc::transfer(std::string &from_account, std::string &to_account, uint64_t amount, std::string &symbol, std::string &memo, bool broadcast /*= true*/)
		{
			return fc::variant_object();
		}

		fc::variant_object crosschain_interface_btc::create_multisig_transaction(std::string &from_account, std::string &to_account, uint64_t amount, std::string &symbol, std::string &memo, bool broadcast /*= true*/)
		{
			return fc::variant_object();
		}

		std::string crosschain_interface_btc::sign_multisig_transaction(fc::variant_object trx, std::string &sign_account, bool broadcast /*= true*/)
		{
			return std::string();
		}

		fc::variant_object crosschain_interface_btc::merge_multisig_transaction(fc::variant_object &trx, std::vector<std::string> signatures)
		{
			return fc::variant_object();
		}

		bool crosschain_interface_btc::validate_link_trx(const hd_trx &trx)
		{
			return false;
		}

		bool crosschain_interface_btc::validate_link_trx(const std::vector<hd_trx> &trx)
		{
			return false;
		}

		bool crosschain_interface_btc::validate_other_trx(const fc::variant_object &trx)
		{
			return false;
		}

		bool crosschain_interface_btc::validate_signature(const std::string &content, const std::string &signature)
		{
			return false;
		}

		bool crosschain_interface_btc::create_signature(const std::string &account, const std::string &content, const std::string &signature)
		{
			return false;
		}

		graphene::crosschain::hd_trx crosschain_interface_btc::turn_trx(const fc::variant_object & trx)
		{
			return graphene::crosschain::hd_trx();
		}

		void crosschain_interface_btc::broadcast_transaction(const fc::variant_object &trx)
		{

		}

		std::vector<fc::variant_object> crosschain_interface_btc::query_account_balance(const std::string &account)
		{
			return std::vector<fc::variant_object>();
		}

		std::vector<fc::variant_object> crosschain_interface_btc::transaction_history(std::string &user_account, uint32_t start_block, uint32_t limit)
		{
			return std::vector<fc::variant_object>();
		}

		std::string crosschain_interface_btc::export_private_key(std::string &account, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_btc::import_private_key(std::string &account, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_btc::backup_wallet(std::string &wallet_name, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_btc::recover_wallet(std::string &wallet_name, std::string &encrypt_passprase)
		{
			return std::string();
		}



	}
}