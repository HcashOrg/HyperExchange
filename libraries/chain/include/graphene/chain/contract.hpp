#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

#include <graphene/chain/contract_entry.hpp>

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
			account_id_type  owner; // contract owner
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

		struct StorageDataChangeType
		{
			string dummy; // FIXME: remove it
			// TODO: StorageDataType storage_diff;
		};

		struct storage_operation : public base_operation
		{
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
				uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large memos.
			};

			address contract_id;
			std::map<std::string, StorageDataChangeType> contract_change_storages;
			asset fee;
			address caller_addr;

			extensions_type   extensions;

			address fee_payer()const { return caller_addr; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
		};
	}
}

FC_REFLECT(graphene::chain::StorageDataChangeType, (dummy));
FC_REFLECT(graphene::chain::storage_operation::fee_parameters_type, (fee)(price_per_kbyte));
FC_REFLECT(graphene::chain::storage_operation, (fee)(caller_addr)(contract_id)(contract_change_storages));
FC_REFLECT(graphene::chain::contract_register_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::contract_register_operation, (fee)(init_cost)(gas_price)(owner)(owner_addr)(register_time)(contract_id)(contract_code))