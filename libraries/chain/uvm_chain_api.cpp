#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/contract_evaluate.hpp>
#include <graphene/chain/forks.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/address.hpp>
#include <uvm/exceptions.h>
#include <uvm/json_reader.h>
#include <fc/crypto/sha1.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/json.hpp>
#include <Keccak.hpp>
#include <sstream>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

namespace graphene {
	namespace chain {

		static int has_error = 0;

		static std::string get_file_name_str_from_contract_module_name(std::string name)
		{
			std::stringstream ss;
			ss << "uvm_contract_" << name;
			return ss.str();
		}

		/**
		* whether exception happen in L
		*/
		bool UvmChainApi::has_exception(lua_State *L)
		{
			return has_error ? true : false;
		}

		/**
		* clear exception marked
		*/
		void UvmChainApi::clear_exceptions(lua_State *L)
		{
			has_error = 0;
		}

		/**
		* when exception happened, use this api to tell uvm
		* @param L the lua stack
		* @param code error code, 0 is OK, other is different error
		* @param error_format error info string, will be released by lua
		* @param ... error arguments
		*/
		void UvmChainApi::throw_exception(lua_State *L, int code, const char *error_format, ...)
		{
			if (has_error)
				return;
			has_error = 1;
			char *msg = (char*)lua_malloc(L, LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH);
			if (msg) {
				memset(msg, 0x0, LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH);

				va_list vap;
				va_start(vap, error_format);
				vsnprintf(msg, LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH, error_format, vap);
				va_end(vap);
				if (strlen(msg) > LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH - 1)
				{
					msg[LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH - 1] = 0;
				}
			}
			else {
				msg = "vm out of memory";
			}
			lua_set_compile_error(L, msg);

			//如果上次的exception code为uvm_API_LVM_LIMIT_OVER_ERROR, 不能被其他异常覆盖
			//只有调用clear清理后，才能继续记录异常
			int last_code = uvm::lua::lib::get_lua_state_value(L, "exception_code").int_value;
			if (last_code != code && last_code != 0)
			{
				return;
			}

			UvmStateValue val_code;
			val_code.int_value = code;

			UvmStateValue val_msg;
			val_msg.string_value = msg;

			uvm::lua::lib::set_lua_state_value(L, "exception_code", val_code, UvmStateValueType::LUA_STATE_VALUE_INT);
			uvm::lua::lib::set_lua_state_value(L, "exception_msg", val_msg, UvmStateValueType::LUA_STATE_VALUE_STRING);
		}

		/**
		* check whether the contract apis limit over, in this lua_State
		* @param L the lua stack
		* @return TRUE(1 or not 0) if over limit(will break the vm), FALSE(0) if not over limit
		*/
		int UvmChainApi::check_contract_api_instructions_over_limit(lua_State *L)
		{
			try {
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				if (evaluator) {
					auto gas_limit = evaluator->get_gas_limit();
					if (gas_limit == 0)
						return 0;
					auto gas_count = uvm::lua::lib::get_lua_state_instructions_executed_count(L);
					return gas_count > gas_limit;
				}
				return 0;
			}FC_CAPTURE_AND_LOG((0))
		}

		static bool check_contract_exsited_by_id(contract_common_evaluate* evaluator, const string& contract_id)
		{
			try {
				try {
					if (evaluator) {
						return evaluator->has_contract(address(contract_id));
					}
					return false;
				}FC_CAPTURE_AND_LOG((nullptr))
			}
			catch (...) {
				return false;
			}
		}
		static bool check_contract_exsited_by_name(contract_common_evaluate* evaluator, const string& contract_name)
		{
			try {
				try {
					if (evaluator) {
						return evaluator->has_contract_of_name(contract_name);
					}
					return false;
				}FC_CAPTURE_AND_LOG((nullptr))
			}
			catch (...) {
				return false;
			}
		}
		static std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(contract_common_evaluate* evaluator, const string& contract_id) {
            try {
            try {
                if (evaluator) {
                    return evaluator->get_contract_code_by_id(contract_id);
                }
                return nullptr;
            }FC_CAPTURE_AND_LOG((nullptr))
	    } catch(...) {
		return nullptr;
	    }
	}

        static std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_name(contract_common_evaluate* evaluator, const string& contract_name) {
            try {
	    try {
                if (evaluator) {
                    return evaluator->get_contract_code_by_name(contract_name);
                }
                return nullptr;
            }FC_CAPTURE_AND_LOG((nullptr))
	    } catch(...) {
		return nullptr;
	    }
        }

        static void put_contract_storage_changes_to_evaluator(contract_common_evaluate* evaluator, const string& contract_id, const contract_storage_changes_type& changes) {
            try {
                if (evaluator) {
					evaluator->set_contract_storage_changes(contract_id, changes);
                }
            }FC_CAPTURE_AND_LOG((nullptr))
        }
        static std::shared_ptr<UvmContractInfo> get_contract_info_by_id(contract_common_evaluate* evaluator, const string& contract_id) {
            try {
                if (evaluator) {
                    return evaluator->get_contract_by_id(contract_id);
                }
                return nullptr;
            }FC_CAPTURE_AND_LOG((nullptr))
        }

