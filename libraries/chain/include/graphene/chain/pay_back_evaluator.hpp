#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/pay_back.hpp>

namespace graphene {
	namespace chain {
		class pay_back_evaluator : public evaluator<pay_back_evaluator> {
		public:
			typedef pay_back_operation operation_type;

			void_result do_evaluate(const pay_back_operation& o);
			void_result do_apply(const pay_back_operation& o);
		};

		class bonus_evaluator : public evaluator<bonus_evaluator>{
		public:
			typedef bonus_operation operation_type;

			void_result do_evaluate(const bonus_operation& o);
			void_result do_apply(const bonus_operation& o);
		};

	}
}