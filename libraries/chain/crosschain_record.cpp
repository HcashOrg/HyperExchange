#include <graphene/chain/crosschain_record.hpp>
#include <graphene/chain/database.hpp>

namespace graphene {
	namespace chain {
		void crosschain_record_operation::validate() const {
			
		}
		share_type crosschain_record_operation::calculate_fee(const fee_parameters_type& k)const {
			return share_type(0);
		}
		void crosschain_withdraw_operation::validate() const {

		}
		share_type crosschain_withdraw_operation::calculate_fee(const fee_parameters_type& k)const {
			return share_type(0);
		}

		void crosschain_withdraw_result_operation::validate() const {

		}
		share_type crosschain_withdraw_result_operation::calculate_fee(const fee_parameters_type& k)const {
			return share_type(0);
		}
	}
}