        static contract_object get_contract_object_by_name(contract_common_evaluate* evaluator, const string& contract_name) {
            try {
                if (evaluator) {
                    return evaluator->get_contract_by_name(contract_name);
                }
                FC_ASSERT(false);
                return contract_object();
            }FC_CAPTURE_AND_LOG((contract_object()))
       
        }





		int UvmChainApi::get_stored_contract_info(lua_State *L, const char *name, std::shared_ptr<UvmContractInfo> contract_info_ret)
		{
			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
			auto code = get_contract_code_by_name(evaluator, std::string(name));
			auto contract_info = get_contract_info_by_id(evaluator, std::string(name));
			if (!code)
				return 0;

			std::string addr_str = string(name);

			return get_stored_contract_info_by_address(L, addr_str.c_str(), contract_info_ret);
		}

		int UvmChainApi::get_stored_contract_info_by_address(lua_State *L, const char *address, std::shared_ptr<UvmContractInfo> contract_info_ret)
		{
			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
			auto code = get_contract_code_by_id(evaluator, std::string(address));
			auto contract_info = get_contract_info_by_id(evaluator, std::string(address));
			if (!code || !contract_info)
				return 0;

			contract_info_ret->contract_apis.clear();

			std::copy(contract_info->contract_apis.begin(), contract_info->contract_apis.end(), std::back_inserter(contract_info_ret->contract_apis));
			std::copy(code->offline_abi.begin(), code->offline_abi.end(), std::back_inserter(contract_info_ret->contract_apis));
			return 1;
		}

		void UvmChainApi::get_contract_address_by_name(lua_State *L, const char *name, char *address, size_t *address_size)
		{
			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
			std::string contract_name = uvm::lua::lib::unwrap_any_contract_name(name);

			auto is_address = is_valid_contract_address(L, contract_name.c_str());
			auto code = is_address ? get_contract_code_by_id(evaluator, contract_name) : get_contract_code_by_name(evaluator, contract_name);
			auto contract_addr = is_address ? (get_contract_info_by_id(evaluator, contract_name)!=nullptr? contract_name : "") : get_contract_object_by_name(evaluator, contract_name).contract_address.operator fc::string();
			if (code && !contract_addr.empty())
			{
				string address_str = contract_addr;
				*address_size = address_str.length();
				strncpy(address, address_str.c_str(), CONTRACT_ID_MAX_LENGTH - 1);
				address[CONTRACT_ID_MAX_LENGTH - 1] = '\0';
				if (*address_size >= CONTRACT_ID_MAX_LENGTH) {
					*address_size = CONTRACT_ID_MAX_LENGTH - 1;
				}
			}
		}

		bool UvmChainApi::check_contract_exist_by_address(lua_State *L, const char *address)
		{
			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
			return check_contract_exsited_by_id(evaluator, std::string(address));
		}

		bool UvmChainApi::check_contract_exist(lua_State *L, const char *name)
		{
			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
			return  check_contract_exsited_by_name(evaluator, std::string(name));
		}

		std::shared_ptr<UvmModuleByteStream> UvmChainApi::get_bytestream_from_code(lua_State *L, const uvm::blockchain::Code& code)
		{
			if (code.code.size() > LUA_MODULE_BYTE_STREAM_BUF_SIZE)
				return nullptr;
			auto p_luamodule = std::make_shared<UvmModuleByteStream>();
			p_luamodule->is_bytes = true;
			p_luamodule->buff.resize(code.code.size());
			memcpy(p_luamodule->buff.data(), code.code.data(), code.code.size());
			p_luamodule->contract_name = "";

			p_luamodule->contract_apis.clear();
			std::copy(code.abi.begin(), code.abi.end(), std::back_inserter(p_luamodule->contract_apis));

			p_luamodule->contract_emit_events.clear();
			std::copy(code.offline_abi.begin(), code.offline_abi.end(), std::back_inserter(p_luamodule->offline_apis));

			p_luamodule->contract_emit_events.clear();
			std::copy(code.events.begin(), code.events.end(), std::back_inserter(p_luamodule->contract_emit_events));

			p_luamodule->contract_storage_properties.clear();
			for (const auto &p : code.storage_properties)
			{
				p_luamodule->contract_storage_properties[p.first] = p.second;
			}
			return p_luamodule;
		}
		/**
		* load contract uvm byte stream from uvm api
		*/
		std::shared_ptr<UvmModuleByteStream> UvmChainApi::open_contract(lua_State *L, const char *name)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);

			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
			std::string contract_name = uvm::lua::lib::unwrap_any_contract_name(name);

			auto code = get_contract_code_by_name(evaluator, contract_name);
			if (code && (code->code.size() <= LUA_MODULE_BYTE_STREAM_BUF_SIZE))
			{
				return get_bytestream_from_code(L, *code);
			}

