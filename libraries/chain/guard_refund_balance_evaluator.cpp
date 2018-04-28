#include <graphene/chain/guard_refund_balance_evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene {
	namespace chain {
		void_result guard_refund_balance_evaluator::do_apply(const guard_refund_balance_operation& o)
		{
			try
			{
			  //adjust the balance of refund_addr
				database& d = db();
				const asset_object&   asset_type = o.refund_asset_id(d);
				const auto refund_addr = o.refund_addr;
				const auto amount = o.refund_amount;
				//check if the status of trx is correct or not
				auto& crosschain_trxs = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto iter = crosschain_trxs.find(transaction_id_type(o.txid));
				d.adjust_balance(refund_addr,asset(amount,asset_type.get_id()));
				d.modify(*iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_canceled;
				});
			}FC_CAPTURE_AND_RETHROW((o))
		}

		void_result guard_refund_balance_evaluator::do_evaluate(const guard_refund_balance_operation& o)
		{
			try
			{
				const database& d = db();
				const asset_object&   asset_type = o.refund_asset_id(d);
				//if status of last crosschain transaction is Failed,
				//refund, and need change the status of that status
				//TODO
				const auto& crosschain_trxs = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto iter = crosschain_trxs.find(transaction_id_type(o.txid));
				FC_ASSERT( iter != crosschain_trxs.end(),"transaction not exist.");
				FC_ASSERT(iter->trx_state == withdraw_without_sign_trx_uncreate, "refund if only crosschain transaction is not created.");
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
				const auto& trx_db = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto iter = trx_db.find(o.not_enough_sign_trx_id);
				FC_ASSERT(iter != trx_db.end(), "transaction not exist.");
				FC_ASSERT(iter->trx_state != withdraw_without_sign_trx_create, "cross chain trx state error");
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
					auto op = source_trx_iter->real_transaction.operations[0];
					auto source_op = op.get<crosschain_withdraw_operation>();
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
					});
				}
				d.modify(*iter, [&](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_canceled;
				});
			}FC_CAPTURE_AND_RETHROW((o))
		}
	}
}