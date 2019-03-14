#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <cborcpp/cbor.h>
#include <native_contract/native_contract_api.h>

namespace graphene {
	namespace chain {
		class native_contract_register_evaluate;
		class contract_common_evaluate;

		const share_type native_contract_register_fee = 1000;

#define THROW_CONTRACT_ERROR(...) FC_ASSERT(false, __VA_ARGS__)

		class abstract_native_contract : public uvm::contract::native_contract_interface
		{
		protected:
			contract_common_evaluate* _evaluate;
			contract_invoke_result _contract_invoke_result;
		private:
			address contract_id;
		public:
			abstract_native_contract(contract_common_evaluate* evaluate, const address& _contract_id) : _evaluate(evaluate), contract_id(_contract_id) {}
			virtual ~abstract_native_contract() {}

			virtual std::shared_ptr<uvm::contract::native_contract_interface> get_proxy() const {
				return nullptr;
			}

			virtual std::string contract_key() const {
				return "abstract";
			}

			virtual std::set<std::string> apis() const {
				return std::set<std::string>{};
			}
			virtual std::set<std::string> offline_apis() const {
				return std::set<std::string>{};
			}
			virtual std::set<std::string> events() const {
				return std::set<std::string>{};
			}

			virtual void invoke(const std::string& api_name, const std::string& api_arg) {
				throw_error("can't call native_contract_store::invoke");
			}

			virtual address contract_address() const;


			virtual gas_count_type gas_count_for_api_invoke(const std::string& api_name) const
			{
				return 100; // now all native api call requires 100 gas count TODO: add storage gas
			}

			void set_contract_storage(const address& contract_address, const std::string& storage_name, const StorageDataType& value);
			void set_contract_storage(const address& contract_address, const std::string& storage_name, cbor::CborObjectP cbor_value);
			void fast_map_set(const address& contract_address, const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value);
			StorageDataType get_contract_storage(const address& contract_address, const string& storage_name) const;
			cbor::CborObjectP get_contract_storage_cbor(const address& contract_address, const std::string& storage_name) const;
			cbor::CborObjectP fast_map_get(const address& contract_address, const std::string& storage_name, const std::string& key) const;
			std::string get_string_contract_storage(const address& contract_address, const std::string& storage_name) const;
			int64_t get_int_contract_storage(const address& contract_address, const std::string& storage_name) const;

			void emit_event(const address& contract_address, const string& event_name, const string& event_arg);

			virtual void current_fast_map_set(const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value) {
				fast_map_set(contract_address(), storage_name, key, cbor_value);
			}
			virtual StorageDataType get_current_contract_storage(const std::string& storage_name) const {
				return get_contract_storage(contract_address(), storage_name);
			}
			virtual cbor::CborObjectP get_current_contract_storage_cbor(const std::string& storage_name) const {
				return get_contract_storage_cbor(contract_address(), storage_name);
			}
			virtual cbor::CborObjectP current_fast_map_get(const std::string& storage_name, const std::string& key) const {
				return fast_map_get(contract_address(), storage_name, key);
			}
			virtual std::string get_string_current_contract_storage(const std::string& storage_name) const {
				return get_string_contract_storage(contract_address(), storage_name);
			}
			virtual int64_t get_int_current_contract_storage(const std::string& storage_name) const {
				return get_int_contract_storage(contract_address(), storage_name);
			}
			virtual void set_current_contract_storage(const std::string& storage_name, cbor::CborObjectP cbor_value) {
				set_contract_storage(contract_address(), storage_name, cbor_value);
			}
			virtual void set_current_contract_storage(const std::string& storage_name, const StorageDataType& value) {
				set_contract_storage(contract_address(), storage_name, value);
			}
			virtual void emit_event(const std::string& event_name, const std::string& event_arg) {
				emit_event(contract_address(), event_name, event_arg);
			}

			uint64_t head_block_num() const;
			std::string caller_address_string() const;
			address caller_address() const;
			void throw_error(const std::string& err) const;

			void add_gas(uint64_t gas);
			void set_invoke_result_caller();

			virtual void* get_result() { return &_contract_invoke_result; }
			virtual void set_api_result(const std::string& api_result) {
				_contract_invoke_result.api_result = api_result;
			}
		};

		// native合约的storage的json dumps和遍历都必须是确定性的

		class native_contract_finder
		{
		public:
			static bool has_native_contract_with_key(const std::string& key);
			static shared_ptr<uvm::contract::native_contract_interface> create_native_contract_by_key(contract_common_evaluate* evaluate, const std::string& key, const address& contract_address);

		};

		struct native_contract_register_operation : public base_operation
		{
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
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
			optional<guarantee_object_id_type> guarantee_id;
			optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }

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

FC_REFLECT(graphene::chain::native_contract_register_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::native_contract_register_operation, (fee)(init_cost)(gas_price)(owner_addr)(owner_pubkey)(register_time)(contract_id)(native_contract_key)(guarantee_id))
