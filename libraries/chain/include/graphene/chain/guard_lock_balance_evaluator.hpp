#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/guard_lock_balance.hpp>

namespace graphene {
	namespace chain {
		class guard_lock_balance_evaluator : public evaluator<guard_lock_balance_evaluator> {
		public:
			typedef guard_lock_balance_operation operation_type;

			void_result do_evaluate(const guard_lock_balance_operation& o);
			void_result do_apply(const guard_lock_balance_operation& o);
		};
		class guard_foreclose_balance_evaluator :public evaluator <guard_foreclose_balance_evaluator> {
		public :
			typedef guard_foreclose_balance_operation operation_type;
			void_result do_evaluate(const guard_foreclose_balance_operation& o);
			void_result do_apply(const guard_foreclose_balance_operation& o);
		};
		class guard_update_multi_account_evaluator :public evaluator<guard_update_multi_account_evaluator> {
		public:
			typedef guard_update_multi_account_operation operation_type;
			void_result do_evaluate(const guard_update_multi_account_operation& o);
			void_result do_apply(const guard_update_multi_account_operation& o);
		};

	}
}