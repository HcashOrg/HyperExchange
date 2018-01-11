#include <graphene/chain/contract_evaluate.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/contract_engine_builder.hpp>
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

		void_result contract_register_evaluate::do_evaluate(const contract_register_operation& o) {


			// TODO: execute contract init api in pendingState
			::blockchain::contract_engine::ContractEngineBuilder builder;
			auto engine = builder.build();
			int exception_code = 0;
			string exception_msg;
			try {
				FC_ASSERT(result_operations.size() == 0);

				GluaStateValue statevalue;
				statevalue.pointer_value = this;
				engine->set_caller((string)(o.owner_addr), (string)(o.owner_addr)); // FIXME: first is owner publickey
																							//engine->set_state_pointer_value("evaluate_state", &eval_state);
				engine->clear_exceptions();
				auto limit = o.init_cost;
				if (limit < 0 || limit == 0)
					FC_CAPTURE_AND_THROW(blockchain::contract_engine::uvm_executor_internal_error);

				engine->set_gas_limit(limit);
				result_operations.resize(0);
				// TODO: push contract_info result operation(with apis, offline_apis, etc.)

				//eval_state.p_result_trx.push_transaction(eval_state.trx);
				//eval_state.p_result_trx.expiration = eval_state.trx.expiration;
				//eval_state.p_result_trx.operations.push_back(ContractInfoOperation(get_contract_id(), owner, contract_code, register_time));
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
			// TODO: commit contract result to db
			return void_result();
		}

		void contract_register_evaluate::pay_fee() {

		}
	}
}
