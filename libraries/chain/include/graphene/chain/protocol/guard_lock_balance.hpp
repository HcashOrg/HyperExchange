#pragma once

#include <graphene/chain/protocol/base.hpp>

namespace graphene {
	namespace chain {
		struct guard_lock_balance_operation :public base_operation{
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			asset_id_type lock_asset_id;
			share_type lock_asset_amount;
			asset fee;
			account_id_type lock_balance_account;
			//committee_member_id_type lock_balance_account;
			account_id_type fee_payer()const {
				return lock_balance_account;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				authority x;
				x.add_authority(lock_balance_account, 1);
				a.push_back(x);
			}
		};
	}
}
FC_REFLECT(graphene::chain::guard_lock_balance_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::guard_lock_balance_operation,(lock_asset_id)(lock_asset_amount)(fee)(lock_balance_account))