			return nullptr;
		}

		std::shared_ptr<UvmModuleByteStream> UvmChainApi::open_contract_by_address(lua_State *L, const char *address)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
			auto code = get_contract_code_by_id(evaluator, std::string(address));
			if (code && (code->code.size() <= LUA_MODULE_BYTE_STREAM_BUF_SIZE))
			{
				return get_bytestream_from_code(L, *code);
			}

			return nullptr;
		}

		UvmStorageValue UvmChainApi::get_storage_value_from_uvm(lua_State *L, const char *contract_name,
			const std::string& name, const std::string& fast_map_key, bool is_fast_map)
		{
			UvmStorageValue null_storage;
			null_storage.type = uvm::blockchain::StorageValueTypes::storage_value_null;

			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
			std::string contract_id = uvm::lua::lib::unwrap_any_contract_name(contract_name);
			if (!check_contract_exist_by_address(L, contract_id.c_str()))
			{
				return null_storage;
			}
			try
			{
				return get_storage_value_from_uvm_by_address(L, contract_id.c_str(), name, fast_map_key, is_fast_map);
			}
			catch (fc::exception &e) {
				return null_storage;
			}
		}

		UvmStorageValue UvmChainApi::get_storage_value_from_uvm_by_address(lua_State *L, const char *contract_address,
			const std::string& name, const std::string& fast_map_key, bool is_fast_map)
		{
			UvmStorageValue null_storage;
			null_storage.type = uvm::blockchain::StorageValueTypes::storage_value_null;

			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
			std::string contract_id(contract_address);
			if (!check_contract_exist_by_address(L, contract_address))
			{
				return null_storage;
			}
			try 
			{
				std::string key = name;
				if (is_fast_map) {
					key = name + "." + fast_map_key;
				}
				ilog("storage ${addr}.${key}:\n", ("addr", contract_address)("key", key));
				auto storage_data = evaluator->get_storage(contract_id, key);
				// TODO: cost more gas when read large storage
				return StorageDataType::create_lua_storage_from_storage_data(L, storage_data);
			}
			catch (fc::exception &e) {
				return null_storage;
			}
		}

		static std::vector<char> str_to_chars(const std::string& str)
		{
			std::vector<char> data(str.size() + 1);
			memcpy(data.data(), str.c_str(), str.size());
			data[str.size()] = '\0';
			return data;
		}

		static std::vector<char> json_to_chars(const jsondiff::JsonValue& json_value)
		{
			const auto &json_str = jsondiff::json_dumps(json_value);
			std::vector<char> data(json_str.size() + 1);
			memcpy(data.data(), json_str.c_str(), json_str.size());
			data[json_str.size()] = '\0';
			return data;
		}

		static bool compare_key(const std::string& first, const std::string& second)
		{
			unsigned int i = 0;
			while ((i<first.length()) && (i<second.length()))
			{
				if (first[i] < second[i])
					return true;
				else if (first[i] > second[i])
					return false;
				else
					++i;
			}
			return (first.length() < second.length());
		}

		// parse arg to json_array when it's json object. otherwhile return itself. And recursively process child elements
		static jsondiff::JsonValue nested_json_object_to_array(const jsondiff::JsonValue& json_value)
		{
			if (json_value.is_object())
			{
				const auto& obj = json_value.as<jsondiff::JsonObject>();
				jsondiff::JsonArray json_array;
				std::list<std::string> keys;
				for (auto it = obj.begin(); it != obj.end(); it++)
				{
					keys.push_back(it->key());
				}
				keys.sort(&compare_key);
				for (const auto& key : keys)
				{
					jsondiff::JsonArray item_json;
					item_json.push_back(key);
					item_json.push_back(nested_json_object_to_array(obj[key]));
					json_array.push_back(item_json);
				}
				return json_array;
			}
			if (json_value.is_array())
			{
				const auto& arr = json_value.as<jsondiff::JsonArray>();
				jsondiff::JsonArray result;
				for (const auto& item : arr)
				{
					result.push_back(nested_json_object_to_array(item));
				}
				return result;
			}
			return json_value;
		}

		static cbor::CborObjectP nested_cbor_object_to_array(const cbor::CborObject* cbor_value)
		{
			if (cbor_value->is_map())
			{
				const auto& map = cbor_value->as_map();
				cbor::CborArrayValue cbor_array;
				std::list<std::string> keys;
				for (auto it = map.begin(); it != map.end(); it++)
				{
					keys.push_back(it->first);
				}
				keys.sort(&compare_key);
				for (const auto& key : keys)
				{
					cbor::CborArrayValue item_json;
					item_json.push_back(cbor::CborObject::from_string(key));
					item_json.push_back(nested_cbor_object_to_array(map.at(key).get()));
					cbor_array.push_back(cbor::CborObject::create_array(item_json));
				}
				return cbor::CborObject::create_array(cbor_array);
			}
			if (cbor_value->is_array())
			{
				const auto& arr = cbor_value->as_array();
				cbor::CborArrayValue result;
				for (const auto& item : arr)
				{
					result.push_back(nested_cbor_object_to_array(item.get()));
				}
				return cbor::CborObject::create_array(result);
			}
			return std::make_shared<cbor::CborObject>(*cbor_value);
		}

		bool UvmChainApi::commit_storage_changes_to_uvm(lua_State *L, AllContractsChangesMap &changes)
		{
			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
			auto use_cbor_diff_flag = use_cbor_diff(L);
			cbor_diff::CborDiff differ;
			jsondiff::JsonDiff json_differ;
			int64_t storage_gas = 0;

			auto gas_limit = uvm::lua::lib::get_lua_state_instructions_limit(L);
			const char* out_of_gas_error = "contract storage changes out of gas";

			// auto txid = evaluator->get_current_trx_id();

			for (auto all_con_chg_iter = changes.begin(); all_con_chg_iter != changes.end(); ++all_con_chg_iter)
			{
				// commit change to evaluator
				contract_storage_changes_type contract_storage_change;
				std::string contract_id = all_con_chg_iter->first;
				ContractChangesMap contract_change = *(all_con_chg_iter->second);
				cbor::CborMapValue nested_changes;
				jsondiff::JsonObject json_nested_changes;

				for (auto con_chg_iter = contract_change.begin(); con_chg_iter != contract_change.end(); ++con_chg_iter)
				{
					std::string contract_name = con_chg_iter->first;

					StorageDataChangeType storage_change;
					// storage_op存储的从before, after改成diff
					auto storage_after = StorageDataType::get_storage_data_from_lua_storage(con_chg_iter->second.after);
					if(use_cbor_diff_flag) {
						auto cbor_storage_before = uvm_storage_value_to_cbor(con_chg_iter->second.before);
                                        	auto cbor_storage_after = uvm_storage_value_to_cbor(con_chg_iter->second.after);
						con_chg_iter->second.cbor_diff = *(differ.diff(cbor_storage_before, cbor_storage_after));
						auto cbor_diff_value = std::make_shared<cbor::CborObject>(con_chg_iter->second.cbor_diff.value());
                                        	const auto& cbor_diff_chars = cbor_diff::cbor_encode(cbor_diff_value);
                                        	storage_change.storage_diff.storage_data = cbor_diff_chars;
						storage_change.after = storage_after;
						contract_storage_change[contract_name] = storage_change;
						nested_changes[contract_name] = cbor_diff_value;
						//if(contract_id=="HXCNwVZ4zpNTEjN1G3jNoY1RuzzLZkWGz6UR") { // using for debug log
							//printf("storage change %s.%s change %s\n", contract_id.c_str(), contract_name.c_str(), cbor_diff_value->str().c_str());
							//printf("storage cbor diff: %s\n", fc::to_hex(cbor_diff_chars).c_str());
						//}
					} else {
						const auto& json_storage_before = uvm_storage_value_to_json(con_chg_iter->second.before);
                                                const auto& json_storage_after = uvm_storage_value_to_json(con_chg_iter->second.after);
                                                con_chg_iter->second.diff = *(json_differ.diff(json_storage_before, json_storage_after));
						storage_change.storage_diff.storage_data = json_to_chars(con_chg_iter->second.diff.value());
						storage_change.after = storage_after;
						contract_storage_change[contract_name] = storage_change;
						json_nested_changes[contract_name] = con_chg_iter->second.diff.value();	
					}
					
				}
				// count gas by changes size
				size_t changes_size = 0;
				if(use_cbor_diff_flag) {
					auto nested_changes_cbor = cbor::CborObject::create_map(nested_changes);
					const auto& changes_parsed_to_array = nested_cbor_object_to_array(nested_changes_cbor.get());
					changes_size = cbor_diff::cbor_encode(changes_parsed_to_array).size();
				} else {
					const auto& changes_parsed_to_array = nested_json_object_to_array(json_nested_changes);
					changes_size = jsondiff::json_dumps(changes_parsed_to_array).size();
				}
				// printf("changes size: %d bytes\n", changes_size);
				auto gas_change = changes_size * 10; // 1 byte storage cost 10 gas
				storage_gas += gas_change;
				//dlog(std::string("txid ") + txid.str() + " storage gas: " + std::to_string(storage_gas));

				if (storage_gas < 0 && gas_limit > 0) {
					throw_exception(L, UVM_API_LVM_LIMIT_OVER_ERROR, out_of_gas_error);
					return false;
				}
				if (use_gas_log(L)) {
					const auto& txid = get_transaction_id_without_gas(L);
					printf("txid %s, contract %s storage change gas %d\n", txid.c_str(), contract_id.c_str(), storage_gas);
				}
				put_contract_storage_changes_to_evaluator(evaluator, contract_id, contract_storage_change);
			}
			if (gas_limit > 0) {
				if (storage_gas > gas_limit || storage_gas + uvm::lua::lib::get_lua_state_instructions_executed_count(L) > gas_limit) {
					throw_exception(L, UVM_API_LVM_LIMIT_OVER_ERROR, out_of_gas_error);
					return false;
				}
			}
			//if (txid.str() == "bbac8579c37e7985c3577d2195e5e79741d38c43") {
				// storage_gas = 10740;      // hardcode fix a bug
			//}
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, storage_gas);
			return true;
		}

		//not use
		bool UvmChainApi::register_storage(lua_State *L, const char *contract_name, const char *name)
		{
			return true;
		}

		intptr_t UvmChainApi::register_object_in_pool(lua_State *L, intptr_t object_addr, UvmOutsideObjectTypes type)
		{
			auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
			// Map<type, Map<object_key, object_addr>>
			std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
			if (node.type == UvmStateValueType::LUA_STATE_VALUE_nullptr)
			{
				node.type = UvmStateValueType::LUA_STATE_VALUE_POINTER;
				object_pools = new std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>>();
				node.value.pointer_value = (void*)object_pools;
				uvm::lua::lib::set_lua_state_value(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY, node.value, node.type);
			}
			else
			{
				object_pools = (std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
			}
			if (object_pools->find(type) == object_pools->end())
			{
				object_pools->emplace(std::make_pair(type, std::make_shared<std::map<intptr_t, intptr_t>>()));
			}
			auto pool = (*object_pools)[type];
			auto object_key = object_addr;
			(*pool)[object_key] = object_addr;
			return object_key;
		}

		intptr_t UvmChainApi::is_object_in_pool(lua_State *L, intptr_t object_key, UvmOutsideObjectTypes type)
		{
			auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
			// Map<type, Map<object_key, object_addr>>
			std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
			if (node.type == UvmStateValueType::LUA_STATE_VALUE_nullptr)
			{
				return 0;
			}
			else
			{
				object_pools = (std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
			}
			if (object_pools->find(type) == object_pools->end())
			{
				object_pools->emplace(std::make_pair(type, std::make_shared<std::map<intptr_t, intptr_t>>()));
			}
			auto pool = (*object_pools)[type];
			return (*pool)[object_key];
		}

		void UvmChainApi::release_objects_in_pool(lua_State *L)
		{
			auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
			// Map<type, Map<object_key, object_addr>>
			std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
			if (node.type == UvmStateValueType::LUA_STATE_VALUE_nullptr)
			{
				return;
			}
			object_pools = (std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
			// TODO: 对于object_pools中不同类型的对象，分别释放
			for (const auto &p : *object_pools)
			{
				auto type = p.first;
				auto pool = p.second;
				for (const auto &object_item : *pool)
				{
					auto object_key = object_item.first;
					auto object_addr = object_item.second;
					if (object_addr == 0)
						continue;
					switch (type)
					{
					case UvmOutsideObjectTypes::OUTSIDE_STREAM_STORAGE_TYPE:
					{
						auto stream = (uvm::lua::lib::UvmByteStream*) object_addr;
						delete stream;
					} break;
					default: {
						continue;
					}
					}
				}
			}
			delete object_pools;
			UvmStateValue null_state_value;
			null_state_value.int_value = 0;
			uvm::lua::lib::set_lua_state_value(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY, null_state_value, UvmStateValueType::LUA_STATE_VALUE_nullptr);
		}

		lua_Integer UvmChainApi::transfer_from_contract_to_address(lua_State *L, const char *contract_address, const char *to_address,
			const char *asset_type, int64_t amount)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			//printf("contract transfer from %s to %s, asset[%s] amount %ld\n", contract_address, to_address, asset_type, amount_str);
			//return true;
            address f_addr;
            address t_addr;
            try {
                f_addr = address(contract_address, GRAPHENE_ADDRESS_PREFIX);
            }
            catch (...)
            {
                return -3;
            }
            try {
                t_addr = address(to_address);
            }
            catch (...)
            {
                return -4;
            }
			if (amount <= 0)
				return -6;
			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
            try {

                asset transfer_amount = evaluator->asset_from_string(asset_type, "0");
                transfer_amount.amount = amount;
                evaluator->transfer_to_address(f_addr, transfer_amount,t_addr);
            }
            catch (blockchain::contract_engine::invalid_asset_symbol& e)
            {
                return -1;//
            }
            catch (blockchain::contract_engine::contract_insufficient_balance& e)
            {
                return -2;
            }
			return 0;
		}

		lua_Integer UvmChainApi::transfer_from_contract_to_public_account(lua_State *L, const char *contract_address, const char *to_account_name,
			const char *asset_type, int64_t amount)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
            auto to_addr=evaluator->get_db().get_account_address(to_account_name);
            if (to_addr == address())
            {
                return -4;
            }
            return transfer_from_contract_to_address(L, contract_address, to_addr.address_to_string().c_str(), asset_type, amount);
			// TODO
			/*
			if (!eval_state_ptr->_current_state->is_valid_account_name(to_account_name))
				return -7;
			auto acc_entry = eval_state_ptr->_current_state->get_account_entry(to_account_name);
			if (!acc_entry.valid())
				return -7;
			return transfer_from_contract_to_address(L, contract_address, acc_entry->owner_address().AddressToString().c_str(), asset_type, amount);
			*/

			return -1;
		}

		int64_t UvmChainApi::get_contract_balance_amount(lua_State *L, const char *contract_address, const char* asset_symbol)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
            address c_addr;
            try {
                c_addr = address(contract_address);
            }
            catch (...)
            {
                return -2;
            }
			try {

				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
                try {
                    asset transfer_amount = evaluator->asset_from_string(asset_symbol, "0");
                    return evaluator->get_contract_balance(c_addr, transfer_amount.asset_id).value;
                }
                catch (blockchain::contract_engine::invalid_asset_symbol& e)
                {
                    return -1;//
                }
                catch (blockchain::contract_engine::contract_not_exsited& e)
                {
                    return -3;//
                }
				return 0;
			}
			catch (const fc::exception& e)
			{
				switch (e.code())
				{
				case 30028://invalid_address
					return -2;
					//case 31003://unknown_balance_entry
					//    return -3;
				case 31303:
					return -1;
				default:
					L->force_stopping = true;
					L->exit_code = LUA_API_INTERNAL_ERROR;
					return -4;
					break;
				}
			}
		}

		int64_t UvmChainApi::get_transaction_fee(lua_State *L)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			try {
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				auto fee = evaluator->origin_op_fee();
				return fee.value;
			}
			catch (fc::exception e)
			{
				L->force_stopping = true;
				L->exit_code = LUA_API_INTERNAL_ERROR;
				return -2;
			}
		}

		uint32_t UvmChainApi::get_chain_now(lua_State *L)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			try {
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				return evaluator->get_db().head_block_time().sec_since_epoch();
			}
			catch (fc::exception e)
			{
				L->force_stopping = true;
				L->exit_code = LUA_API_INTERNAL_ERROR;
				return 0;
			}
		}


		uint32_t UvmChainApi::get_chain_safe_random(lua_State *L, bool diff_in_diff_txs) {
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			try {
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				auto& d = evaluator->get_db();
				fc::sha256::encoder random_generater;
				const auto& cur_block = d.fetch_block_by_id(d.head_block_id());
				auto current_random = d.get_dynamic_global_properties().current_random_seed;
				auto current_secret = SecretHashType();
				current_secret = d.get_random_padding(diff_in_diff_txs);
				
				if (current_random.valid()) {
					fc::raw::pack(random_generater, *current_random);

				}
				else {
					fc::raw::pack(random_generater, "");
				}
				fc::raw::pack(random_generater, current_secret);
				
				auto hash = random_generater.result();
				return hash._hash[3] % ((1 << 31) - 1);
			}
			catch (fc::exception e)
			{
				L->force_stopping = true;
				L->exit_code = LUA_API_INTERNAL_ERROR;
				return 0;
			}
		}

		uint32_t UvmChainApi::get_chain_random(lua_State *L)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			try {
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				const auto& d = evaluator->get_db();
				const auto& block = d.fetch_block_by_id(d.head_block_id());
				auto hash = block->digest();
				return hash._hash[3] % ((1 << 31) - 1);
			}
			catch (fc::exception e)
			{
				L->force_stopping = true;
				L->exit_code = LUA_API_INTERNAL_ERROR;
				return 0;
			}
		}

		std::string UvmChainApi::get_transaction_id(lua_State *L)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			return get_transaction_id_without_gas(L);
		}

		std::string UvmChainApi::get_transaction_id_without_gas(lua_State *L) const {
			try {
                                auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
                                return evaluator->get_current_trx_id().str();
                        }
                        catch (fc::exception e)
                        {
                                L->force_stopping = true;
                                L->exit_code = LUA_API_INTERNAL_ERROR;
                                return "";
                        }
		}


		uint32_t UvmChainApi::get_header_block_num(lua_State *L)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			try {
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				return evaluator->get_db().head_block_num();
			}
			catch (fc::exception e)
			{
				L->force_stopping = true;
				L->exit_code = LUA_API_INTERNAL_ERROR;
				return 0;
			}
		}

		uint32_t UvmChainApi::get_header_block_num_without_gas(lua_State *L) const {
			try {
                                auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				if(!evaluator)
					return 0;
                                return evaluator->get_db().head_block_num();
                        }
                        catch (...)
                        {
                                L->force_stopping = true;
                                L->exit_code = LUA_API_INTERNAL_ERROR;
                                return 0;
                        }

		}

		uint32_t UvmChainApi::wait_for_future_random(lua_State *L, int next)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			try {
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				const auto& d = evaluator->get_db();
				auto target = d.head_block_num() + next;
				if (target < next)
					return 0;
				return target;
			}
			catch (fc::exception e)
			{
				L->force_stopping = true;
				L->exit_code = LUA_API_INTERNAL_ERROR;
				return 0;
			}
		}
		//获取指定块与之前50块的pre_secret hash出的结果，该值在指定块被产出的上一轮出块时就已经确定，而无人可知，无法操控
		//如果希望使用该值作为随机值，以随机值作为其他数据的选取依据时，需要在目标块被产出前确定要被筛选的数据
		//如投注彩票，只允许在目标块被产出前投注
		int32_t UvmChainApi::get_waited(lua_State *L, uint32_t num)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			try {
				if (num <= 1)
					return -2;
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				const auto& d = evaluator->get_db();
				if (d.head_block_num() < num)
					return -1;
				const auto& block = d.fetch_block_by_number(num);
				auto hash = block->digest();
				return hash._hash[3] % ((1 << 31) - 1);
			}
			catch (const fc::exception& e)
			{
				L->force_stopping = true;
				L->exit_code = LUA_API_INTERNAL_ERROR;
				return -1;
			}
			//string get_wait
		}

		void UvmChainApi::emit(lua_State *L, const char* contract_id, const char* event_name, const char* event_param)
		{
			uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
			try {
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
                address contract_addr(contract_id);
				evaluator->emit_event(contract_addr, string(event_name), string(event_param));
			}
			catch (const fc::exception&)
			{
				L->force_stopping = true;
				L->exit_code = LUA_API_INTERNAL_ERROR;
				return;
			}
		}

		bool UvmChainApi::is_valid_address(lua_State *L, const char *address_str)
		{
			std::string addr(address_str);
			try {
				return address::is_valid(addr);
			} catch(...) {
				return false;
			}
		}
		bool UvmChainApi::is_valid_contract_address(lua_State *L, const char *address_str)
		{
			std::string addr(address_str);
			try {
				auto temp = graphene::chain::address(addr);
				if (temp.version != addressVersion::CONTRACT)
					return false;
			}
			catch (fc::exception&)
			{
				return false;
			}
			catch(...)
			{
				return false;
			}
			return true;
		}
		const char* UvmChainApi::get_system_asset_symbol(lua_State *L)
		{
			return GRAPHENE_SYMBOL;
		}

		uint64_t UvmChainApi::get_system_asset_precision(lua_State *L)
		{
			return GRAPHENE_BLOCKCHAIN_PRECISION;
		}

		static std::vector<char> hex_to_chars(const std::string& hex_string) {
			std::vector<char> chars(hex_string.size() / 2);
			auto bytes_count = fc::from_hex(hex_string, chars.data(), chars.size());
			if (bytes_count != chars.size()) {
				throw uvm::core::UvmException("parse hex to bytes error");
			}
			return chars;
		}

		std::vector<unsigned char> UvmChainApi::hex_to_bytes(const std::string& hex_string) {
			const auto& chars = hex_to_chars(hex_string);
			std::vector<unsigned char> bytes(chars.size());
			memcpy(bytes.data(), chars.data(), chars.size());
			return bytes;
		}
		std::string UvmChainApi::bytes_to_hex(std::vector<unsigned char> bytes) {
			std::vector<char> chars(bytes.size());
			memcpy(chars.data(), bytes.data(), bytes.size());
			return fc::to_hex(chars);
		}
		std::string UvmChainApi::sha256_hex(const std::string& hex_string) {
			const auto& chars = hex_to_chars(hex_string);
			auto hash_result = fc::sha256::hash(chars.data(), chars.size());
			return hash_result.str();
		}
		std::string UvmChainApi::sha1_hex(const std::string& hex_string) {
			const auto& chars = hex_to_chars(hex_string);
			auto hash_result = fc::sha1::hash(chars.data(), chars.size());
			return hash_result.str();
		}
		std::string UvmChainApi::sha3_hex(const std::string& hex_string) {
			Keccak keccak(Keccak::Keccak256);
			const auto& chars = hex_to_chars(hex_string);
			auto hash_result = keccak(chars.data(), chars.size());
			return hash_result;
		}
		std::string UvmChainApi::ripemd160_hex(const std::string& hex_string) {
			const auto& chars = hex_to_chars(hex_string);
			auto hash_result = fc::ripemd160::hash(chars.data(), chars.size());
			return hash_result.str();
		}

		std::string UvmChainApi::get_address_role(lua_State* L, const std::string& addr) {
			try {
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				if (evaluator) {
					return evaluator->get_address_role(addr);
				}
				return "address";
			}
			catch (...) {
				return "other";
			}
		}

		int64_t UvmChainApi::get_fork_height(lua_State* L, const std::string& fork_key) {
			if(fork_key == "MOD_CHANGE_LIST") {
				return USE_MOD_CHANGE_LIST_HEIGHT;
			}
			return -1;
		}

		bool UvmChainApi::use_cbor_diff(lua_State* L) const {
			if (1 == L->cbor_diff_state) {
				return true;
			}
			else if (2 == L->cbor_diff_state) {
				return false;
			}
			bool result = true;
			try {
                                auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				if(!evaluator)
					return true;
                                const auto& d = evaluator->get_db();
                                auto head_block_num = d.head_block_num();
                                result = head_block_num > USE_CBOR_DIFF_FORK_HEIGHT;
                        }
                        catch (...)
                        {
                                result = true;
                        }
			L->cbor_diff_state = result ? 1 : 2; // cache it
                        return result;
		}

		bool UvmChainApi::use_fast_map_set_nil(lua_State *L) const {
			bool result = true;
                        try {
                                auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
                                if(!evaluator)
                                        return true;
                                const auto& d = evaluator->get_db();
                                auto head_block_num = d.head_block_num();
                                result = head_block_num > USE_FAST_MAP_TABLE_NIL_FORK_HEIGHT;
                        }
                        catch (...)
                        {
                                result = true;
                        }
                        return result;

		}

		std::string UvmChainApi::pubkey_to_address_string(const fc::ecc::public_key& pub) const {
			address addr(pub, addressVersion::NORMAL);
			return addr.address_to_string();
		}

		bool UvmChainApi::use_gas_log(lua_State* L) const {
			// const auto& txid = get_transaction_id_without_gas(L);
			// auto blknum = get_header_block_num_without_gas(L);
			return false;	
		}

                bool UvmChainApi::use_step_log(lua_State* L) const {
			// const auto& txid = get_transaction_id_without_gas(L);
			// auto blknum = get_header_block_num_without_gas(L);
			return false;
		}

		namespace fs = boost::filesystem;

		static void dump_text_to_file(const std::string& filepath, const std::string& text) {
			auto fpath = fs::path(filepath);
			auto file_dir = fpath.parent_path();
			if (!fs::exists(file_dir)) {
				fs::create_directories(file_dir);
				if (!fs::exists(file_dir)) {
					std::cout << "directory " << file_dir << " for dump not exist" << std::endl;
					return;
				}
			}
			std::ofstream of(filepath, std::ios::app);
			of << text;
			of.close();
		}

		void UvmChainApi::before_contract_invoke(lua_State* L, const std::string& contract_addr, const std::string& txid) {
			auto blknum = get_header_block_num_without_gas(L);
			if(blknum < VM_ALLOW_DISABLE_JSON_LOADS_NEGATIVE) {
				Json_Reader::vm_disable_json_loads_negative = true;
			} else {
				Json_Reader::vm_disable_json_loads_negative = false;
			}	
				if(true)
					return;
				printf("before_contract_invoke txid: %s, contract: %s\n", txid.c_str(), contract_addr.c_str());
				std::stringstream out;
				dump_contract_state(L, contract_addr, txid, out);
				std::string out_ss = out.str();
				std::cout << "contract " << contract_addr << " state before txid " << txid << ":" << std::endl;
				std::cout << out_ss << std::endl;
				fs::path full_path(fs::current_path());
				dump_text_to_file((full_path / "logs" / "tmp_contract_state.log").string(), out_ss);
			}
			
		void UvmChainApi::dump_contract_state(lua_State* L, const std::string& contract_addr, const std::string& txid, std::ostream& out) {
			try {
				printf("enter dump_contract_state func\n");
				auto evaluator = contract_common_evaluate::get_contract_evaluator(L);
				auto& db = evaluator->get_db();
				address addr(contract_addr);
				const auto& contract_storages = db.get_contract_all_storages(addr);
				const auto& contract_balances = db.get_contract_balances(addr);
				fc::mutable_variant_object result_json;
				fc::mutable_variant_object storages_json;
				fc::variants balances_json;
				for (const auto& p : contract_storages) {
					const auto& storage_data = p.second;
					const auto& storage_val = StorageDataType::create_lua_storage_from_storage_data(L, storage_data);
					storages_json[p.first] = ::uvm_storage_value_to_json(storage_val);
				}
				result_json["storages"] = storages_json;
				for (const auto& p : contract_balances) {
					fc::variants item;
					item.push_back(p.asset_id.instance);
					item.push_back(p.amount.value);
					balances_json.push_back(item);
				}
				result_json["balances"] = balances_json;
				result_json["address"] = contract_addr;
				result_json["txid"] = txid;
				const auto& result_json_str = fc::json::to_string(result_json, fc::json::legacy_generator);
				out << result_json_str;
			}
			catch(const fc::exception& e) {
				printf("dump_contract_state error: %s\n", e.to_detail_string().c_str());
				L->force_stopping = true;
                                L->exit_code = LUA_API_INTERNAL_ERROR;
				return;
			}
                        catch(const std::exception& e) {
                                printf("dump_contract_state error: %s\n", e.what());
                                L->force_stopping = true;
                                L->exit_code = LUA_API_INTERNAL_ERROR;
				return;
                        }
			catch (...)
			{
				printf("got unknown error\n");
				L->force_stopping = true;
				L->exit_code = LUA_API_INTERNAL_ERROR;
				return;
			}
		}

	}
}
