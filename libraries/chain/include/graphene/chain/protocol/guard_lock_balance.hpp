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
			guard_member_id_type lock_balance_account;
			account_id_type lock_balance_account_id;
			address lock_address;
			//guard_member_id_type lock_balance_account;
			address fee_payer()const {
				return lock_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, lock_address, 1));
// 				authority x;
// 				x.add_authority(lock_balance_account_id, 1);
// 				a.push_back(x);
			}
		};
		struct guard_foreclose_balance_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			asset_id_type foreclose_asset_id;
			share_type foreclose_asset_amount;
			asset fee;
			guard_member_id_type foreclose_balance_account;
			account_id_type foreclose_balance_account_id;
			address foreclose_address;
			//guard_member_id_type lock_balance_account;
			address fee_payer()const {
				return foreclose_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, foreclose_address, 1));
			}
		};

		struct guard_update_multi_account_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};

			string chain_type;
			asset fee;
			string cold;
			string hot;
			address addr;
			address fee_payer()const {
				return addr;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, addr, 1));
			}
		};

	}
}
FC_REFLECT(graphene::chain::guard_lock_balance_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::guard_lock_balance_operation,(lock_asset_id)(lock_asset_amount)(lock_address)(fee)(lock_balance_account)(lock_balance_account_id))

FC_REFLECT(graphene::chain::guard_foreclose_balance_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::guard_foreclose_balance_operation,(foreclose_asset_id)(foreclose_asset_amount)(fee)(foreclose_address)(foreclose_balance_account)(foreclose_balance_account_id))

FC_REFLECT(graphene::chain::guard_update_multi_account_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::guard_update_multi_account_operation, (chain_type)(fee)(cold)(hot))
