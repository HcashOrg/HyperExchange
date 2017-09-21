#include <graphene/chain/protocol/lockbalance.hpp>

namespace graphene {
	namespace chain {
		void foreclose_balance_operation::validate() const{
			FC_ASSERT(fee.amount >= 0);
			FC_ASSERT(foreclose_asset_amount > 0);
			
		}
		share_type foreclose_balance_operation::calculate_fee(const fee_parameters_type& k)const{
			return share_type(0);
		}
		void lockbalance_operation::validate() const{
			FC_ASSERT(fee.amount >= 0);
			FC_ASSERT(lock_asset_amount > 0);
		}
		share_type lockbalance_operation::calculate_fee(const fee_parameters_type& k)const{
			return share_type(0);
		}
	}
}