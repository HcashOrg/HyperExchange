#include <graphene/chain/guard_refund_balance_evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene {
	namespace chain {
		void_result guard_refund_balance_evaluator::do_apply(const guard_refund_balance_operation& o)
		{
			try
			{

			}FC_CAPTURE_AND_RETHROW((o))
		}

		void_result guard_refund_balance_evaluator::do_evaluate(const guard_refund_balance_operation& o)
		{
			try
			{
				const database& d = db();
				const asset_object&   asset_type = o.refund_asset_id(d);
				// 			auto & iter = d.get_index_type<guard_member_index>().indices().get<by_account>();
				// 			auto itr = iter.find(o.lock_balance_account);
				// 			FC_ASSERT(itr != iter.end(), "Dont have lock guard account");

			}FC_CAPTURE_AND_RETHROW((o))
		}
	}
}