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
		class contract_common_evaluate  {
		protected:
			std::vector<asset> gas_fees;
            generic_evaluator* gen_eval=nullptr;
			std::shared_ptr<address> caller_address;
			std::shared_ptr<fc::ecc::public_key> caller_pubkey;

            gas_count_type gas_used;
			contract_invoke_result invoke_contract_result;
            //balances
            std::map<std::pair<address, asset_id_type>, share_type> contract_withdraw;
            std::map<std::pair<address, asset_id_type>, share_type> contract_balances;
            std::map<std::pair<address, asset_id_type>, share_type> deposit_to_address;
            std::map<std::pair<address, asset_id_type>, share_type> deposit_contract;
            //storages
        public:
            std::unordered_map<std::string, std::unordered_map<std::string, StorageDataChangeType>> contracts_storage_changes;

            contract_common_evaluate(generic_evaluator* gen_eval);
            virtual ~contract_common_evaluate();
            std::shared_ptr<address> get_caller_address() const;
            std::shared_ptr<fc::ecc::public_key> get_caller_pubkey() const;
            database& get_db() const;
            StorageDataType get_storage(const string &contract_id, const string &storage_name) const;
            std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_name(const string &contract_name) const;
            asset asset_from_sting(const string& symbol, const string& amount);
            std::shared_ptr<uvm::blockchain::Code> get_contract_code_from_db_by_id(const string &contract_id) const;
            void add_gas_fee(const asset& fee);
            void undo_balance_contract_effected();
            void deposit_to_contract(const address& contract, const asset& amount);
            void do_apply_fees_balance(const address& caller_addr);
            void do_apply_balance();
            transaction_id_type get_current_trx_id() const;
            void do_apply_contract_event_notifies();
            void transfer_to_address(const address& contract, const asset & amount, const address & to);
            share_type get_contract_balance(const address& contract, const asset_id_type& asset_id);
			void emit_event(const address& contract_addr, const string& event_name, const string& event_arg);
			virtual share_type origin_op_fee() const = 0;
            virtual  std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const=0;
            virtual contract_object get_contract_by_name(const string& contract_name) const=0;
            virtual std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;;
		};

		class contract_register_evaluate :public evaluator<contract_register_evaluate>,public contract_common_evaluate{
		private:
			gas_count_type gas_used;
			contract_register_operation origin_op;
			contract_object new_contract;
		public:
            contract_register_evaluate():contract_common_evaluate(this){}
			typedef contract_register_operation operation_type;

			void_result do_evaluate(const operation_type& o);
			void_result do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
			contract_object get_contract_by_name(const string& contract_name) const;
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			address origin_op_contract_id() const;
			virtual share_type origin_op_fee() const;
		};

		class native_contract_register_evaluate :public evaluator<native_contract_register_evaluate>, public contract_common_evaluate {
		private:
			gas_count_type gas_used;
			native_contract_register_operation origin_op;
			contract_object new_contract;
		public:
            native_contract_register_evaluate(): contract_common_evaluate(this) {}
		public:
			typedef native_contract_register_operation operation_type;

			void_result do_evaluate(const operation_type& o);
			void_result do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
			contract_object get_contract_by_name(const string& contract_name) const;
			address origin_op_contract_id() const;
			virtual share_type origin_op_fee() const;
		};

		class contract_invoke_evaluate : public evaluator<contract_invoke_evaluate>, public contract_common_evaluate {
		private:
			gas_count_type gas_used;
			contract_invoke_operation origin_op;
    	public:
			typedef contract_invoke_operation operation_type;
            contract_invoke_evaluate() : contract_common_evaluate(this) {}
			void_result do_evaluate(const operation_type& o);
			void_result do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
			contract_object get_contract_by_name(const string& contract_name) const;
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			virtual share_type origin_op_fee() const;
		};

		class contract_upgrade_evaluate : public evaluator<contract_upgrade_evaluate>, public contract_common_evaluate {
		private:
			gas_count_type gas_used;
			contract_upgrade_operation origin_op;
		public:
			typedef contract_upgrade_operation operation_type;

            contract_upgrade_evaluate() : contract_common_evaluate(this) {}
			void_result do_evaluate(const operation_type& o);
			void_result do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
			contract_object get_contract_by_name(const string& contract_name) const;
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			virtual share_type origin_op_fee() const;
		};

        class contract_transfer_evaluate : public evaluator<contract_transfer_evaluate>, public contract_common_evaluate {
        private:
            transfer_contract_operation origin_op;

        public:
            typedef transfer_contract_operation operation_type;

            contract_transfer_evaluate() : contract_common_evaluate(this) {}
            void_result do_evaluate(const operation_type& o);
            void_result do_apply(const operation_type& o);


            std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
            contract_object get_contract_by_name(const string& contract_name) const;
            std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			virtual share_type origin_op_fee() const;
        };

	}
}