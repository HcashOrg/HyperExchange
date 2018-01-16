#include <graphene/chain/contract_evaluate.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/contract_engine_builder.hpp>
#include <graphene/chain/uvm_chain_api.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/transaction_object.hpp>

#include <fc/array.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <boost/uuid/sha1.hpp>
#include <exception>

namespace graphene {
	namespace chain {

		using uvm::lua::api::global_uvm_chain_api;

		void_result contract_register_evaluate::do_evaluate(const contract_register_operation& o) {

			if (!global_uvm_chain_api)
				global_uvm_chain_api = new UvmChainApi();

			// TODO: execute contract init api in pendingState
			::blockchain::contract_engine::ContractEngineBuilder builder;
			auto engine = builder.build();
			int exception_code = 0;
			string exception_msg;
			try {

				origin_op = o;
				engine->set_caller((string)(o.owner_addr), (string)(o.owner_addr)); // FIXME: first is owner publickey
				engine->set_state_pointer_value("register_evaluate_state", this);
				engine->clear_exceptions();
				auto limit = o.init_cost;
				if (limit < 0 || limit == 0)
					FC_CAPTURE_AND_THROW(blockchain::contract_engine::uvm_executor_internal_error);

				engine->set_gas_limit(limit);
				contracts_storage_changes.clear();
				try
				{
					engine->execute_contract_init_by_address((string)o.contract_id, "", nullptr);
				}
				catch (uvm::core::GluaException &e)
				{
					throw e; // TODO: change to other error type
				}

				gas_used = engine->gas_used();
				FC_ASSERT(gas_used <= o.init_cost && gas_used > 0, "costs of execution can be only between 0 and init_cost");

				//	ShareType required = get_amount_sum(exec_cost, eval_state._current_state->get_default_margin().amount);
				//	required = get_amount_sum(required, register_fee);
				//	required = get_amount_sum(required, transaction_fee.amount);

				//	map<BalanceIdType, ShareType> withdraw_map;
				//	withdraw_enough_balances(balances, required, withdraw_map);
				//	eval_state.p_result_trx.operations.push_back(BalancesWithdrawOperation(withdraw_map));
				//	eval_state.p_result_trx.operations.push_back(DepositContractOperation(get_contract_id(), eval_state._current_state->get_default_margin(), deposit_contract_margin));//todo 保证金存入
				
				// TODO: withdraw from owner and deposit margin balance to contract

			}
			catch (std::exception &e)
			{
				throw e; // TODO
			}
			/*catch (contract_run_out_of_money& e)
			{
			if (!eval_state.evaluate_contract_testing)
			{
			if (eval_state.throw_exec_exception)
			FC_CAPTURE_AND_THROW(hsrcore::blockchain::contract_run_out_of_money);

			eval_state.p_result_trx.operations.resize(0);
			eval_state.p_result_trx.push_transaction(eval_state.trx);
			eval_state.p_result_trx.expiration = eval_state.trx.expiration;
			map<BalanceIdType, ShareType> withdraw_map;
			required = get_amount_sum(register_fee, transaction_fee.amount);
			required = get_amount_sum(required, initcost.amount);

			withdraw_enough_balances(balances, required, withdraw_map);
			eval_state.p_result_trx.operations.push_back(BalancesWithdrawOperation(withdraw_map));
			}
			else
			FC_CAPTURE_AND_THROW(hsrcore::blockchain::contract_run_out_of_testing_money);

			}
			catch (const contract_error& e)
			{
			if (!eval_state.evaluate_contract_testing)
			{
			if (eval_state.throw_exec_exception)
			FC_CAPTURE_AND_THROW(hsrcore::blockchain::contract_execute_error, (exception_msg));
			Asset exec_cost = eval_state._current_state->get_amount(engine->gas_used());
			std::map<BalanceIdType, ShareType> withdraw_map;
			withdraw_enough_balances(balances, (exec_cost + eval_state.required_fees).amount, withdraw_map);
			eval_state.p_result_trx.operations.resize(1);
			eval_state.p_result_trx.expiration = eval_state.trx.expiration;
			eval_state.p_result_trx.operations.push_back(BalancesWithdrawOperation(withdraw_map));
			}
			else
			FC_CAPTURE_AND_THROW(hsrcore::blockchain::contract_execute_error_in_testing, (exception_msg));

			}*/

			return void_result();
		}

		void_result contract_register_evaluate::do_apply(const contract_register_operation& o) {
			database& d = db();
			// commit contract result to db
			for (const auto &pair1 : contracts_storage_changes)
			{
				const auto &contract_id = pair1.first;
				const auto &contract_storage_changes = pair1.second;
				for (const auto &pair2 : contract_storage_changes)
				{
					const auto &storage_name = pair2.first;
					const auto &change = pair2.second;
					d.set_contract_storage(address(contract_id), storage_name, change);
				}
			}
			return void_result();
		}

		void contract_register_evaluate::pay_fee() {

		}

		std::shared_ptr<GluaContractInfo> contract_register_evaluate::get_contract_by_id(const string &contract_id) const
		{
			if (string(origin_op.contract_id) == contract_id)
			{
				auto contract_info = std::make_shared<GluaContractInfo>();
				const auto &code = origin_op.contract_code;
				for (const auto & api : code.abi) {
					contract_info->contract_apis.push_back(api);
				}
				return contract_info;
			}
			else
			{
				return nullptr;
			}
		}

		std::shared_ptr<uvm::blockchain::Code> contract_register_evaluate::get_contract_code_by_id(const string &contract_id) const
		{
			if (string(origin_op.contract_id) == contract_id)
			{
				auto code = std::make_shared<uvm::blockchain::Code>();
				*code = origin_op.contract_code;
				return code;
			}
			else
			{
				return nullptr;
			}
		}

		address contract_register_evaluate::origin_op_contract_id() const
		{
			return origin_op.contract_id;
		}

		StorageDataType contract_register_evaluate::get_storage(const string &contract_id, const string &storage_name) const
		{
			database& d = db();
			auto storage_data = d.get_contract_storage(address(contract_id), storage_name);
			return storage_data;
		}

	}
}
