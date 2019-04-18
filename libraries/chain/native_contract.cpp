#include <graphene/chain/native_contract.hpp>
#include <graphene/chain/contract_evaluate.hpp>
#include <graphene/utilities/ordered_json.hpp>

#include <boost/algorithm/string.hpp>
#include <cborcpp/cbor.h>
#include <cbor_diff/cbor_diff.h>
#include <fc/crypto/hex.hpp>
#include <fc/io/json.hpp>
#include <jsondiff/jsondiff.h>
#include <native_contract/native_token_contract.h>

namespace graphene {
	namespace chain {
		using namespace cbor_diff;
		using namespace cbor;

		void abstract_native_contract::set_contract_storage(const address& contract_address, const string& storage_name, const StorageDataType& value)
		{
			if (_contract_invoke_result.storage_changes.find(string(contract_address)) == _contract_invoke_result.storage_changes.end())
			{
				_contract_invoke_result.storage_changes[string(contract_address)] = contract_storage_changes_type();
			}
			auto& storage_changes = _contract_invoke_result.storage_changes[string(contract_address)];
			if (storage_changes.find(storage_name) == storage_changes.end())
			{
				StorageDataChangeType change;
				change.after = value;
				const auto &before = _evaluate->get_storage(string(contract_address), storage_name);
				cbor_diff::CborDiff differ;
				const auto& before_cbor = cbor_decode(before.storage_data);
				const auto& after_cbor = cbor_decode(change.after.storage_data);
				auto diff = differ.diff(before_cbor, after_cbor);
				change.storage_diff.storage_data = cbor_encode(diff->value());
				change.before = before;
				storage_changes[storage_name] = change;
			}
			else
			{
				auto& change = storage_changes[storage_name];
				auto before = change.before;
				auto after = value;
				change.after = after;
				cbor_diff::CborDiff differ;
				const auto& before_cbor = cbor_diff::cbor_decode(before.storage_data);
				const auto& after_cbor = cbor_diff::cbor_decode(after.storage_data);
				auto diff = differ.diff(before_cbor, after_cbor);
				change.storage_diff.storage_data = cbor_encode(diff->value());
			}
		}

		void abstract_native_contract::set_contract_storage(const address& contract_address, const string& storage_name, cbor::CborObjectP cbor_value) {
			StorageDataType value;
			value.storage_data = cbor_encode(cbor_value);
			return set_contract_storage(contract_address, storage_name, value);
		}

		void abstract_native_contract::transfer_to_address(const address& from_contract_address, const address& to_address, const std::string& asset_symbol, const uint64_t amount) {
			auto a = _evaluate->asset_from_string(asset_symbol, std::to_string(amount));
			_evaluate->invoke_contract_result = _contract_invoke_result;
			_evaluate->transfer_to_address(from_contract_address, a, to_address);
			_contract_invoke_result = _evaluate->invoke_contract_result;
		}

		StorageDataType abstract_native_contract::get_contract_storage(const address& contract_address, const std::string& storage_name) const
		{
			if (_contract_invoke_result.storage_changes.find(contract_address.operator fc::string()) == _contract_invoke_result.storage_changes.end())
			{
				return _evaluate->get_storage(contract_address.operator fc::string(), storage_name);
			}
			auto& storage_changes = _contract_invoke_result.storage_changes.at(contract_address.address_to_string());
			if (storage_changes.find(storage_name) == storage_changes.end())
			{
				return _evaluate->get_storage(contract_address.operator fc::string(), storage_name);
			}
			return storage_changes.at(storage_name).after;
		}

		void abstract_native_contract::fast_map_set(const address& contract_address, const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value) {
			std::string full_key = storage_name + "." + key;
			set_contract_storage(contract_address, full_key, cbor_value);
		}


		cbor::CborObjectP abstract_native_contract::get_contract_storage_cbor(const address& contract_address, const std::string& storage_name) const {
			const auto& data = get_contract_storage(contract_address, storage_name);
			return cbor_decode(data.storage_data);
		}

		std::string abstract_native_contract::get_string_contract_storage(const address& contract_address, const std::string& storage_name) const {
			auto cbor_data = get_contract_storage_cbor(contract_address, storage_name);
			if (!cbor_data->is_string()) {
				throw_error(std::string("invalid string contract storage ") + contract_address.address_to_string() + "." + storage_name);
			}
			return cbor_data->as_string();
		}

