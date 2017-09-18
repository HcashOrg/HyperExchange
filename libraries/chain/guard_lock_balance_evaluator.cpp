#include <graphene/chain/guard_lock_balance_evaluator.hpp>

namespace graphene {
	namespace chain {
		void_result guard_lock_balance_evaluator::do_evaluate(const guard_lock_balance_operation& o) {
			return void_result();
		}
		void_result guard_lock_balance_evaluator::do_apply(const guard_lock_balance_operation& o) {
			return void_result();
		}
	}
}