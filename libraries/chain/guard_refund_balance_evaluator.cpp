#include <graphene/chain/guard_refund_balance_evaluator.hpp>
#include <graphene/chain/database.hpp>

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
	}
}