		cbor::CborObjectP abstract_native_contract::fast_map_get(const address& contract_address, const std::string& storage_name, const std::string& key) const {
			std::string full_key = storage_name + "." + key;
			return get_contract_storage_cbor(contract_address, full_key);
		}

		int64_t abstract_native_contract::get_int_contract_storage(const address& contract_address, const std::string& storage_name) const {
			auto cbor_data = get_contract_storage_cbor(contract_address, storage_name);
			if (!cbor_data->is_integer()) {
				throw_error(std::string("invalid int contract storage ") + contract_address.address_to_string() + "." + storage_name);
			}
			return cbor_data->force_as_int();
		}

		uint64_t abstract_native_contract::head_block_num() const {
			return _evaluate->get_db().head_block_num();
		}

		std::string abstract_native_contract::caller_address_string() const {
			return _evaluate->get_caller_address()->address_to_string();
		}

		address abstract_native_contract::caller_address() const {
			return *(_evaluate->get_caller_address());
		}

		address abstract_native_contract::contract_address() const {
			return contract_id;
		}

		void abstract_native_contract::throw_error(const std::string& err) const {
			FC_THROW_EXCEPTION(fc::assert_exception, err);
		}

		void abstract_native_contract::add_gas(uint64_t gas) {
			// _contract_invoke_result.gas_used += gas;
		}

		void abstract_native_contract::set_invoke_result_caller() {
			_contract_invoke_result.invoker = caller_address();
		}

		void abstract_native_contract::emit_event(const address& contract_address, const string& event_name, const string& event_arg)
		{
			FC_ASSERT(!event_name.empty());
			contract_event_notify_info info;
            info.op_num = _evaluate->get_gen_eval()->get_trx_eval_state()->op_num;
            info.event_name = event_name;
            info.event_arg = event_arg;
            //todo
		    //info.caller_addr = caller_address->address_to_string();
            info.block_num = 1 + head_block_num();

			_contract_invoke_result.events.push_back(info);
		}

		bool native_contract_finder::has_native_contract_with_key(const std::string& key)
		{
			std::vector<std::string> native_contract_keys = {
				uvm::contract::token_native_contract::native_contract_key()
			};
			return std::find(native_contract_keys.begin(), native_contract_keys.end(), key) != native_contract_keys.end();
		}
		shared_ptr<uvm::contract::native_contract_interface> native_contract_finder::create_native_contract_by_key(contract_common_evaluate* evaluate, const std::string& key, const address& contract_address)
		{
			auto store = std::make_shared<abstract_native_contract>(evaluate, contract_address);
			
			if (key == uvm::contract::token_native_contract::native_contract_key())
			{
				return std::make_shared<uvm::contract::token_native_contract>(store);
			}
			else
			{
				return nullptr;
			}
		}

		void            native_contract_register_operation::validate()const
		{
			FC_ASSERT(init_cost > 0 && init_cost <= BLOCKLINK_MAX_GAS_LIMIT);
			// FC_ASSERT(fee.amount == 0 & fee.asset_id == asset_id_type(0));
			FC_ASSERT(gas_price >= BLOCKLINK_MIN_GAS_PRICE);
			FC_ASSERT(contract_id == calculate_contract_id());
			FC_ASSERT(native_contract_finder::has_native_contract_with_key(native_contract_key));
			FC_ASSERT(contract_id.version == addressVersion::CONTRACT);
		}
		share_type      native_contract_register_operation::calculate_fee(const fee_parameters_type& schedule)const
		{
			// base fee
			share_type core_fee_required = schedule.fee;
			core_fee_required += calculate_data_fee(100, schedule.price_per_kbyte); // native contract base fee
            core_fee_required += count_gas_fee(gas_price, init_cost);
			return core_fee_required;
		}
        address native_contract_register_operation::calculate_contract_id() const
		{
			address id;
			fc::sha512::encoder enc;
			std::pair<address, fc::time_point> info_to_digest(owner_addr, register_time);
			fc::raw::pack(enc, info_to_digest);

			id.addr = fc::ripemd160::hash(enc.result());
			id.version = addressVersion::CONTRACT;
			return id;

		}
	}
}
