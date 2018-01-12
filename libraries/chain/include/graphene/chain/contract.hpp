#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <jsondiff/jsondiff.h>
#include <jsondiff/exceptions.h>
#include <uvm/uvm_lib.h>

namespace graphene {
	namespace chain {

		struct contract_register_operation : public base_operation
		{
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
				uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large memos.
			};
			

			asset fee; // transaction fee limit
			gas_count_type init_cost; // contract init gas used
			gas_price_type gas_price; // gas price of this contract transaction
			address owner_addr;
			fc::time_point     register_time;
			address contract_id;
			uvm::blockchain::Code  contract_code;

			extensions_type   extensions;

			address fee_payer()const { return owner_addr; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const
			{
				a.push_back(authority(1, owner_addr, 1));
			}
		};

		
	}
}


FC_REFLECT(graphene::chain::contract_register_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::contract_register_operation, (fee)(init_cost)(gas_price)(owner_addr)(register_time)(contract_id)(contract_code))