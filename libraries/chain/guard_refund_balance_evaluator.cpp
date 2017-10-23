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
				d.adjust_balance(refund_addr,asset(amount,asset_type.get_id()));
			  //modify the status of transaction
				//TODO
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
			}FC_CAPTURE_AND_RETHROW((o))
		}
	}
}