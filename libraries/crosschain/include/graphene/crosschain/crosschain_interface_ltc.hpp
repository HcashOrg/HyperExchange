#include <vector>
#include <string>
#include <graphene/crosschain/crosschain_impl.hpp>
#include <fc/network/http/connection.hpp>


namespace graphene {
	namespace crosschain {


		class crosschain_interface_ltc : public abstract_crosschain_interface
		{
		public:
			crosschain_interface_ltc() {}
			virtual ~crosschain_interface_ltc() {}
			virtual bool valid_config();
			virtual void initialize_config(fc::variant_object &json_config) override;
			virtual bool create_wallet(std::string wallet_name, std::string wallet_passprase) override;
			virtual bool unlock_wallet(std::string wallet_name, std::string wallet_passprase, uint32_t duration) override;
			virtual bool open_wallet(std::string wallet_name) override;
			virtual void close_wallet() override;
			virtual std::vector<std::string> wallet_list() override;
			virtual std::string create_normal_account(std::string account_name) override;
			virtual std::map<std::string,std::string> create_multi_sig_account(std::string account_name, std::vector<std::string> addresses, uint32_t nrequired) override;
			virtual std::vector<hd_trx> deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit) override;
			virtual fc::variant_object transaction_query(std::string trx_id) override;
			virtual fc::variant_object transfer(std::string &from_account, std::string &to_account, uint64_t amount, std::string &symbol, std::string &memo, bool broadcast = true) override;
			virtual fc::variant_object create_multisig_transaction(std::string &from_account, std::string &to_account, const std::string& amount, std::string &symbol, std::string &memo, bool broadcast = true) override;
			virtual fc::variant_object create_multisig_transaction(std::string &from_account, const std::map<std::string, std::string> dest_info, std::string &symbol, std::string &memo, const std::string& prk) override;
			virtual std::string sign_multisig_transaction(fc::variant_object trx, std::string &sign_account,const std::string& redeemScript, bool broadcast = true) override;
			virtual fc::variant_object merge_multisig_transaction(fc::variant_object &trx, std::vector<std::string> signatures) override;
			virtual bool validate_link_trx(const hd_trx &trx) override;
			virtual bool validate_link_trx(const std::vector<hd_trx> &trx) override;
			virtual bool validate_other_trx(const fc::variant_object &trx) override;
			virtual bool validate_signature(const std::string &account, const std::string &content, const std::string &signature) override;
			virtual bool create_signature(const std::string &account, const std::string &content, std::string &signature, const std::string& prk) override;
			virtual hd_trx turn_trx(const fc::variant_object & trx) override;
			virtual std::map<std::string, graphene::crosschain::hd_trx> turn_trxs(const fc::variant_object & trx)override;
			virtual void broadcast_transaction(const fc::variant_object &trx) override;
			virtual std::vector<fc::variant_object> query_account_balance(const std::string &account) override;
			virtual std::vector<fc::variant_object> transaction_history(std::string symbol,std::string &user_account, uint32_t start_block, uint32_t limit, uint32_t& end_block_num) override;
			virtual std::string export_private_key(std::string &account, std::string &encrypt_passprase) override;
			virtual std::string import_private_key(std::string &account, std::string &encrypt_passprase) override;
			virtual std::string backup_wallet(std::string &wallet_name, std::string &encrypt_passprase) override;
			virtual std::string recover_wallet(std::string &wallet_name, std::string &encrypt_passprase) override;


		private:
			fc::variant_object _config;
			std::string _plugin_wallet_filepath;
			std::string _wallet_name;
			std::string _rpc_method;
			std::string _rpc_url;
			const std::string chain_type = "LTC";
			fc::http::headers _rpc_headers;
		};
	}
}

