#pragma once

#include <graphene/chain/protocol/base.hpp>

namespace graphene {namespace chain {
		struct lockbalance_operation : public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
				uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large memos.
			};
			asset lockbalance;
			address contract_addr;
			account_id_type from_account;
			account_id_type lock_account;
			address from_addr;
			asset fee;

			/// User provided data encrypted to the memo key of the "to" account
			//optional<memo_data> memo;
			//extensions_type   extensions;

			account_id_type fee_payer()const { return from_account; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const
			{
				a.push_back(authority(1, from_addr, 1));
			}
		};
	}
}

FC_REFLECT(graphene::chain::lockbalance_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::lockbalance_operation,(lockbalance)(contract_addr)(from_account)(lock_account)(from_addr)(fee))