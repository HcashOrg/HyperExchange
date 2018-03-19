#include <graphene/chain/coldhot_withdraw.hpp>
#include <graphene/chain/database.hpp>

namespace graphene {
	namespace chain {
		void coldhot_transfer_operation::validate() const {

		}
		share_type coldhot_transfer_operation::calculate_fee(const fee_parameters_type& k)const {
			return share_type(0);
		}
		void coldhot_transfer_without_sign_operation::validate() const {

		}
		share_type coldhot_transfer_without_sign_operation::calculate_fee(const fee_parameters_type& k)const {
			return share_type(0);
		}

		void coldhot_transfer_with_sign_operation::validate() const {

		}
		share_type coldhot_transfer_with_sign_operation::calculate_fee(const fee_parameters_type& k)const {
			return share_type(0);
		}
		void coldhot_transfer_combine_sign_operation::validate() const {

		}
		share_type coldhot_transfer_combine_sign_operation::calculate_fee(const fee_parameters_type& k)const {
			return share_type(0);
		}
		void coldhot_transfer_result_operation::validate() const {

		}
		share_type coldhot_transfer_result_operation::calculate_fee(const fee_parameters_type& k)const {
			return share_type(0);
		}
		void coldhot_cancel_transafer_transaction_operation::validate() const {

		}
		share_type coldhot_cancel_transafer_transaction_operation::calculate_fee(const fee_parameters_type& k)const {
			return share_type(0);
		}
	}
}