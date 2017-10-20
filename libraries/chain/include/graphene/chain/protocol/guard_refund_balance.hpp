#pragma once
#include <graphene/chain/protocol/base.hpp>


namespace graphene {
	namespace chain {

		struct guard_refund_balance_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			asset fee;
			asset_id_type refund_asset_id;
			share_type refund_amount;
			address    refund_addr;
			void       validate()const;
			address fee_payer()const {
				return refund_addr;
			}
			share_type  calculate_fee(const fee_parameters_type& k)const { return 0; }
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, refund_addr, 1));
			}

		};


	}
}
FC_REFLECT(graphene::chain::guard_refund_balance_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::guard_refund_balance_operation, (fee)(refund_asset_id)(refund_amount)(refund_addr))