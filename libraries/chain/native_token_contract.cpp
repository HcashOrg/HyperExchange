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
		using namespace std;



	// token contract
	std::string token_native_contract::contract_key() const
	{
		return token_native_contract::native_contract_key();
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
		const auto& cur_contract_id = contract_address();
		set_contract_storage(cur_contract_id, "name", CborObject::from_string(""));
		set_contract_storage(cur_contract_id, "symbol", CborObject::from_string(""));
		set_contract_storage(cur_contract_id, "supply", CborObject::from_int(0));
		set_contract_storage(cur_contract_id, "precision", CborObject::from_int(0));
		// fast map storages: users, allowed
		set_contract_storage(cur_contract_id, "state", CborObject::from_string(not_inited_state_of_token_contract));
		auto caller_addr = caller_address_string();
		FC_ASSERT(!caller_addr.empty(), "caller_address can't be empty");
		set_contract_storage(cur_contract_id, "admin", CborObject::from_string(caller_addr));
		return _contract_invoke_result;
	}

	string token_native_contract::check_admin()
	{
		const auto& cur_contract_id = contract_address();
		const auto& caller_addr = caller_address_string();
		const auto& admin = get_string_contract_storage(cur_contract_id, "admin");
		if (admin == caller_addr)
			return admin;
		throw_error("only admin can call this api");
		return "";
	}

	string token_native_contract::get_storage_state()
	{
		const auto& cur_contract_id = contract_address();
		const auto& state = get_string_contract_storage(cur_contract_id, "state");
		return state;
	}

	string token_native_contract::get_storage_token_name()
	{
		const auto& cur_contract_id = contract_address();
		const auto& name = get_string_contract_storage(cur_contract_id, "name");
		return name;
	}

	string token_native_contract::get_storage_token_symbol()
	{
		const auto& cur_contract_id = contract_address();
		const auto& symbol = get_string_contract_storage(cur_contract_id, "symbol");
		return symbol;
	}


	int64_t token_native_contract::get_storage_supply()
	{
		const auto& cur_contract_id = contract_address();
		auto supply = get_int_contract_storage(cur_contract_id, "supply");
		return supply;
	}
	int64_t token_native_contract::get_storage_precision()
	{
		const auto& cur_contract_id = contract_address();
		auto precision = get_int_contract_storage(cur_contract_id, "precision");
		return precision;
	}

	int64_t token_native_contract::get_balance_of_user(const std::string& owner_addr) const
	{
		const auto& cur_contract_id = contract_address();
		auto user_balance_cbor = fast_map_get(cur_contract_id, "users", owner_addr);
		if (!user_balance_cbor->is_integer())
			return 0;
		return user_balance_cbor->force_as_int();
	}

	cbor::CborMapValue token_native_contract::get_allowed_of_user(const std::string& from_addr) const {
		const auto& cur_contract_id = contract_address();
		auto user_allowed = fast_map_get(cur_contract_id, "allowed", from_addr);
		if (!user_allowed->is_map()) {
			return CborObject::create_map(0)->as_map();
		}
		return user_allowed->as_map();
	}

	std::string token_native_contract::get_from_address()
	{
		return caller_address_string(); // FIXME: when get from_address, caller maybe other contract
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
		if (get_storage_state() != not_inited_state_of_token_contract)
			throw_error("this token contract inited before");
		std::vector<string> parsed_args;
		boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
		if (parsed_args.size() < 4)
			throw_error("argument format error, need format: name,symbol,supply,precision");
		string name = parsed_args[0];
		boost::trim(name);
		string symbol = parsed_args[1];
		boost::trim(symbol);
		if (name.empty() || symbol.empty())
			throw_error("argument format error, need format: name,symbol,supply,precision");
		string supply_str = parsed_args[2];
		if (!is_integral(supply_str))
			throw_error("argument format error, need format: name,symbol,supply,precision");
		int64_t supply = std::stoll(supply_str);
		if (supply <= 0)
			throw_error("argument format error, supply must be positive integer");
		string precision_str = parsed_args[3];
		if (!is_integral(precision_str))
			throw_error("argument format error, need format: name,symbol,supply,precision");
		int64_t precision = std::stoll(precision_str);
		if (precision <= 0)
			throw_error("argument format error, precision must be positive integer");
		std::vector<int64_t> allowed_precisions = { 1,10,100,1000,10000,100000,1000000,10000000,100000000 };
		if (std::find(allowed_precisions.begin(), allowed_precisions.end(), precision) == allowed_precisions.end())
			throw_error("argument format error, precision must be any one of [1,10,100,1000,10000,100000,1000000,10000000,100000000]");
		const auto& cur_contract_id = contract_address();
		set_contract_storage(cur_contract_id, string("state"), CborObject::from_string(common_state_of_token_contract));
		set_contract_storage(cur_contract_id, string("precision"), CborObject::from_int(precision));
		set_contract_storage(cur_contract_id, string("supply"), CborObject::from_int(supply));
		set_contract_storage(cur_contract_id, string("name"), CborObject::from_string(name));
		set_contract_storage(cur_contract_id, "symbol", CborObject::from_string(symbol));

		auto caller_addr = caller_address_string();
		fast_map_set(cur_contract_id, "users", caller_addr, CborObject::from_int(supply));
		emit_event(cur_contract_id, "Inited", supply_str);
		return _contract_invoke_result;
	}

	contract_invoke_result token_native_contract::balance_of_api(const std::string& api_name, const std::string& api_arg)
	{
		if (get_storage_state() != common_state_of_token_contract)
			throw_error("this token contract state doesn't allow transfer");
		std::string owner_addr = api_arg;
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
			throw_error("this token contract state doesn't allow this api");
		std::vector<string> parsed_args;
		boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
		if (parsed_args.size() < 2)
			throw_error("argument format error, need format: spenderAddress, authorizerAddress");
		string spender_address = parsed_args[0];
		boost::trim(spender_address);
		string authorizer_address = parsed_args[1];
		boost::trim(authorizer_address);
		int64_t approved_amount = 0;
		auto allowed_data = get_allowed_of_user(authorizer_address);
		if (allowed_data.find(spender_address) != allowed_data.end())
		{
			approved_amount = allowed_data[spender_address]->force_as_int();
		}

		_contract_invoke_result.api_result = std::to_string(approved_amount);
		return _contract_invoke_result;
	}
	contract_invoke_result token_native_contract::all_approved_from_user_api(const std::string& api_name, const std::string& api_arg)
	{
		if (get_storage_state() != common_state_of_token_contract)
			throw_error("this token contract state doesn't allow this api");
		string from_address = api_arg;
		boost::trim(from_address);

		const auto& allowed_data = get_allowed_of_user(from_address);
		auto allowed_data_cbor = CborObject::create_map(allowed_data);
		auto allowed_data_json = allowed_data_cbor->to_json();
		auto allowed_data_str = uvm::util::json_ordered_dumps(allowed_data_json);
		_contract_invoke_result.api_result = allowed_data_str;
		return _contract_invoke_result;
	}

	contract_invoke_result token_native_contract::transfer_api(const std::string& api_name, const std::string& api_arg)
	{
		if (get_storage_state() != common_state_of_token_contract)
			throw_error("this token contract state doesn't allow transfer");
		std::vector<string> parsed_args;
		boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
		if (parsed_args.size() < 2)
			throw_error("argument format error, need format: toAddress,amount(with precision, integer)");
		string to_address = parsed_args[0];
		boost::trim(to_address);
		string amount_str = parsed_args[1];
		boost::trim(amount_str);
		if (!is_integral(amount_str))
			throw_error("argument format error, amount must be positive integer");
		int64_t amount = std::stoll(amount_str);
		if (amount <= 0)
			throw_error("argument format error, amount must be positive integer");

		string from_addr = get_from_address();
		const auto& cur_contract_id = contract_address();
		auto from_user_balance = get_balance_of_user(from_addr);
		if (from_user_balance < amount)
			throw_error("you have not enoungh amount to transfer out");
		auto from_addr_remain = from_user_balance - amount;
		if (from_addr_remain > 0) {
			fast_map_set(cur_contract_id, "users", from_addr, CborObject::from_int(from_addr_remain));
		}
		else {
			fast_map_set(cur_contract_id, "users", from_addr, CborObject::create_null());
		}
		auto to_amount = get_balance_of_user(to_address);
		fast_map_set(cur_contract_id, "users", to_address, CborObject::from_int(to_amount + amount));
		jsondiff::JsonObject event_arg;
		event_arg["from"] = from_addr;
		event_arg["to"] = to_address;
		event_arg["amount"] = amount;
		emit_event(cur_contract_id, "Transfer", uvm::util::json_ordered_dumps(event_arg));
		return _contract_invoke_result;
	}

	contract_invoke_result token_native_contract::approve_api(const std::string& api_name, const std::string& api_arg)
	{
		if (get_storage_state() != common_state_of_token_contract)
			throw_error("this token contract state doesn't allow approve");
		std::vector<string> parsed_args;
		boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
		if (parsed_args.size() < 2)
			throw_error("argument format error, need format: spenderAddress, amount(with precision, integer)");
		string spender_address = parsed_args[0];
		boost::trim(spender_address);
		string amount_str = parsed_args[1];
		boost::trim(amount_str);
		if (!is_integral(amount_str))
			throw_error("argument format error, amount must be positive integer");
		int64_t amount = std::stoll(amount_str);
		if (amount <= 0)
			throw_error("argument format error, amount must be positive integer");
		std::string contract_caller = get_from_address();
		auto allowed_data = get_allowed_of_user(contract_caller);
		allowed_data[spender_address] = CborObject::from_int(amount);
		if (allowed_data.size() > 1000)
			throw_error("you approved to too many users");
		const auto& cur_contract_id = contract_address();
		fast_map_set(cur_contract_id, "allowed", contract_caller, CborObject::create_map(allowed_data));
		jsondiff::JsonObject event_arg;
		event_arg["from"] = contract_caller;
		event_arg["spender"] = spender_address;
		event_arg["amount"] = amount;
		emit_event(cur_contract_id, "Approved", uvm::util::json_ordered_dumps(event_arg));
		return _contract_invoke_result;
	}

	contract_invoke_result token_native_contract::transfer_from_api(const std::string& api_name, const std::string& api_arg)
	{
		if (get_storage_state() != common_state_of_token_contract)
			throw_error("this token contract state doesn't allow transferFrom");
		std::vector<string> parsed_args;
		boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
		if (parsed_args.size() < 3)
			throw_error("argument format error, need format:fromAddress, toAddress, amount(with precision, integer)");
		string from_address = parsed_args[0];
		boost::trim(from_address);
		string to_address = parsed_args[1];
		boost::trim(to_address);
		string amount_str = parsed_args[2];
		boost::trim(amount_str);
		if (!is_integral(amount_str))
			throw_error("argument format error, amount must be positive integer");
		int64_t amount = std::stoll(amount_str);
		if (amount <= 0)
			throw_error("argument format error, amount must be positive integer");

		if (get_balance_of_user(from_address) < amount)
		{
			throw_error("fromAddress not have enough token to withdraw");
		}
		const auto& cur_contract_id = contract_address();
		auto allowed_data = get_allowed_of_user(from_address);
		auto contract_caller = get_from_address();
		if (allowed_data.find(contract_caller) == allowed_data.end())
			throw_error("not enough approved amount to withdraw");
		auto approved_amount = allowed_data[contract_caller]->force_as_int();
		if (approved_amount < amount)
			throw_error("not enough approved amount to withdraw");
		auto from_addr_remain = get_balance_of_user(from_address) - amount;
		if (from_addr_remain > 0)
			fast_map_set(cur_contract_id, "users", from_address, CborObject::from_int(from_addr_remain));
		else
			fast_map_set(cur_contract_id, "users", from_address, CborObject::create_null());
		auto to_amount = get_balance_of_user(to_address);
		fast_map_set(cur_contract_id, "users", to_address, CborObject::from_int(to_amount + amount));

		allowed_data[contract_caller] = CborObject::from_int(approved_amount - amount);
		if (allowed_data[contract_caller]->force_as_int() == 0)
			allowed_data.erase(contract_caller);
		fast_map_set(cur_contract_id, "allowed", from_address, CborObject::create_map(allowed_data));

		jsondiff::JsonObject event_arg;
		event_arg["from"] = from_address;
		event_arg["to"] = to_address;
		event_arg["amount"] = amount;
		emit_event(cur_contract_id, "Transfer", uvm::util::json_ordered_dumps(event_arg));

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
			contract_invoke_result res = apis[api_name](api_name, api_arg);
			res.invoker = caller_address();
			add_gas(gas_count_for_api_invoke(api_name));
			// res.gas_used = _contract_invoke_result.gas_used;
			return res;
		}
		throw_error("token api not found");
		return contract_invoke_result{};

		}
	}
}
