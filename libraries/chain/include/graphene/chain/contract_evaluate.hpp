#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/account_object.hpp>

namespace graphene {
	namespace chain {
		class contract_register_evaluate :public evaluator<contract_register_evaluate> {
		private:
			gas_count_type gas_used;
			std::vector<base_operation> result_operations;
		public:
			typedef contract_register_operation operation_type;

			void_result do_evaluate(const contract_register_operation& o);
			void_result do_apply(const contract_register_operation& o);

			virtual void pay_fee() override;
		};
	}
}