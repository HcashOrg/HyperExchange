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
			address    guard_address;
			void       validate()const;

			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, guard_address, 1));
				// 				authority x;
				// 				x.add_authority(lock_balance_account_id, 1);
				// 				a.push_back(x);
			}

		};


	}
}
FC_REFLECT(graphene::chain::guard_refund_balance_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::guard_refund_balance_operation, (fee)(refund_asset_id)(guard_address))