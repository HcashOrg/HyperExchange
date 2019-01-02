#include <graphene/chain/guard_refund_balance_evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/transaction_object.hpp>

namespace graphene {
	namespace chain {
		void_result guard_refund_balance_evaluator::do_apply(const guard_refund_balance_operation& o)
		{
			try
			{
				database& d = db();
			  //adjust the balance of refund_addr
				const auto& crosschain_trxs = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto iter = crosschain_trxs.find(transaction_id_type(o.txid));
				auto op = iter->real_transaction.operations[0];
				auto asset_id = op.get<crosschain_withdraw_operation>().asset_id;

				auto amount = op.get<crosschain_withdraw_operation>().amount;
				auto obj = d.get(asset_id);
				const auto refund_addr = o.refund_addr;
				d.adjust_balance(refund_addr, obj.amount_from_string(amount));
				d.modify(*iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_canceled;
					obj.block_num = d.head_block_num();
				});
				d.modify(d.get(asset_id_type(asset_id)).dynamic_asset_data_id(d), [=](asset_dynamic_data_object& d) {
					d.current_supply += obj.amount_from_string(amount).amount;
				});

			}FC_CAPTURE_AND_RETHROW((o))
		}

		void_result guard_refund_balance_evaluator::do_evaluate(const guard_refund_balance_operation& o)
		{
			try
			{
				const database& d = db();
				//if status of last crosschain transaction is Failed,
				//refund, and need change the status of that status
				//TODO
				const auto& crosschain_trxs = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto iter = crosschain_trxs.find(transaction_id_type(o.txid));
				FC_ASSERT( iter != crosschain_trxs.end(),"transaction not exist.");
				FC_ASSERT(iter->trx_state == withdraw_without_sign_trx_uncreate, "refund if only crosschain transaction is not created.");
				FC_ASSERT(iter->real_transaction.operations.size() == 1, "operation size error");
				auto op = iter->real_transaction.operations[0];
				FC_ASSERT(op.which() == operation::tag<crosschain_withdraw_operation>::value, "operation type error");
				auto trx = iter->real_transaction;
				flat_set<public_key_type> keys = trx.get_signature_keys(d.get_chain_id());
				flat_set<address> addrs;
				for (auto key : keys)
				{
					addrs.insert(address(key));
				}
				FC_ASSERT(addrs.find(o.refund_addr) != addrs.end() ,"withdraw is not created by this account.");
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result guard_refund_crosschain_trx_evaluator::do_evaluate(const guard_refund_crosschain_trx_operation& o) {
			try {
				const database& d = db();
				const auto& guard_db = d.get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_id);
				FC_ASSERT(guard_iter != guard_db.end(),"cant find this guard");
				const auto& account_db = d.get_index_type<account_index>().indices().get<by_id>();
				auto account_iter = account_db.find(guard_iter->guard_member_account);
				FC_ASSERT(account_iter != account_db.end(), "cant find this account");
				FC_ASSERT(account_iter->addr == o.guard_address, "guard address error");
				const auto& trx_db = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto iter = trx_db.find(o.not_enough_sign_trx_id);
				FC_ASSERT(iter != trx_db.end(), "transaction not exist.");
				FC_ASSERT(iter->trx_state == withdraw_without_sign_trx_create, "cross chain trx state error");
				FC_ASSERT(iter->real_transaction.operations.size() == 1, "operation size error");
				auto op = iter->real_transaction.operations[0];
				FC_ASSERT(op.which() == operation::tag<crosschain_withdraw_without_sign_operation>::value, "operation type error");
				auto without_sign_op = op.get<crosschain_withdraw_without_sign_operation>();
				auto asset_symbol = without_sign_op.asset_symbol;
				const asset_object&   asset_type = without_sign_op.asset_id(d);
				FC_ASSERT(asset_symbol == asset_type.symbol);
				for (const auto& op : without_sign_op.ccw_trx_ids) {
					const auto source_trx_iter = trx_db.find(op);
					FC_ASSERT(source_trx_iter != trx_db.end(), "source trx exist error");
					FC_ASSERT(source_trx_iter->real_transaction.operations.size() == 1, "source trx operation size error");
					auto op1 = source_trx_iter->real_transaction.operations[0];
					auto source_op = op1.get<crosschain_withdraw_operation>();
					FC_ASSERT(asset_symbol == source_op.asset_symbol);
					FC_ASSERT(without_sign_op.asset_id == source_op.asset_id);
				}
			}FC_CAPTURE_AND_RETHROW((o))			
		}
		void_result guard_refund_crosschain_trx_evaluator::do_apply(const guard_refund_crosschain_trx_operation& o) {
			try {
				database& d = db();
				auto& crosschain_trxs = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto iter = crosschain_trxs.find(transaction_id_type(o.not_enough_sign_trx_id));
				auto crosschain_sign_trxs_range = d.get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>().equal_range(iter->transaction_id);
				for (auto sign_trx : boost::make_iterator_range(crosschain_sign_trxs_range.first, crosschain_sign_trxs_range.second)) {
					auto sign_iter = crosschain_trxs.find(sign_trx.transaction_id);
					d.modify(*sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_canceled;
						obj.block_num = d.head_block_num();
					});
				}
				auto op = iter->real_transaction.operations[0];
				auto without_sign_op = op.get<crosschain_withdraw_without_sign_operation>();
				const asset_object&   asset_type = without_sign_op.asset_id(d);
				for (const auto& source_trx_id : without_sign_op.ccw_trx_ids) {
					auto source_iter = crosschain_trxs.find(source_trx_id);
					auto op = source_iter->real_transaction.operations[0];
					auto source_op = op.get<crosschain_withdraw_operation>();
					d.adjust_balance(source_op.withdraw_account, asset(asset_type.amount_from_string(source_op.amount).amount, source_op.asset_id));
					d.modify(*source_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_canceled;
						obj.block_num = d.head_block_num();
					});
					d.modify(asset_type.dynamic_asset_data_id(d), [&asset_type,source_op](asset_dynamic_data_object& d) {
						d.current_supply += asset_type.amount_from_string(source_op.amount).amount;
					});
				}
				d.modify(*iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_canceled;
					obj.block_num = d.head_block_num();
				});
				d.modify(asset_type.dynamic_asset_data_id(d), [&asset_type,without_sign_op](asset_dynamic_data_object& d) {
					d.current_supply -= without_sign_op.crosschain_fee.amount;
				});
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result guard_cancel_combine_trx_evaluator::do_evaluate(const guard_cancel_combine_trx_operation& o) {
			try {
				const database& d = db();
				
				const auto& guard_db = d.get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_id);
				FC_ASSERT(guard_iter->senator_type == PERMANENT);
				FC_ASSERT(guard_iter != guard_db.end(), "cant find this guard");
				const auto& account_db = d.get_index_type<account_index>().indices().get<by_id>();
				auto account_iter = account_db.find(guard_iter->guard_member_account);
				FC_ASSERT(account_iter != account_db.end(), "cant find this account");
				FC_ASSERT(account_iter->addr == o.guard_address, "guard address error");
				const auto& trx_db = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto iter = trx_db.find(o.fail_trx_id);
				FC_ASSERT(iter != trx_db.end(), "transaction not exist.");
				FC_ASSERT(iter->trx_state == withdraw_combine_trx_create, "cross chain trx state error");
				FC_ASSERT(iter->real_transaction.operations.size() == 1, "operation size error");
				const auto& trx_history_db = d.get_index_type<trx_index>().indices().get<by_trx_id>();
				const auto trx_history_iter = trx_history_db.find(o.fail_trx_id);
				FC_ASSERT(trx_history_iter != trx_history_db.end());
				auto current_blockNum = d.get_dynamic_global_properties().head_block_number;
				FC_ASSERT(trx_history_iter->block_num + 720 < current_blockNum);
				auto without_iter = trx_db.find(iter->relate_transaction_id);
				FC_ASSERT(without_iter != trx_db.end(), "without transaction not exist.");
				FC_ASSERT(without_iter->real_transaction.operations.size() == 1, "operation size error");
				auto op = without_iter->real_transaction.operations[0];
				FC_ASSERT(op.which() == operation::tag<crosschain_withdraw_without_sign_operation>::value, "operation type error");
				auto without_sign_op = op.get<crosschain_withdraw_without_sign_operation>();
				auto asset_symbol = without_sign_op.asset_symbol;
				const asset_object&   asset_type = without_sign_op.asset_id(d);
				FC_ASSERT(asset_symbol == asset_type.symbol);
				for (const auto& op : without_sign_op.ccw_trx_ids) {
					const auto source_trx_iter = trx_db.find(op);
					FC_ASSERT(source_trx_iter != trx_db.end(), "source trx exist error");
					FC_ASSERT(source_trx_iter->real_transaction.operations.size() == 1, "source trx operation size error");
					auto op1 = source_trx_iter->real_transaction.operations[0];
					auto source_op = op1.get<crosschain_withdraw_operation>();
					FC_ASSERT(asset_symbol == source_op.asset_symbol);
					FC_ASSERT(without_sign_op.asset_id == source_op.asset_id);
				}
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result guard_cancel_combine_trx_evaluator::do_apply(const guard_cancel_combine_trx_operation& o) {
			try {
				database& d = db();
				auto& crosschain_trxs = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto iter = crosschain_trxs.find(transaction_id_type(o.fail_trx_id));
				
				auto without_iter = crosschain_trxs.find(iter->relate_transaction_id);
				auto crosschain_sign_trxs_range = d.get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>().equal_range(without_iter->transaction_id);
				for (auto sign_trx : boost::make_iterator_range(crosschain_sign_trxs_range.first, crosschain_sign_trxs_range.second)) {
					auto sign_iter = crosschain_trxs.find(sign_trx.transaction_id);
					d.modify(*sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_canceled;
						obj.block_num = d.head_block_num();
					});
				}
				auto op = without_iter->real_transaction.operations[0];
				auto without_sign_op = op.get<crosschain_withdraw_without_sign_operation>();
				const asset_object&   asset_type = without_sign_op.asset_id(d);
				for (const auto& source_trx_id : without_sign_op.ccw_trx_ids) {
					auto source_iter = crosschain_trxs.find(source_trx_id);
					auto op = source_iter->real_transaction.operations[0];
					auto source_op = op.get<crosschain_withdraw_operation>();
					d.adjust_balance(source_op.withdraw_account, asset(asset_type.amount_from_string(source_op.amount).amount, source_op.asset_id));
					d.modify(*source_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_canceled;
						obj.block_num = d.head_block_num();
					});
					d.modify(asset_type.dynamic_asset_data_id(d), [&asset_type, source_op](asset_dynamic_data_object& d) {
						d.current_supply += asset_type.amount_from_string(source_op.amount).amount;
					});
				}
				d.modify(*iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_canceled;
					obj.block_num = d.head_block_num();
				});
				d.modify(*without_iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_canceled;
					obj.block_num = d.head_block_num();
				});
				d.modify(asset_type.dynamic_asset_data_id(d), [&asset_type, without_sign_op](asset_dynamic_data_object& d) {
					d.current_supply -= without_sign_op.crosschain_fee.amount;
				});
			}FC_CAPTURE_AND_RETHROW((o))
		}
