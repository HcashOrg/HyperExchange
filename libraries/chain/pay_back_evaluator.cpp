#include <graphene/chain/pay_back_evaluator.hpp>
#include <graphene/chain/pay_back_object.hpp>
namespace graphene {
	namespace chain {
		void_result pay_back_evaluator::do_evaluate(const pay_back_operation& o) {
			try {
				const database& d = db();
				auto& payback_db = d.get_index_type<payback_index>().indices().get<by_payback_address>();
				auto pay_back_iter = payback_db.find(o.pay_back_owner);
				FC_ASSERT(pay_back_iter != payback_db.end(),"This address doesnt own pay back balace!");
				auto temp_payback_balance = pay_back_iter->owner_balance;
				for (const auto& pay_back_obj : o.pay_back_balance) {
					auto temp_iter = temp_payback_balance.find(pay_back_obj.first);
					FC_ASSERT(temp_iter != temp_payback_balance.end());
					FC_ASSERT(pay_back_obj.second.amount > 0);
					FC_ASSERT(temp_iter->second >= pay_back_obj.second);
				}
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}
		void_result pay_back_evaluator::do_apply(const pay_back_operation& o) {
			for (const auto& pay_back_obj : o.pay_back_balance){
				db().adjust_pay_back_balance(o.pay_back_owner,asset(-pay_back_obj.second.amount,pay_back_obj.second.asset_id));
				db().adjust_balance(o.pay_back_owner, pay_back_obj.second);
			}
			return void_result();
		}
	}
}