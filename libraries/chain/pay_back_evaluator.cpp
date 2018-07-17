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
				share_type min_payback_balance = d.get_global_properties().parameters.min_pay_back_balance;
				auto other_payback_balance = d.get_global_properties().parameters.min_pay_back_balance_other_asset;
				
				std::map<std::string,asset> pay_back;
				for (const auto& pay_back_obj : o.pay_back_balance) {
					auto symbol =d.get( pay_back_obj.second.asset_id).symbol;
					if (pay_back.count(symbol) == 0)
						pay_back[symbol] = pay_back_obj.second;
					else
						pay_back[symbol] += pay_back_obj.second;
					auto temp_iter = temp_payback_balance.find(pay_back_obj.first);
					FC_ASSERT(pay_back_obj.second.amount > 0);
					FC_ASSERT(temp_iter != temp_payback_balance.end());
					FC_ASSERT(pay_back_obj.second <= temp_iter->second);
				}

				for (const auto& p_back : pay_back)
				{
					if (other_payback_balance.count(p_back.first) != 0) {
						if (min_payback_balance > other_payback_balance[p_back.first].amount)
							min_payback_balance = other_payback_balance[p_back.first].amount;
					}
					FC_ASSERT(p_back.second.amount > 0);
					FC_ASSERT(p_back.second.amount >= min_payback_balance, "doesnt get enough pay back");
				}
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}
		void_result pay_back_evaluator::do_apply(const pay_back_operation& o) {
			for (const auto& pay_back_obj : o.pay_back_balance){
				db().adjust_pay_back_balance(o.pay_back_owner,asset(-pay_back_obj.second.amount,pay_back_obj.second.asset_id), pay_back_obj.first);
				db().adjust_balance(o.pay_back_owner, pay_back_obj.second);
			}
			return void_result();
		}

		void_result bonus_evaluator::do_evaluate(const bonus_operation& o) {
			try {
				const database& d = db();
				auto bonus_balances = d.get_bonus_balance(o.bonus_owner);
				for (const auto& itr : o.bonus_balance)
				{
					FC_ASSERT(itr.second > 0, "${asset} should be larger than 0", ("asset", itr.first));
					FC_ASSERT(itr.second <= bonus_balances[itr.first], "${asset} is not enough to obtain", ("asset", itr.first));
				}
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}

		void_result bonus_evaluator::do_apply(const bonus_operation& o)
		{
			for (const auto& bonus : o.bonus_balance) {
				auto asset_obj = db().get_asset(bonus.first);
				db().adjust_bonus_balance(o.bonus_owner, asset(-bonus.second, asset_obj->get_id()));
				db().adjust_balance(o.bonus_owner,asset(bonus.second,asset_obj->get_id()));
			}
			return void_result();
		}
	}
}