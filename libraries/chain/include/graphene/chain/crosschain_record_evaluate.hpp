#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/crosschain_record.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/crosschain/crosschain.hpp>
namespace graphene {
	namespace chain {
		class crosschain_record_evaluate :public evaluator<crosschain_record_evaluate> {
		public:
			typedef crosschain_record_operation operation_type;

			void_result do_evaluate(const crosschain_record_operation& o);
			void_result do_apply(const crosschain_record_operation& o);

			virtual void pay_fee() override;
		};
		class crosschain_withdraw_evaluate :public evaluator<crosschain_withdraw_evaluate> {
		public:
			typedef crosschain_withdraw_operation operation_type;

			void_result do_evaluate(const crosschain_withdraw_operation& o);
			void_result do_apply(const crosschain_withdraw_operation& o);

			virtual void pay_fee() override;
		};
		class crosschain_withdraw_without_sign_evaluate :public evaluator<crosschain_withdraw_without_sign_evaluate> {
		public:
			typedef crosschain_withdraw_without_sign_operation operation_type;

			void_result do_evaluate(const crosschain_withdraw_without_sign_operation& o);
			void_result do_apply(const crosschain_withdraw_without_sign_operation& o);

			virtual void pay_fee() override;
		};
		class crosschain_withdraw_with_sign_evaluate :public evaluator<crosschain_withdraw_with_sign_evaluate> {
		public:
			typedef crosschain_withdraw_with_sign_operation operation_type;

			void_result do_evaluate(const crosschain_withdraw_with_sign_operation& o);
			void_result do_apply(const crosschain_withdraw_with_sign_operation& o);

			virtual void pay_fee() override;
		};
		class crosschain_withdraw_combine_sign_evaluate :public evaluator<crosschain_withdraw_combine_sign_evaluate> {
		public:
			typedef crosschain_withdraw_combine_sign_operation operation_type;

			void_result do_evaluate(const crosschain_withdraw_combine_sign_operation& o);
			void_result do_apply(const crosschain_withdraw_combine_sign_operation& o);

			virtual void pay_fee() override;
		};
		class crosschain_withdraw_result_evaluate :public evaluator<crosschain_withdraw_result_evaluate> {
		public:
			typedef crosschain_withdraw_result_operation operation_type;

			void_result do_evaluate(const crosschain_withdraw_result_operation& o);
			void_result do_apply(const crosschain_withdraw_result_operation& o);

			virtual void pay_fee() override;
		};
	}
}