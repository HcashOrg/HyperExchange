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

            gas_count_type gas_used;
            std::map<std::pair<address, asset_id_type>, share_type> contract_withdraw;
            std::map<std::pair<address, asset_id_type>, share_type> contract_balances;
            std::map<std::pair<address, asset_id_type>, share_type> deposit_to_address;
            std::map<std::pair<address, asset_id_type>, share_type> deposit_contract;
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
            void add_gas_fee(const asset& fee)
            {
                for (auto fee_it : gas_fees)
                {
                    if (fee_it.asset_id == fee.asset_id)
                    {
                        fee_it.amount += fee.amount;
                        return;
                    }   
                }
                gas_fees.push_back(fee);
            }
            void deposit_to_contract(const address& contract, const asset& amount)
            {
                share_type to_deposit = amount.amount;
                auto index = std::make_pair(contract, amount.asset_id);
                if (!db().has_contract(contract))
                    FC_CAPTURE_AND_THROW(blockchain::contract_engine::contract_not_exsited, (contract));
                auto withdraw=contract_withdraw.find(index);
                if(withdraw!= contract_withdraw.end())
                {
                    if (withdraw->second >= to_deposit)
                    {
                        withdraw->second -= to_deposit;
                        to_deposit = 0;
                    }
                    else
                    {
                        to_deposit -= withdraw->second;
                        withdraw->second = 0;
                    }
                }
                if (to_deposit == 0)
                    return;
                auto deposit = deposit_contract.find(index);
                if (deposit == deposit_contract.end())
                {
                    auto res = deposit_contract.insert(std::make_pair(index, 0));
                    if (res.second)
                    {
                        deposit = res.first;
                    }
                }
                deposit_contract[index] += to_deposit;
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
            void do_apply_balance()
            {
                for (auto to_contract = deposit_contract.begin(); to_contract != deposit_contract.end(); to_contract++)
                {
                    db().adjust_contract_balance(to_contract->first.first, asset(to_contract->second, to_contract->first.second));
                }
                for (auto to_withraw = contract_withdraw.begin(); to_withraw != contract_withdraw.end(); to_withraw++)
                {
                    db().adjust_contract_balance(to_withraw->first.first, asset(0 - to_withraw->second, to_withraw->first.second));
                }
                for (auto to_deposit = deposit_to_address.begin(); to_deposit != deposit_to_address.end(); to_deposit++)
                {
                    db().adjust_balance(to_deposit->first.first, asset(to_deposit->second, to_deposit->first.second));
                }
            }
            void transfer_to_address(const address& contract, const asset & amount, const address & to)
            {
                //withdraw
                share_type to_withdraw = amount.amount;
                if (!db().has_contract(contract))
                    FC_CAPTURE_AND_THROW(contract_not_exsited, (contract));
                std::pair<address, asset_id_type> index = std::make_pair(contract, amount.asset_id);
                auto balance = contract_balances.find(index);
                if (balance == contract_balances.end())
                {
                    auto res = contract_balances.insert(std::make_pair(index, db().get_contract_balance(index.first, index.second).amount));
                    if (res.second)
                    {
                        balance = res.first;
                    }
                }
                share_type all_balance = balance->second;
                auto deposit = deposit_contract.find(index);
                if (deposit != deposit_contract.end())
                {
                    if (deposit->second >= to_withdraw)
                    {
                        deposit->second -= to_withdraw;
                        to_withraw = 0;
                    }
                    else
                    {
                        to_withdraw -= deposit->second;
                        deposit->second = 0;
                    }
                }
                auto withdraw= contract_withdraw.find(index);
                if (withdraw != contract_withdraw.end())
                {
                    withdraw->second += to_withdraw;
                }
                else
                {
                    contract_withdraw.insert(std::make_pair(index, to_withdraw));
                }
                if (balance->second<withdraw->second)
                    FC_CAPTURE_AND_THROW(blockchain::contract_engine::contract_insufficient_balance, ("insufficient contract balance"));

                //deposit
                if (deposit_to_address.find(index) != deposit_to_address.end())
                    deposit_to_address[index] += amount.amount;
                else
                    deposit_to_address[index] = amount.amount;
                balance->second -= amount.amount;

            }
            share_type get_contract_balance(const address& contract, const asset_id_type& asset_id)
            {
                //balance= db_balance+deposit-withdraw
                share_type running_balance;
                //db_balance
                std::pair<address, asset_id_type> index = std::make_pair(contract, asset_id);
                auto balance = contract_balances.find(index);
                if (balance == contract_balances.end())
                {
                    auto res = contract_balances.insert(std::make_pair(index, db().get_contract_balance(index.first, index.second).amount));
                    if (res.second)
                    {
                        balance = res.first;
                    }
                }
                running_balance = balance->second;

                //deposit
                auto deposit = deposit_contract.find(index);
                if (deposit != deposit_contract.end())
                {
                    running_balance += deposit->second;
                }

                //withdraw
                auto withdraw = contract_withdraw.find(index);
                if (withdraw != contract_withdraw.end())
                {
                    running_balance -= withdraw->second;
                }
                return running_balance;
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
		};

		class contract_upgrade_evaluate : public contract_common_evaluate<contract_upgrade_evaluate> {
		private:
			gas_count_type gas_used;
			contract_upgrade_operation origin_op;
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
		};

        class contract_transfer_evaluate : public contract_common_evaluate<contract_transfer_evaluate> {
        private:
            transfer_contract_operation origin_op;

        public:
            std::unordered_map<std::string, std::unordered_map<std::string, StorageDataChangeType>> contracts_storage_changes;
        public:
            typedef transfer_contract_operation operation_type;

            void_result do_evaluate(const operation_type& o);
            void_result do_apply(const operation_type& o);


            std::shared_ptr<GluaContractInfo> get_contract_by_id(const string &contract_id) const;
            contract_object get_contract_by_name(const string& contract_name) const;
            std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
        };

	}
}