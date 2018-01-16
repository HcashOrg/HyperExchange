#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/account_object.hpp>

#include <memory>
#include <unordered_map>

namespace graphene {
	namespace chain {
		class contract_register_evaluate :public evaluator<contract_register_evaluate> {
		private:
			gas_count_type gas_used;
			contract_register_operation origin_op;
		public:
			std::unordered_map<std::string, std::unordered_map<std::string, StorageDataChangeType>> contracts_storage_changes;
		public:
			typedef contract_register_operation operation_type;

			void_result do_evaluate(const contract_register_operation& o);
			void_result do_apply(const contract_register_operation& o);

			virtual void pay_fee() override;

			std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			address origin_op_contract_id() const;
			StorageDataType get_storage(const string &contract_id, const string &storage_name) const;
		};
	}
}