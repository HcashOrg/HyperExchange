#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/lockbalance.hpp>

namespace graphene {
	namespace chain {
		class lockbalance_operation_evaluator : public evaluator<lockbalance_operation_evaluator>
		{
		public:
			typedef lockbalance_operation operation_type;

			void_result do_evaluate(const lockbalance_operation& o);
			void_result do_apply(const lockbalance_operation& o);
		};

	}
}