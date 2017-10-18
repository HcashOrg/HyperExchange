#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <vector>
#include <graphene/crosschain/crosschain_impl.hpp>
#include <graphene/wallet/wallet.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/elliptic.hpp>

using boost::multi_index_container;
using namespace boost::multi_index;

namespace graphene {
	namespace crosschain {

		struct block_num {};
		struct trx_id {};
		struct to_addr {};

		typedef	multi_index_container<
			handle_history_trx,
			indexed_by<
			ordered_non_unique<
			tag<block_num>, BOOST_MULTI_INDEX_MEMBER(handle_history_trx, int64_t, block_num)>,
			ordered_non_unique<
			tag<trx_id>, BOOST_MULTI_INDEX_MEMBER(handle_history_trx, std::string, trx_id)>,
			ordered_non_unique<
			tag<to_addr>, BOOST_MULTI_INDEX_MEMBER(handle_history_trx, std::string, to_account)>
			>
		> transaction_emu_db;

		class crosschain_interface_emu : public abstract_crosschain_interface
		{
		public:
			crosschain_interface_emu() {}
			virtual ~crosschain_interface_emu() {}

			virtual void initialize_config(fc::variant_object &json_config) override ;
			virtual bool create_wallet(std::string wallet_name, std::string wallet_passprase) override ;
			virtual bool unlock_wallet(std::string wallet_name, std::string wallet_passprase, uint32_t duration) override ;
			virtual bool open_wallet(string wallet_name) override ;
			virtual void close_wallet() override ;
			virtual std::vector<std::string> wallet_list() override ;
			virtual std::string create_normal_account(std::string account_name) override ;
			virtual std::string create_multi_sig_account(std::string account_name, std::vector<std::string> addresses, uint32_t nrequired) override ;
			virtual std::vector<hd_trx> deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit) override ;
			virtual fc::variant_object transaction_query(std::string trx_id) override ;
			virtual fc::variant_object transfer(std::string &from_account, std::string &to_account, uint64_t amount, std::string &symbol, std::string &memo, bool broadcast = true) override ;
			virtual fc::variant_object create_multisig_transaction(std::string &from_account, std::string &to_account, uint64_t amount, std::string &symbol, std::string &memo, bool broadcast = true) override ;
			virtual std::string sign_multisig_transaction(fc::variant_object trx, std::string &sign_account, bool broadcast = true) override ;
			virtual fc::variant_object merge_multisig_transaction(fc::variant_object &trx, std::vector<std::string> signatures) override ;
			virtual bool validate_link_trx(const hd_trx &trx) override ;
			virtual bool validate_link_trx(const std::vector<hd_trx> &trx) override ;
			virtual bool validate_other_trx(const fc::variant_object &trx) override ;
			virtual bool validate_signature(const std::string &content, const std::string &signature);
			virtual void broadcast_transaction(const fc::variant_object &trx) override ;
			virtual std::vector<fc::variant_object> query_account_balance(const std::string &account) override ;
			virtual std::vector<fc::variant_object> transaction_history(std::string &user_account, uint32_t start_block, uint32_t limit) override ;
			virtual std::string export_private_key(std::string &account, std::string &encrypt_passprase) override ;
			virtual std::string import_private_key(std::string &account, std::string &encrypt_passprase) override ;
			virtual std::string backup_wallet(std::string &wallet_name, std::string &encrypt_passprase) override ;
			virtual std::string recover_wallet(std::string &wallet_name, std::string &encrypt_passprase) override ;


			bool load_wallet_file(std::string wallet_filename = "");
			void lock();
			bool is_locked();
			void save_wallet_file(std::string wallet_filename = "");
			void encrypt_keys();
			void enable_umask_protection();
			void disable_umask_protection();
			fc::ecc::private_key derive_private_key(const std::string& prefix_string,
				int sequence_number);




		private:
			fc::variant_object config;
			transaction_emu_db _transactions;
			std::map<std::string, uint64_t> _balances;
			std::string _plugin_wallet_filepath ;
			std::string _wallet_name;
			graphene::wallet::wallet_data _wallet;
			chain_id_type           _chain_id;
			fc::sha512              _checksum;
			map<address, std::string> _keys;
		};
	}
}

