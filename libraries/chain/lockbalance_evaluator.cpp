#include <graphene/chain/lockbalance_evaluator.hpp>

namespace graphene{
	namespace chain{
		void_result lockbalance_operation_evaluator::do_evaluate(const lockbalance_operation& o)
		{
			const database& d = db();

			const asset_object&   asset_type = o.lockbalance.asset_id(d);
			bool insufficient_balance = d.get_balance(o.from_addr, asset_type.id).amount >= o.lockbalance.amount;
			FC_ASSERT(insufficient_balance,
				"Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from addr '${a}' to '${t}'",
				("a", o.from_account)("t", o.lock_account)("total_transfer", d.to_pretty_string(o.lockbalance))("balance", d.to_pretty_string(d.get_balance(o.from_addr, asset_type.id))));
			return void_result();
		}
		void_result lockbalance_operation_evaluator::do_apply(const lockbalance_operation& o)
		{
			try {
				database& d = db();
				d.adjust_balance(o.from_addr, -o.lockbalance);

			} FC_CAPTURE_AND_RETHROW((o))
		}
	}
}