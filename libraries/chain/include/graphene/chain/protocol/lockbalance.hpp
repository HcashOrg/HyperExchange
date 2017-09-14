#pragma once

#include <graphene/chain/protocol/base.hpp>

namespace graphene {
	namespace chain {
		struct foreclose_balance_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			asset fee;
			asset_id_type foreclose_asset_id;
			share_type foreclose_asset_amount;
			address foreclose_contract_addr;
			account_id_type foreclose_account;
			witness_id_type foreclose_miner_account;

			address foreclose_addr;
			account_id_type fee_payer()const { return foreclose_account; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const{
				a.push_back(authority(1, foreclose_addr, 1));
			}
		};
		struct lockbalance_operation : public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			asset_id_type lock_asset_id;
			share_type lock_asset_amount;
			address contract_addr;

			account_id_type lock_balance_account;
			witness_id_type lockto_miner_account;

			address lock_balance_addr;
			asset fee;

			account_id_type fee_payer()const { 
				return lock_balance_account; 
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const{
				a.push_back(authority(1, lock_balance_addr, 1));
			}
		};
	}
}

FC_REFLECT(graphene::chain::lockbalance_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::foreclose_balance_operation::fee_parameters_type, (fee))

FC_REFLECT(graphene::chain::lockbalance_operation,(lock_asset_id)(lock_asset_amount)(contract_addr)(lock_balance_account)(lockto_miner_account)(lock_balance_addr)(fee))
FC_REFLECT(graphene::chain::foreclose_balance_operation,(fee)(foreclose_asset_id)(foreclose_asset_amount)(foreclose_miner_account)(foreclose_contract_addr)(foreclose_account)(foreclose_addr))