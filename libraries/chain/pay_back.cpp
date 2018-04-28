#include <graphene/chain/pay_back.hpp>

namespace graphene {
	namespace chain {
		void pay_back_operation::validate()const {
			FC_ASSERT(fee.amount >= 0);
			for (const auto & obj : pay_back_balance){
				FC_ASSERT(obj.second.amount > 0);
			}			
		}
// 		share_type      pay_back_operation::calculate_fee(const fee_parameters_type& k)const {
// 			return share_type(0);
// 		}
	}
}