#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/contract_object.hpp>
#include <graphene/chain/native_contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_engine_builder.hpp>
#include <graphene/chain/uvm_chain_api.hpp>
#include <graphene/chain/database.hpp>
#include <memory>
#include <unordered_map>

namespace graphene {
	namespace chain {
		template<typename DerivedEvaluator>
		class contract_common_evaluate : public evaluator<DerivedEvaluator> {
		protected:
			std::vector<asset> gas_fees;
			std::shared_ptr<address> caller_address;
			std::shared_ptr<fc::ecc::public_key> caller_pubkey;
		public:
			virtual ~contract_common_evaluate(){}
			std::shared_ptr<address> get_caller_address() const
			{
				return caller_address;
			}
			std::shared_ptr<fc::ecc::public_key> get_caller_pubkey() const
			{
				return caller_pubkey;
			}
			StorageDataType get_storage(const string &contract_id, const string &storage_name) const
			{
				database& d = db();
				auto storage_data = d.get_contract_storage(address(contract_id), storage_name);
				return storage_data;
			}
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_name(const string &contract_name) const
			{
				if (!db().has_contract_of_name(contract_name))
					return nullptr;
				if (contract_name.empty())
					return nullptr;
				auto contract_info = std::make_shared<GluaContractInfo>();
				const auto &contract = db().get_contract_of_name(contract_name);
				// TODO: when contract is native contract
				const auto &code = contract.code;
				for (const auto & api : code.abi) {
					contract_info->contract_apis.push_back(api);
				}
				auto ccode = std::make_shared<uvm::blockchain::Code>();
				*ccode = code;
				return ccode;
			}
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_from_db_by_id(const string &contract_id) const
			{
				address contract_addr(contract_id);
				if (!db().has_contract(contract_addr))
					return nullptr;
				auto contract_info = std::make_shared<GluaContractInfo>();
				const auto &contract = db().get_contract(contract_addr);
				// TODO: when contract is native contract
				const auto &code = contract.code;
				for (const auto & api : code.abi) {
					contract_info->contract_apis.push_back(api);
				}
				auto ccode = std::make_shared<uvm::blockchain::Code>();
				*ccode = code;
				return ccode;
			}
			void do_apply_fees_balance(const address& caller_addr)
			{
				for (auto fee : gas_fees)
				{
					FC_ASSERT(fee.amount >= 0);
					asset fee_to_cost;
					fee_to_cost.asset_id = fee.asset_id;
					fee_to_cost.amount = -fee.amount;
					// db().adjust_balance(caller_addr, fee_to_cost); // FIXME: now account have no money
				}
			}
		};

		class contract_register_evaluate :public contract_common_evaluate<contract_register_evaluate> {
		private:
			gas_count_type gas_used;
			contract_register_operation origin_op;
			contract_object new_contract;
		public:
			// TODO: change to contract_invoke_result type
			std::unordered_map<std::string, std::unordered_map<std::string, StorageDataChangeType>> contracts_storage_changes;
		public:
			typedef contract_register_operation operation_type;

			void_result do_evaluate(const operation_type& o);
			void_result do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
			contract_object get_contract_by_name(const string& contract_name) const;
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			address origin_op_contract_id() const;
			void do_apply_balance();
		};

		class native_contract_register_evaluate :public contract_common_evaluate<native_contract_register_evaluate> {
		private:
			gas_count_type gas_used;
			native_contract_register_operation origin_op;
			contract_object new_contract;
		public:
			// TODO: change to contract_invoke_result type
			std::unordered_map<std::string, std::unordered_map<std::string, StorageDataChangeType>> contracts_storage_changes;
		public:
			typedef native_contract_register_operation operation_type;

			void_result do_evaluate(const operation_type& o);
			void_result do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
			contract_object get_contract_by_name(const string& contract_name) const;
			address origin_op_contract_id() const;
		};

		class contract_invoke_evaluate : public contract_common_evaluate<contract_invoke_evaluate> {
		private:
			gas_count_type gas_used;
			contract_invoke_operation origin_op;
            std::map<std::pair<address, asset_id_type>, share_type> contract_withdraw;
            std::map<std::pair<address, asset_id_type>, share_type> contract_balances;
            std::map<std::pair<address, asset_id_type>, share_type> deposit_to_address;
		public:
			std::unordered_map<std::string, std::unordered_map<std::string, StorageDataChangeType>> contracts_storage_changes;
		public:
			typedef contract_invoke_operation operation_type;

			void_result do_evaluate(const operation_type& o);
			void_result do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
			contract_object get_contract_by_name(const string& contract_name) const;
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
            void transfer_to_address(const address& contract,const asset& amount, const address& to);
            void do_apply_balance();
		};

		class contract_upgrade_evaluate : public contract_common_evaluate<contract_upgrade_evaluate> {
		private:
			gas_count_type gas_used;
			contract_upgrade_operation origin_op;
			std::map<std::pair<address, asset_id_type>, share_type> contract_withdraw;
			std::map<std::pair<address, asset_id_type>, share_type> contract_balances;
			std::map<address, asset> deposit_to_address;
		public:
			std::unordered_map<std::string, std::unordered_map<std::string, StorageDataChangeType>> contracts_storage_changes;
		public:
			typedef contract_upgrade_operation operation_type;

			void_result do_evaluate(const operation_type& o);
			void_result do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
			contract_object get_contract_by_name(const string& contract_name) const;
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			void transfer_to_address(const address& contract, const asset& amount, const address& to);
			void do_apply_balance();
		};

	}
}