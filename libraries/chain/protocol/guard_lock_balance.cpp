#include <graphene/chain/protocol/guard_lock_balance.hpp>
#include <graphene/chain/database.hpp>
namespace graphene {
	namespace chain {
		void guard_lock_balance_operation::validate() const {
			FC_ASSERT(fee.amount >= 0);
			FC_ASSERT(lock_asset_amount > 0);
		}
		share_type guard_lock_balance_operation::calculate_fee(const fee_parameters_type& k)const {
			return share_type(0);
		}
	}
}