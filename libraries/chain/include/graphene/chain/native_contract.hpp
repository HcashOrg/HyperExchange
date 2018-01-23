#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/vesting_balance_object.hpp>

namespace graphene {
	namespace chain {
		class abstract_native_contract
		{
		public:
			virtual ~abstract_native_contract() {}

			// unique key to identify native contract
			virtual std::string contract_key() const = 0;
			virtual address contract_address() const = 0;
			virtual std::set<std::string> apis() const = 0;
			virtual std::set<std::string> offline_apis() const = 0;
			virtual std::set<std::string> events() const = 0;

			virtual contract_invoke_result invoke(const std::string& api_name, const std::string& api_arg) = 0;

		};

		// FIXME: remove the demo native contract
		class demo_native_contract : public abstract_native_contract
		{
		public:
			address contract_id;
		public:
			static std::string native_contract_key() { return "demo"; }

			demo_native_contract(const address& _contract_id) : contract_id(_contract_id) {}
			virtual ~demo_native_contract();
			virtual std::string contract_key() const;
			virtual address contract_address() const;
			virtual std::set<std::string> apis() const;
			virtual std::set<std::string> offline_apis() const;
			virtual std::set<std::string> events() const;

			virtual contract_invoke_result invoke(const std::string& api_name, const std::string& api_arg);
		};

		class native_contract_finder
		{
		public:
			static bool has_native_contract_with_key(const std::string& key);
			static shared_ptr<abstract_native_contract> create_native_contract_by_key(const std::string& key, const address& contract_address);

		};

		struct native_contract_register_operation : public base_operation
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
			string  native_contract_key;

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
	}
}

FC_REFLECT(graphene::chain::demo_native_contract, (contract_id))
FC_REFLECT(graphene::chain::native_contract_register_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::native_contract_register_operation, (fee)(init_cost)(gas_price)(owner_addr)(owner_pubkey)(register_time)(contract_id)(native_contract_key))