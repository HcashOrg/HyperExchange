#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <jsondiff/jsondiff.h>
#include <jsondiff/exceptions.h>
#include <uvm/uvm_lib.h>

#include <map>
#include <unordered_map>

namespace graphene {
	namespace chain {

		struct contract_invoke_result
		{
			std::string api_result;
			std::unordered_map<std::string, std::unordered_map<std::string, StorageDataChangeType>> storage_changes;
			std::unordered_map<address, std::unordered_map<unsigned_int, share_type>> contracts_balance_changes;
            std::unordered_map<address, asset> deposit_to_address;
			// TODO: balance changes
		};

		struct contract_register_operation : public base_operation
		{
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
				uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large fields.
			};
			

			asset fee; // transaction fee limit
			gas_count_type init_cost; // contract init limit
			gas_price_type gas_price; // gas price of this contract transaction
			address owner_addr;
			fc::ecc::public_key owner_pubkey;
			fc::time_point_sec     register_time;
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
			address calculate_contract_id() const;
		};

		struct contract_upgrade_operation : public base_operation
		{
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
				uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large fields.
			};


			asset fee; // transaction fee limit
			gas_count_type invoke_cost; // contract init limit
			gas_price_type gas_price; // gas price of this contract transaction
			address caller_addr;
			fc::ecc::public_key caller_pubkey;
			address contract_id;
			string contract_name;
			string contract_desc;

			extensions_type   extensions;

			address fee_payer()const { return caller_addr; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const
			{
				a.push_back(authority(1, caller_addr, 1));
			}
		};

		struct contract_invoke_operation : public base_operation
		{
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
				uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large fields.
			};


			asset fee; // transaction fee limit
			gas_count_type invoke_cost; // contract invoke gas limit
			gas_price_type gas_price; // gas price of this contract transaction
			address caller_addr;
			fc::ecc::public_key caller_pubkey;
			address contract_id;
			string contract_api;
			string contract_arg;

			extensions_type   extensions;

			address fee_payer()const { return caller_addr; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const
			{
				a.push_back(authority(1, caller_addr, 1));
			}
		};

		class ContractHelper
		{
		public:
			static int common_fread_int(FILE* fp, int* dst_int);
			static int common_fwrite_int(FILE* fp, const int* src_int);
			static int common_fwrite_stream(FILE* fp, const void* src_stream, int len);
			static int common_fread_octets(FILE* fp, void* dst_stream, int len);
			static std::string to_printable_hex(unsigned char chr);
			static int save_code_to_file(const string& name, UvmModuleByteStream *stream, char* err_msg);
			static uvm::blockchain::Code load_contract_from_file(const fc::path &path);
		};
		

	}
}

FC_REFLECT(graphene::chain::contract_invoke_result, (api_result)(storage_changes)(contracts_balance_changes)(deposit_to_address))
FC_REFLECT(graphene::chain::contract_register_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::contract_register_operation, (fee)(init_cost)(gas_price)(owner_addr)(owner_pubkey)(register_time)(contract_id)(contract_code))
FC_REFLECT(graphene::chain::contract_invoke_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::contract_invoke_operation, (fee)(invoke_cost)(gas_price)(caller_addr)(caller_pubkey)(contract_id)(contract_api)(contract_arg))
FC_REFLECT(graphene::chain::contract_upgrade_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::contract_upgrade_operation, (fee)(invoke_cost)(gas_price)(caller_addr)(caller_pubkey)(contract_id)(contract_name)(contract_desc))