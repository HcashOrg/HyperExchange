#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/crosschain_record.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/crosschain/crosschain.hpp>
namespace graphene {
	namespace chain {
		class crosschain_record_evaluate :public evaluator<crosschain_record_evaluate> {
			typedef crosschain_record_operation operation_type;

			void_result do_evaluate(const crosschain_record_operation& o);
			void_result do_apply(const crosschain_record_operation& o);

			virtual void pay_fee() override;
		};
		class crosschain_withdraw_evaluate :public evaluator<crosschain_withdraw_evaluate> {
			typedef crosschain_withdraw_operation operation_type;

			void_result do_evaluate(const crosschain_withdraw_operation& o);
			void_result do_apply(const crosschain_withdraw_operation& o);

			virtual void pay_fee() override;
		};
		class crosschain_withdraw_result_evaluate :public evaluator<crosschain_withdraw_result_evaluate> {
			typedef crosschain_withdraw_result_operation operation_type;

			void_result do_evaluate(const crosschain_withdraw_result_operation& o);
			void_result do_apply(const crosschain_withdraw_result_operation& o);

			virtual void pay_fee() override;
		};
	}
}