#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/contract_object.hpp>
#include <graphene/chain/native_contract.hpp>
#include <memory>
#include <unordered_map>

namespace graphene {
	namespace chain {
		class contract_register_evaluate :public evaluator<contract_register_evaluate> {
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
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			address origin_op_contract_id() const;
			StorageDataType get_storage(const string &contract_id, const string &storage_name) const;
		};

		class native_contract_register_evaluate :public evaluator<native_contract_register_evaluate> {
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
			address origin_op_contract_id() const;
			StorageDataType get_storage(const string &contract_id, const string &storage_name) const;
		};

		class contract_invoke_evaluate :public evaluator<contract_invoke_evaluate> {
		private:
			gas_count_type gas_used;
			contract_invoke_operation origin_op;
            std::map<std::pair<address, asset_id_type>,share_type> contract_withdraw;
            std::map<std::pair<address, asset_id_type>,share_type> contract_balances;
            std::map<address, asset> deposit_to_address;
		public:
			std::unordered_map<std::string, std::unordered_map<std::string, StorageDataChangeType>> contracts_storage_changes;
		public:
			typedef contract_invoke_operation operation_type;

			void_result do_evaluate(const operation_type& o);
			void_result do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			StorageDataType get_storage(const string &contract_id, const string &storage_name) const;
            void transfer_to_address(const address& contract,const asset& amount, const address& to);
            void do_apply_balance();
		};

	}
}