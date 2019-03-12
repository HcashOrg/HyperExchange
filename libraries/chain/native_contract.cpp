#include <graphene/chain/native_contract.hpp>
#include <graphene/chain/contract_evaluate.hpp>
#include <graphene/utilities/ordered_json.hpp>
#include <graphene/chain/native_token_contract.hpp>

#include <boost/algorithm/string.hpp>
#include <cborcpp/cbor.h>
#include <cbor_diff/cbor_diff.h>
#include <fc/crypto/hex.hpp>
#include <fc/io/json.hpp>
#include <jsondiff/jsondiff.h>

namespace graphene {
	namespace chain {
		using namespace cbor_diff;
		using namespace cbor;
		// TODO: use fast_map to store users, allowed of token contract

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

		void abstract_native_contract::throw_error(const std::string& err) const {
			FC_THROW_EXCEPTION(fc::assert_exception, err);
		}

		void abstract_native_contract::add_gas(uint64_t gas) {
			// _contract_invoke_result.gas_used += gas;
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

		bool abstract_native_contract::has_api(const string& api_name) const
		{
			const auto& api_names = apis();
			return api_names.find(api_name) != api_names.end();
		}

		std::string demo_native_contract::contract_key() const
		{
			return demo_native_contract::native_contract_key();
		}
        address demo_native_contract::contract_address() const {
			return contract_id;
		}
		std::set<std::string> demo_native_contract::apis() const {
			return { "init", "hello", "contract_balance", "withdraw", "on_deposit_asset" };
		}
		std::set<std::string> demo_native_contract::offline_apis() const {
			return {};
		}
		std::set<std::string> demo_native_contract::events() const {
			return {};
		}

		contract_invoke_result demo_native_contract::invoke(const std::string& api_name, const std::string& api_arg) {
			contract_invoke_result result;
			printf("demo native contract called\n");
			printf("api %s called with arg %s\n", api_name.c_str(), api_arg.c_str());

            result.invoker = *(_evaluate->get_caller_address());
			if (api_name == "contract_balance")
			{
				auto system_asset_id = _evaluate->asset_from_string(string(GRAPHENE_SYMBOL), string("0")).asset_id;
				auto balance = _evaluate->get_contract_balance(contract_id, system_asset_id);
				result.api_result = std::to_string(balance.value);
			}
			else if (api_name == "withdraw")
			{
				auto system_asset_id = _evaluate->asset_from_string(string(GRAPHENE_SYMBOL), string("0")).asset_id;
				auto balance = _evaluate->get_contract_balance(contract_id, system_asset_id);
				if(balance.value <= 0)
					THROW_CONTRACT_ERROR("can't withdraw because of empty balance");
				_evaluate->transfer_to_address(contract_id, balance, *(_evaluate->get_caller_address()));
			}
			return result;
		}

		// token contract
		std::string token_native_contract::contract_key() const
		{
			return token_native_contract::native_contract_key();
		}
        address token_native_contract::contract_address() const {
			return contract_id;
		}
		std::set<std::string> token_native_contract::apis() const {
			return { "init", "init_token", "transfer", "transferFrom", "balanceOf", "approve", "approvedBalanceFrom", "allApprovedFromUser", "state", "supply", "precision", "tokenName", "tokenSymbol" };
		}
		std::set<std::string> token_native_contract::offline_apis() const {
			return { "balanceOf", "approvedBalanceFrom", "allApprovedFromUser", "state", "supply", "precision", "tokenName", "tokenSymbol" };
		}
		std::set<std::string> token_native_contract::events() const {
			return { "Inited", "Transfer", "Approved" };
		}

		static const string not_inited_state_of_token_contract = "NOT_INITED";
		static const string common_state_of_token_contract = "COMMON";

		contract_invoke_result token_native_contract::init_api(const std::string& api_name, const std::string& api_arg)
		{
			set_contract_storage(contract_id, string("name"), CborObject::from_string(""));
			set_contract_storage(contract_id, string("symbol"), CborObject::from_string(""));
			set_contract_storage(contract_id, string("supply"), CborObject::from_int(0));
			set_contract_storage(contract_id, string("precision"), CborObject::from_int(0));
			set_contract_storage(contract_id, string("users"), CborObject::create_map(0));
			set_contract_storage(contract_id, string("allowed"), CborObject::create_map(0));
			set_contract_storage(contract_id, string("state"), CborObject::from_string(not_inited_state_of_token_contract));
			auto caller_addr = _evaluate->get_caller_address();
			FC_ASSERT(caller_addr);
			set_contract_storage(contract_id, string("admin"), CborObject::from_string(string(*caller_addr)));
			return _contract_invoke_result;
		}

		string token_native_contract::check_admin()
		{
			auto caller_addr = _evaluate->get_caller_address();
			if (!caller_addr)
				THROW_CONTRACT_ERROR("only admin can call this api");
			auto admin_storage = get_contract_storage(contract_id, string("admin"));
			auto admin = cbor_diff::cbor_decode(admin_storage.storage_data);
			if (admin->is_string() && admin->as_string() == string(*caller_addr))
				return admin->as_string();
			THROW_CONTRACT_ERROR("only admin can call this api");
		}

		string token_native_contract::get_storage_state()
		{
			auto state_storage = get_contract_storage(contract_id, string("state"));
			auto state = cbor_decode(state_storage.storage_data);
			return state->as_string();
		}

                string token_native_contract::get_storage_token_name()
                {       
                        auto name_storage = get_contract_storage(contract_id, string("name"));
                        auto name = cbor_decode(name_storage.storage_data);
                        return name->as_string();
                }

                string token_native_contract::get_storage_token_symbol()
                {       
                        auto symbol_storage = get_contract_storage(contract_id, string("symbol"));
                        auto symbol = cbor_decode(symbol_storage.storage_data);
                        return symbol->as_string();
                }


		int64_t token_native_contract::get_storage_supply()
		{
			auto supply_storage = get_contract_storage(contract_id, string("supply"));
			auto supply = cbor_decode(supply_storage.storage_data);
			return supply->force_as_int();
		}
		int64_t token_native_contract::get_storage_precision()
		{
			auto precision_storage = get_contract_storage(contract_id, string("precision"));
			auto precision = cbor_decode(precision_storage.storage_data);
			return precision->force_as_int();
		}

		cbor::CborMapValue token_native_contract::get_storage_users()
		{
			auto users_storage = get_contract_storage(contract_id, string("users"));
			auto users = cbor_decode(users_storage.storage_data);
			return users->as_map();
		}

		cbor::CborMapValue token_native_contract::get_storage_allowed()
		{
			auto allowed_storage = get_contract_storage(contract_id, string("allowed"));
			auto allowed = cbor_decode(allowed_storage.storage_data);
			return allowed->as_map();
		}

		int64_t token_native_contract::get_balance_of_user(const string& owner_addr)
		{
			const auto& users = get_storage_users();
			if (users.find(owner_addr) == users.end())
				return 0;
			auto user_balance_cbor = users.at(owner_addr);
			return user_balance_cbor->force_as_int();
		}

		std::string token_native_contract::get_from_address()
		{
			return string(*(_evaluate->get_caller_address())); // FIXME: when get from_address, caller maybe other contract
		}

		static bool is_numeric(std::string number)
		{
			char* end = 0;
			std::strtod(number.c_str(), &end);

			return end != 0 && *end == 0;
		}


		static bool is_integral(std::string number)
		{
			return is_numeric(number.c_str()) && std::strchr(number.c_str(), '.') == 0;
		}

		// arg format: name,symbol,supply,precision
		contract_invoke_result token_native_contract::init_token_api(const std::string& api_name, const std::string& api_arg)
		{	
			check_admin();
			if(get_storage_state()!= not_inited_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract inited before");
			std::vector<string> parsed_args;
			boost::split(parsed_args, api_arg, boost::is_any_of(","));
			if (parsed_args.size() < 4)
				THROW_CONTRACT_ERROR("argument format error, need format: name,symbol,supply,precision");
			string name = parsed_args[0];
			boost::trim(name);
			string symbol = parsed_args[1];
			boost::trim(symbol);
			if(name.empty() || symbol.empty())
				THROW_CONTRACT_ERROR("argument format error, need format: name,symbol,supply,precision");
			string supply_str = parsed_args[2];
			if (!is_integral(supply_str))
				THROW_CONTRACT_ERROR("argument format error, need format: name,symbol,supply,precision");
			int64_t supply = std::stoll(supply_str);
			if(supply <= 0)
				THROW_CONTRACT_ERROR("argument format error, supply must be positive integer");
			string precision_str = parsed_args[3];
			if(!is_integral(precision_str))
				THROW_CONTRACT_ERROR("argument format error, need format: name,symbol,supply,precision");
			int64_t precision = std::stoll(precision_str);
			if(precision <= 0)
				THROW_CONTRACT_ERROR("argument format error, precision must be positive integer");
			// allowedPrecisions = [1,10,100,1000,10000,100000,1000000,10000000,100000000]
			std::vector<int64_t> allowed_precisions = { 1,10,100,1000,10000,100000,1000000,10000000,100000000 };
			if(std::find(allowed_precisions.begin(), allowed_precisions.end(), precision) == allowed_precisions.end())
				THROW_CONTRACT_ERROR("argument format error, precision must be any one of [1,10,100,1000,10000,100000,1000000,10000000,100000000]");
			set_contract_storage(contract_id, string("state"), CborObject::from_string(common_state_of_token_contract));
			set_contract_storage(contract_id, string("precision"), CborObject::from_int(precision));
			set_contract_storage(contract_id, string("supply"), CborObject::from_int(supply));
			set_contract_storage(contract_id, string("name"), CborObject::from_string(name));
			set_contract_storage(contract_id, "symbol", CborObject::from_string(symbol));

			cbor::CborMapValue users;
			auto caller_addr = string(*(_evaluate->get_caller_address()));
			users[caller_addr] = cbor::CborObject::from_int(supply);
			set_contract_storage(contract_id, string("users"), CborObject::create_map(users));
			emit_event(contract_id, "Inited", supply_str);
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::balance_of_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow transfer");
			std::string owner_addr = api_arg;
			if(!address::is_valid(owner_addr))
				THROW_CONTRACT_ERROR("owner address is not valid address format");
			auto amount = get_balance_of_user(owner_addr);
			_contract_invoke_result.api_result = std::to_string(amount);
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::state_api(const std::string& api_name, const std::string& api_arg)
		{
			const auto& state = get_storage_state();
			_contract_invoke_result.api_result = state;
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::token_name_api(const std::string& api_name, const std::string& api_arg)
                {
                        const auto& token_name = get_storage_token_name();
                        _contract_invoke_result.api_result = token_name;
                        return _contract_invoke_result;
                }

		contract_invoke_result token_native_contract::token_symbol_api(const std::string& api_name, const std::string& api_arg)
                {
                        const auto& token_symbol = get_storage_token_symbol();
                        _contract_invoke_result.api_result = token_symbol;
                        return _contract_invoke_result;
                }

		contract_invoke_result token_native_contract::supply_api(const std::string& api_name, const std::string& api_arg)
		{
			auto supply = get_storage_supply();
			_contract_invoke_result.api_result = std::to_string(supply);
			return _contract_invoke_result;
		}
		contract_invoke_result token_native_contract::precision_api(const std::string& api_name, const std::string& api_arg)
		{
			auto precision = get_storage_precision();
			_contract_invoke_result.api_result = std::to_string(precision);
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::approved_balance_from_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow this api");
			auto allowed = get_storage_allowed();
			std::vector<string> parsed_args;
			boost::split(parsed_args, api_arg, boost::is_any_of(","));
			if (parsed_args.size() < 2)
				THROW_CONTRACT_ERROR("argument format error, need format: spenderAddress, authorizerAddress");
			string spender_address = parsed_args[0];
			boost::trim(spender_address);
			if (!address::is_valid(spender_address))
				THROW_CONTRACT_ERROR("argument format error, spender address format error");
			string authorizer_address = parsed_args[1];
			boost::trim(authorizer_address);
			if (!address::is_valid(authorizer_address))
				THROW_CONTRACT_ERROR("argument format error, authorizer address format error");
			int64_t approved_amount = 0;
			if (allowed.find(authorizer_address) != allowed.end())
			{
				auto allowed_data = allowed[authorizer_address]->as_map();
				if (allowed_data.find(spender_address) != allowed_data.end())
				{
					approved_amount = allowed_data[spender_address]->force_as_int();
				}
			}

			_contract_invoke_result.api_result = std::to_string(approved_amount);
			return _contract_invoke_result;
		}
		contract_invoke_result token_native_contract::all_approved_from_user_api(const std::string& api_name, const std::string& api_arg)
		{	
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow this api");
			auto allowed = get_storage_allowed();
			string from_address = api_arg;
			boost::trim(from_address);
			if (!address::is_valid(from_address))
				THROW_CONTRACT_ERROR("argument format error, from address format error");
			
			cbor::CborMapValue allowed_data;
			if (allowed.find(from_address) != allowed.end())
			{
				allowed_data = allowed[from_address]->as_map();
			}
			auto allowed_data_cbor = CborObject::create_map(allowed_data);
			auto allowed_data_json = allowed_data_cbor->to_json();
			auto allowed_data_str = graphene::utilities::json_ordered_dumps(allowed_data_json);
			_contract_invoke_result.api_result = allowed_data_str;
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::transfer_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow transfer");
			std::vector<string> parsed_args;
			boost::split(parsed_args, api_arg, boost::is_any_of(","));
			if (parsed_args.size() < 2)
				THROW_CONTRACT_ERROR("argument format error, need format: toAddress,amount(with precision, integer)");
			string to_address = parsed_args[0];
			boost::trim(to_address);
			if(!address::is_valid(to_address))
				THROW_CONTRACT_ERROR("argument format error, to address format error");
			string amount_str = parsed_args[1];
			boost::trim(amount_str);
			if(!is_integral(amount_str))
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");
			int64_t amount = std::stoll(amount_str);
			if(amount <= 0)
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");
			
			string from_addr = get_from_address();
			auto users = get_storage_users();
			if(users.find(from_addr)==users.end() || users[from_addr]->force_as_int()<amount)
				THROW_CONTRACT_ERROR("you have not enoungh amount to transfer out");
			auto from_addr_remain = users[from_addr]->force_as_int() - amount;
			if (from_addr_remain > 0)
				users[from_addr] = CborObject::from_int(from_addr_remain);
			else
				users.erase(from_addr);
			int64_t to_amount = 0;
			if(users.find(to_address) != users.end())
				to_amount = users[to_address]->force_as_int();
			users[to_address] = CborObject::from_int(to_amount + amount);
			set_contract_storage(contract_id, string("users"), CborObject::create_map(users));
			jsondiff::JsonObject event_arg;
			event_arg["from"] = from_addr;
			event_arg["to"] = to_address;
			event_arg["amount"] = amount;
			emit_event(contract_id, "Transfer", graphene::utilities::json_ordered_dumps(event_arg));
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::approve_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow approve");
			std::vector<string> parsed_args;
			boost::split(parsed_args, api_arg, boost::is_any_of(","));
			if (parsed_args.size() < 2)
				THROW_CONTRACT_ERROR("argument format error, need format: spenderAddress, amount(with precision, integer)");
			string spender_address = parsed_args[0];
			boost::trim(spender_address);
			if (!address::is_valid(spender_address))
				THROW_CONTRACT_ERROR("argument format error, spender address format error");
			string amount_str = parsed_args[1];
			boost::trim(amount_str);
			if (!is_integral(amount_str))
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");
			int64_t amount = std::stoll(amount_str);
			if (amount <= 0)
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");
			auto allowed = get_storage_allowed();
			cbor::CborMapValue allowed_data;
			std::string contract_caller = get_from_address();
			if (allowed.find(contract_caller) == allowed.end())
				allowed_data = cbor::CborMapValue();
			else
			{
				allowed_data = allowed[contract_caller]->as_map();
			}
			allowed_data[spender_address] = CborObject::from_int(amount);
			allowed[contract_caller] = CborObject::create_map(allowed_data);
			set_contract_storage(contract_id, string("allowed"), CborObject::create_map(allowed));
			jsondiff::JsonObject event_arg;
			event_arg["from"] = contract_caller;
			event_arg["spender"] = spender_address;
			event_arg["amount"] = amount;
			emit_event(contract_id, "Approved", graphene::utilities::json_ordered_dumps(event_arg));
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::transfer_from_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow transferFrom");
			std::vector<string> parsed_args;
			boost::split(parsed_args, api_arg, boost::is_any_of(","));
			if (parsed_args.size() < 3)
				THROW_CONTRACT_ERROR("argument format error, need format:fromAddress, toAddress, amount(with precision, integer)");
			string from_address = parsed_args[0];
			boost::trim(from_address);
			if (!address::is_valid(from_address))
			{
				THROW_CONTRACT_ERROR("argument format error, from address format error");
			}
			string to_address = parsed_args[1];
			boost::trim(to_address);
			if (!address::is_valid(to_address))
				THROW_CONTRACT_ERROR("argument format error, to address format error");
			string amount_str = parsed_args[2];
			boost::trim(amount_str);
			if (!is_integral(amount_str))
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");
			int64_t amount = std::stoll(amount_str);
			if (amount <= 0)
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");

			auto users = get_storage_users();
			auto allowed = get_storage_allowed();
			if (get_balance_of_user(from_address) < amount)
			{
				THROW_CONTRACT_ERROR("fromAddress not have enough token to withdraw");
			}
			cbor::CborMapValue allowed_data;
			if (allowed.find(from_address) == allowed.end())
				THROW_CONTRACT_ERROR("not enough approved amount to withdraw");
			else
			{
				allowed_data = allowed[from_address]->as_map();
			}
			auto contract_caller = get_from_address();
			if(allowed_data.find(contract_caller)==allowed_data.end())
				THROW_CONTRACT_ERROR("not enough approved amount to withdraw");
			auto approved_amount = allowed_data[contract_caller]->force_as_int();
			if(approved_amount < amount)
				THROW_CONTRACT_ERROR("not enough approved amount to withdraw");
			auto from_addr_remain = users[from_address]->force_as_int() - amount;
			if (from_addr_remain > 0)
				users[from_address] = cbor::CborObject::from_int(from_addr_remain);
			else
				users.erase(from_address);
			int64_t to_amount = 0;
			if(users.find(to_address) != users.end())
				to_amount = users[to_address]->force_as_int();
			users[to_address] = cbor::CborObject::from_int(to_amount + amount);
			set_contract_storage(contract_id, string("users"), CborObject::create_map(users));
							
			allowed_data[contract_caller] = cbor::CborObject::from_int(approved_amount - amount);
			if (allowed_data[contract_caller]->force_as_int() == 0)
				allowed_data.erase(contract_caller);
			allowed[from_address] = CborObject::create_map(allowed_data);
			set_contract_storage(contract_id, string("allowed"), CborObject::create_map(allowed));
									
			jsondiff::JsonObject event_arg;
			event_arg["from"] = from_address;
			event_arg["to"] = to_address;
			event_arg["amount"] = amount;
			emit_event(contract_id, "Transfer", graphene::utilities::json_ordered_dumps(event_arg));
			
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::invoke(const std::string& api_name, const std::string& api_arg) {
			std::map<std::string, std::function<contract_invoke_result(const std::string&, const std::string&)>> apis = {
				{"init", std::bind(&token_native_contract::init_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"init_token", std::bind(&token_native_contract::init_token_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"transfer", std::bind(&token_native_contract::transfer_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"transferFrom", std::bind(&token_native_contract::transfer_from_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"balanceOf", std::bind(&token_native_contract::balance_of_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"approve", std::bind(&token_native_contract::approve_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"approvedBalanceFrom", std::bind(&token_native_contract::approved_balance_from_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"allApprovedFromUser", std::bind(&token_native_contract::all_approved_from_user_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"state", std::bind(&token_native_contract::state_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"supply", std::bind(&token_native_contract::supply_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"precision", std::bind(&token_native_contract::precision_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"tokenName", std::bind(&token_native_contract::token_name_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"tokenSymbol", std::bind(&token_native_contract::token_symbol_api, this, std::placeholders::_1, std::placeholders::_2)},
			};
            if (apis.find(api_name) != apis.end())
            {
                contract_invoke_result res= apis[api_name](api_name, api_arg);
                res.invoker = *(_evaluate->get_caller_address());
                return res;
            }
			THROW_CONTRACT_ERROR("token api not found");
		}

		bool native_contract_finder::has_native_contract_with_key(const std::string& key)
		{
			std::vector<std::string> native_contract_keys = {
				// demo_native_contract::native_contract_key(),
				token_native_contract::native_contract_key()
			};
			return std::find(native_contract_keys.begin(), native_contract_keys.end(), key) != native_contract_keys.end();
		}
		shared_ptr<abstract_native_contract> native_contract_finder::create_native_contract_by_key(contract_common_evaluate* evaluate, const std::string& key, const address& contract_address)
		{
			/*if (key == demo_native_contract::native_contract_key())
			{
				return std::make_shared<demo_native_contract>(evaluate, contract_address);
			}
			else */
			if (key == token_native_contract::native_contract_key())
			{
				return std::make_shared<token_native_contract>(evaluate, contract_address);
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
