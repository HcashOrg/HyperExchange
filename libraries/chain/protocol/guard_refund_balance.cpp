#include <graphene/chain/protocol/guard_refund_balance.hpp>
#include <graphene/chain/database.hpp>
namespace graphene {
	namespace chain {
		void guard_refund_balance_operation::validate() const
		{
			FC_ASSERT(refund_amount > 0);
		}
	}
}