#define PASS_HX_BLOCK_NUM 220000
		void_result senator_pass_success_trx_evaluate::do_evaluate(const senator_pass_success_trx_operation& o) {
			try {
				const database& d = db();
				auto current_blockNum = db().head_block_num();
				FC_ASSERT(current_blockNum >= PASS_HX_BLOCK_NUM, "You cant use this function now");
				const auto& guard_db = d.get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_id);
				FC_ASSERT(guard_iter->senator_type == PERMANENT);
				FC_ASSERT(guard_iter != guard_db.end(), "cant find this guard");
				const auto& account_db = d.get_index_type<account_index>().indices().get<by_id>();
				auto account_iter = account_db.find(guard_iter->guard_member_account);
				FC_ASSERT(account_iter != account_db.end(), "cant find this account");
				FC_ASSERT(account_iter->addr == o.guard_address, "guard address error");
				const auto& trx_db = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto iter = trx_db.find(o.pass_transaction_id);
				FC_ASSERT(iter != trx_db.end(), "transaction not exist.");
				FC_ASSERT(iter->trx_state == withdraw_combine_trx_create, "cross chain trx state error");
				FC_ASSERT(iter->real_transaction.operations.size() == 1, "operation size error");
				FC_ASSERT(iter->real_transaction.operations[0].which() == operation::tag<crosschain_withdraw_combine_sign_operation>::value);
				/*
				const auto& trx_history_db = d.get_index_type<trx_index>().indices().get<by_trx_id>();
				const auto trx_history_iter = trx_history_db.find(o.pass_transaction_id);
				FC_ASSERT(trx_history_iter != trx_history_db.end());
				auto current_blockNum = d.get_dynamic_global_properties().head_block_number;
				FC_ASSERT(trx_history_iter->block_num + 20 < current_blockNum);
				*/
				auto without_iter = trx_db.find(iter->relate_transaction_id);
				FC_ASSERT(without_iter != trx_db.end(), "without transaction not exist.");
				FC_ASSERT(without_iter->real_transaction.operations.size() == 1, "operation size error");
				auto op = without_iter->real_transaction.operations[0];
				FC_ASSERT(op.which() == operation::tag<crosschain_withdraw_without_sign_operation>::value, "operation type error");
				auto without_sign_op = op.get<crosschain_withdraw_without_sign_operation>();
				auto asset_symbol = without_sign_op.asset_symbol;
				const asset_object&   asset_type = without_sign_op.asset_id(d);
				FC_ASSERT(asset_symbol == asset_type.symbol);
				for (const auto& op : without_sign_op.ccw_trx_ids) {
					const auto source_trx_iter = trx_db.find(op);
					FC_ASSERT(source_trx_iter != trx_db.end(), "source trx exist error");
					FC_ASSERT(source_trx_iter->real_transaction.operations.size() == 1, "source trx operation size error");
					auto op1 = source_trx_iter->real_transaction.operations[0];
					auto source_op = op1.get<crosschain_withdraw_operation>();
					FC_ASSERT(asset_symbol == source_op.asset_symbol);
					FC_ASSERT(without_sign_op.asset_id == source_op.asset_id);
				}
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result senator_pass_success_trx_evaluate::do_apply(const senator_pass_success_trx_operation& o) {
			try {
				database& d = db();
				auto& crosschain_trxs = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto iter = crosschain_trxs.find(transaction_id_type(o.pass_transaction_id));

				auto without_iter = crosschain_trxs.find(iter->relate_transaction_id);
				auto crosschain_sign_trxs_range = d.get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>().equal_range(without_iter->transaction_id);
				for (auto sign_trx : boost::make_iterator_range(crosschain_sign_trxs_range.first, crosschain_sign_trxs_range.second)) {
					auto sign_iter = crosschain_trxs.find(sign_trx.transaction_id);
					d.modify(*sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_transaction_confirm;
						obj.block_num = d.head_block_num();
					});
				}
				auto op = without_iter->real_transaction.operations[0];
				auto without_sign_op = op.get<crosschain_withdraw_without_sign_operation>();
				const asset_object&   asset_type = without_sign_op.asset_id(d);
				for (const auto& source_trx_id : without_sign_op.ccw_trx_ids) {
					auto source_iter = crosschain_trxs.find(source_trx_id);
					auto op = source_iter->real_transaction.operations[0];
					auto source_op = op.get<crosschain_withdraw_operation>();
					//d.adjust_balance(source_op.withdraw_account, asset(asset_type.amount_from_string(source_op.amount).amount, source_op.asset_id));
					d.modify(*source_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_transaction_confirm;
						obj.block_num = d.head_block_num();
					});
					/*
					d.modify(asset_type.dynamic_asset_data_id(d), [&asset_type, source_op](asset_dynamic_data_object& d) {
						d.current_supply += asset_type.amount_from_string(source_op.amount).amount;
					});*/
				}
				d.modify(*iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_transaction_confirm;
					obj.block_num = d.head_block_num();
				});
				d.modify(*without_iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_transaction_confirm;
					obj.block_num = d.head_block_num();
				});
				
				/*
				d.modify(asset_type.dynamic_asset_data_id(d), [&asset_type, without_sign_op](asset_dynamic_data_object& d) {
					d.current_supply -= without_sign_op.crosschain_fee.amount;
				});*/
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result eths_cancel_unsigned_transaction_evaluator::do_evaluate(const eths_cancel_unsigned_transaction_operation& o) {
			try {
				const database& d = db();
		
				const auto& guard_db = d.get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_id);
				FC_ASSERT(guard_iter->senator_type == PERMANENT);
				FC_ASSERT(guard_iter != guard_db.end(), "cant find this guard");
				const auto& account_db = d.get_index_type<account_index>().indices().get<by_id>();
				auto account_iter = account_db.find(guard_iter->guard_member_account);
				FC_ASSERT(account_iter != account_db.end(), "cant find this account");
				FC_ASSERT(account_iter->addr == o.guard_address, "guard address error");
				const auto& trx_db = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto iter = trx_db.find(o.cancel_trx_id);
				FC_ASSERT(iter != trx_db.end(), "transaction not exist.");
				FC_ASSERT(iter->trx_state == withdraw_eth_guard_need_sign, "cross chain trx state error");
				FC_ASSERT(iter->real_transaction.operations.size() == 1, "operation size error");
				auto combine_op = iter->real_transaction.operations[0];
				FC_ASSERT(combine_op.which() == operation::tag<crosschain_withdraw_combine_sign_operation>::value, "operation type error 1");
				const auto& trx_history_db = d.get_index_type<trx_index>().indices().get<by_trx_id>();
				const auto trx_history_iter = trx_history_db.find(o.cancel_trx_id);
				FC_ASSERT(trx_history_iter != trx_history_db.end());
				auto current_blockNum = d.get_dynamic_global_properties().head_block_number;
				FC_ASSERT(trx_history_iter->block_num + 720 < current_blockNum);
				auto without_iter = trx_db.find(iter->relate_transaction_id);
				FC_ASSERT(without_iter != trx_db.end(), "without transaction not exist.");
				FC_ASSERT(without_iter->real_transaction.operations.size() == 1, "operation size error");
				auto op = without_iter->real_transaction.operations[0];
				FC_ASSERT(op.which() == operation::tag<crosschain_withdraw_without_sign_operation>::value, "operation type error");
				auto without_sign_op = op.get<crosschain_withdraw_without_sign_operation>();
				auto asset_symbol = without_sign_op.asset_symbol;
				const asset_object&   asset_type = without_sign_op.asset_id(d);
				FC_ASSERT(asset_symbol == asset_type.symbol);
				for (const auto& op : without_sign_op.ccw_trx_ids) {
					const auto source_trx_iter = trx_db.find(op);
					FC_ASSERT(source_trx_iter != trx_db.end(), "source trx exist error");
					FC_ASSERT(source_trx_iter->real_transaction.operations.size() == 1, "source trx operation size error");
					auto op1 = source_trx_iter->real_transaction.operations[0];
					auto source_op = op1.get<crosschain_withdraw_operation>();
					FC_ASSERT(asset_symbol == source_op.asset_symbol);
					FC_ASSERT(without_sign_op.asset_id == source_op.asset_id);
				}
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result eths_cancel_unsigned_transaction_evaluator::do_apply(const eths_cancel_unsigned_transaction_operation& o) {
			try {
				database& d = db();
				auto& crosschain_trxs = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto iter = crosschain_trxs.find(transaction_id_type(o.cancel_trx_id));

				auto without_iter = crosschain_trxs.find(iter->relate_transaction_id);
				auto crosschain_sign_trxs_range = d.get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>().equal_range(without_iter->transaction_id);
				for (auto sign_trx : boost::make_iterator_range(crosschain_sign_trxs_range.first, crosschain_sign_trxs_range.second)) {
					auto sign_iter = crosschain_trxs.find(sign_trx.transaction_id);
					d.modify(*sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_sign_trx;
					});
				}
				auto op = without_iter->real_transaction.operations[0];
				auto without_sign_op = op.get<crosschain_withdraw_without_sign_operation>();
				const asset_object&   asset_type = without_sign_op.asset_id(d);
				for (const auto& source_trx_id : without_sign_op.ccw_trx_ids) {
					auto source_iter = crosschain_trxs.find(source_trx_id);
					auto op = source_iter->real_transaction.operations[0];
					auto source_op = op.get<crosschain_withdraw_operation>();
					//d.adjust_balance(source_op.withdraw_account, asset(asset_type.amount_from_string(source_op.amount).amount, source_op.asset_id));
					d.modify(*source_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_without_sign_trx_create;
					});
					//d.modify(asset_type.dynamic_asset_data_id(d), [&asset_type, source_op](asset_dynamic_data_object& d) {
					//	d.current_supply += asset_type.amount_from_string(source_op.amount).amount;
					//});
				}
				
				//d.modify(*iter, [&](crosschain_trx_object& obj) {
				//	obj.trx_state = withdraw_canceled;
				//});
				d.modify(*without_iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_without_sign_trx_create;
				});
				d.remove(*iter);
				//d.modify(asset_type.dynamic_asset_data_id(d), [&asset_type, without_sign_op](asset_dynamic_data_object& d) {
				//	d.current_supply -= without_sign_op.crosschain_fee.amount;
				//});
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result eth_cancel_fail_crosschain_trx_evaluate::do_evaluate(const eth_cancel_fail_crosschain_trx_operation& o) {
			try {
				const database& d = db();
				const auto& guard_db = d.get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_id);
				FC_ASSERT(guard_iter != guard_db.end(), "cant find this guard");
				FC_ASSERT(guard_iter->senator_type == PERMANENT);
				const auto& account_db = d.get_index_type<account_index>().indices().get<by_id>();
				auto account_iter = account_db.find(guard_iter->guard_member_account);
				FC_ASSERT(account_iter != account_db.end(), "cant find this account");
				FC_ASSERT(account_iter->addr == o.guard_address, "guard address error");
				const auto& trx_db = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto iter = trx_db.find(o.fail_transaction_id);
				FC_ASSERT(iter != trx_db.end(), "transaction not exist.");
				FC_ASSERT(iter->trx_state == withdraw_eth_guard_sign, "cross chain trx state error");
				FC_ASSERT(iter->real_transaction.operations.size() == 1, "operation size error");
				const auto& trx_history_db = d.get_index_type<trx_index>().indices().get<by_trx_id>();
				const auto trx_history_iter = trx_history_db.find(o.fail_transaction_id);
				FC_ASSERT(trx_history_iter != trx_history_db.end());
				auto current_blockNum = d.get_dynamic_global_properties().head_block_number;
				FC_ASSERT(trx_history_iter->block_num + 720 < current_blockNum);
				auto op = iter->real_transaction.operations[0];
				FC_ASSERT(op.which() == operation::tag<eths_guard_sign_final_operation>::value, "operation type error");
				auto eths_guard_sign_final_op = op.get<eths_guard_sign_final_operation>();
				auto without_iter = trx_db.find(iter->relate_transaction_id);
				FC_ASSERT(without_iter != trx_db.end(), "without transaction not exist.");
				auto without_op = without_iter->real_transaction.operations[0];
				auto without_sign_op = without_op.get<crosschain_withdraw_without_sign_operation>();
				auto asset_symbol = without_sign_op.asset_symbol;
				const asset_object&   asset_type = without_sign_op.asset_id(d);
				FC_ASSERT(asset_symbol == "ETH" || asset_symbol.find("ERC") != asset_symbol.npos);
				FC_ASSERT(asset_symbol == asset_type.symbol);
				for (const auto& op : without_sign_op.ccw_trx_ids) {
					const auto source_trx_iter = trx_db.find(op);
					FC_ASSERT(source_trx_iter != trx_db.end(), "source trx exist error");
					FC_ASSERT(source_trx_iter->real_transaction.operations.size() == 1, "source trx operation size error");
					auto op1 = source_trx_iter->real_transaction.operations[0];
					auto source_op = op1.get<crosschain_withdraw_operation>();
					FC_ASSERT(asset_symbol == source_op.asset_symbol);
					FC_ASSERT(without_sign_op.asset_id == source_op.asset_id);
				}
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(eths_guard_sign_final_op.chain_type))
					return void_result();
				auto hdl = manager.get_crosschain_handle(std::string(eths_guard_sign_final_op.chain_type));
				if (!hdl->valid_config())
					return void_result();
				std::string eth_fail_transaction_id = eths_guard_sign_final_op.signed_crosschain_trx_id;
				if (eths_guard_sign_final_op.signed_crosschain_trx_id.find('|') != eths_guard_sign_final_op.signed_crosschain_trx_id.npos) {
					auto pos = eths_guard_sign_final_op.signed_crosschain_trx_id.find('|');
					eth_fail_transaction_id = eths_guard_sign_final_op.signed_crosschain_trx_id.substr(0,pos);
				}
				auto eth_transaction = hdl->transaction_query(eth_fail_transaction_id);
				FC_ASSERT(eth_transaction.contains("respit_trx"));
				FC_ASSERT(eth_transaction.contains("source_trx"));
				auto respit_trx = eth_transaction["respit_trx"].get_object();
				auto source_trx = eth_transaction["source_trx"].get_object();
				FC_ASSERT(respit_trx.contains("logs"));
				FC_ASSERT(respit_trx.contains("gasUsed"));
				FC_ASSERT(source_trx.contains("gas"));
				auto receipt_logs = respit_trx["logs"].get_array();
				FC_ASSERT(receipt_logs.size() == 0, "this trasnaction not fail");
				FC_ASSERT(source_trx["gas"].as_string() == respit_trx["gasUsed"].as_string());
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result eth_cancel_fail_crosschain_trx_evaluate::do_apply(const eth_cancel_fail_crosschain_trx_operation& o) {
			try {
				database& d = db();
				auto& crosschain_trxs = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto iter = crosschain_trxs.find(transaction_id_type(o.fail_transaction_id));
				auto without_sign_iter = crosschain_trxs.find(iter->relate_transaction_id);
				auto crosschain_sign_trxs_range = d.get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>().equal_range(without_sign_iter->transaction_id);
				for (auto sign_trx : boost::make_iterator_range(crosschain_sign_trxs_range.first, crosschain_sign_trxs_range.second)) {
					auto sign_iter = crosschain_trxs.find(sign_trx.transaction_id);
					d.modify(*sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_canceled;
						obj.block_num = d.head_block_num();
					});
				}
				auto op = without_sign_iter->real_transaction.operations[0];
				auto without_sign_op = op.get<crosschain_withdraw_without_sign_operation>();
				const asset_object&   asset_type = without_sign_op.asset_id(d);
				for (const auto& source_trx_id : without_sign_op.ccw_trx_ids) {
					auto source_iter = crosschain_trxs.find(source_trx_id);
					auto op = source_iter->real_transaction.operations[0];
					auto source_op = op.get<crosschain_withdraw_operation>();
					d.adjust_balance(source_op.withdraw_account, asset(asset_type.amount_from_string(source_op.amount).amount, source_op.asset_id));
					d.modify(*source_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_canceled;
						obj.block_num = d.head_block_num();
					});
					d.modify(asset_type.dynamic_asset_data_id(d), [&asset_type, source_op](asset_dynamic_data_object& d) {
						d.current_supply += asset_type.amount_from_string(source_op.amount).amount;
					});
				}
				d.modify(*iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_canceled;
					obj.block_num = d.head_block_num();
				});
				d.modify(*without_sign_iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_canceled;
					obj.block_num = d.head_block_num();
				});
				d.modify(asset_type.dynamic_asset_data_id(d), [&asset_type, without_sign_op](asset_dynamic_data_object& d) {
					d.current_supply -= without_sign_op.crosschain_fee.amount;
				});
			}FC_CAPTURE_AND_RETHROW((o))
		}
	}
}