#pragma once

#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene {
	namespace chain {

		class guard_refund_balance_evaluator : public evaluator<guard_refund_balance_evaluator>
		{
		public:
			void_result do_evaluate(const guard_refund_balance_operation& o);
			void_result do_apply(const guard_refund_balance_operation& o);
		};


	}
}