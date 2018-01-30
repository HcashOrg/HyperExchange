#include <graphene/chain/uvm_chain_api.hpp>
#include <graphene/chain/protocol/address.hpp>
#include <graphene/chain/contract_evaluate.hpp>

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
			has_error = 1;
			char *msg = (char*)lua_malloc(L, LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH);
			memset(msg, 0x0, LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH);

			va_list vap;
			va_start(vap, error_format);
			vsnprintf(msg, LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH, error_format, vap);
			va_end(vap);
			if (strlen(msg) > LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH - 1)
			{
				msg[LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH - 1] = 0;
			}
			lua_set_compile_error(L, msg);

			//如果上次的exception code为uvm_API_LVM_LIMIT_OVER_ERROR, 不能被其他异常覆盖
			//只有调用clear清理后，才能继续记录异常
			int last_code = uvm::lua::lib::get_lua_state_value(L, "exception_code").int_value;
			if (last_code != code && last_code != 0)
			{
				return;
			}

			GluaStateValue val_code;
			val_code.int_value = code;

			GluaStateValue val_msg;
			val_msg.string_value = msg;

			uvm::lua::lib::set_lua_state_value(L, "exception_code", val_code, GluaStateValueType::LUA_STATE_VALUE_INT);
			uvm::lua::lib::set_lua_state_value(L, "exception_msg", val_msg, GluaStateValueType::LUA_STATE_VALUE_STRING);
		}

		/**
		* check whether the contract apis limit over, in this lua_State
		* @param L the lua stack
		* @return TRUE(1 or not 0) if over limit(will break the vm), FALSE(0) if not over limit
		*/
		int UvmChainApi::check_contract_api_instructions_over_limit(lua_State *L)
		{
			return 0; // FIXME: need fill by uvm api
		}


		static std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(contract_common_evaluate* evaluator, const string& contract_id) {
            try {
                if (evaluator) {
                    return evaluator->get_contract_code_by_id(contract_id);
                }
                return nullptr;
            }FC_CAPTURE_AND_LOG((nullptr))
		 }

        static std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_name(contract_common_evaluate* evaluator, const string& contract_name) {
            try {
                if (evaluator) {
                    return evaluator->get_contract_code_by_name(contract_name);
                }
                return nullptr;
            }FC_CAPTURE_AND_LOG((nullptr))
        }

        static void put_contract_storage_changes_to_evaluator(contract_common_evaluate* evaluator, const string& contract_id, std::unordered_map<std::string, StorageDataChangeType> changes) {
            try {
                if (evaluator) {
                    evaluator->contracts_storage_changes[contract_id] = changes;
                }
            }FC_CAPTURE_AND_LOG((nullptr))
        }
        static std::shared_ptr<GluaContractInfo> get_contract_info_by_id(contract_common_evaluate* evaluator, const string& contract_id) {
            try {
                if (evaluator) {
                    return evaluator->get_contract_by_id(contract_id);
                }
                return nullptr;
            }FC_CAPTURE_AND_LOG((nullptr))
        }

        static contract_object get_contract_info_by_name(contract_common_evaluate* evaluator, const string& contract_name) {
            try {
                if (evaluator) {
                    return evaluator->get_contract_by_name(contract_name);
                }
                FC_ASSERT(false);
                return contract_object();
            }FC_CAPTURE_AND_LOG((contract_object()))
       
        }





		int UvmChainApi::get_stored_contract_info(lua_State *L, const char *name, std::shared_ptr<GluaContractInfo> contract_info_ret)
		{
			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
			auto code = get_contract_code_by_name(evaluator, std::string(name));
			auto contract_info = get_contract_info_by_id(evaluator, std::string(name));
			if (!code)
				return 0;

			std::string addr_str = string(name);

			return get_stored_contract_info_by_address(L, addr_str.c_str(), contract_info_ret);
		}

		int UvmChainApi::get_stored_contract_info_by_address(lua_State *L, const char *address, std::shared_ptr<GluaContractInfo> contract_info_ret)
		{
			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
			auto code = get_contract_code_by_id(evaluator, std::string(address));
			auto contract_info = get_contract_info_by_id(evaluator, std::string(address));
			if (!code)
				return 0;

			contract_info_ret->contract_apis.clear();

			std::copy(contract_info->contract_apis.begin(), contract_info->contract_apis.end(), std::back_inserter(contract_info_ret->contract_apis));
			std::copy(code->offline_abi.begin(), code->offline_abi.end(), std::back_inserter(contract_info_ret->contract_apis));
			return 1;
		}

		void UvmChainApi::get_contract_address_by_name(lua_State *L, const char *name, char *address, size_t *address_size)
		{
			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
			std::string contract_name = uvm::lua::lib::unwrap_any_contract_name(name);
			auto is_address = address::is_valid(contract_name, GRAPHENE_CONTRACT_ADDRESS_PREFIX) ? true : false;
			auto code = is_address ? get_contract_code_by_id(evaluator, contract_name) : get_contract_code_by_name(evaluator, contract_name);
			auto contract_addr = is_address ? (get_contract_info_by_id(evaluator, contract_name)? contract_name : "") : string(get_contract_info_by_name(evaluator, contract_name).contract_address);
			if (code && !contract_addr.empty())
			{
				string address_str = contract_addr;
				*address_size = address_str.length();
				strncpy(address, address_str.c_str(), CONTRACT_ID_MAX_LENGTH - 1);
				address[CONTRACT_ID_MAX_LENGTH - 1] = '\0';
			}
		}

		bool UvmChainApi::check_contract_exist_by_address(lua_State *L, const char *address)
		{
			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
			auto code = get_contract_code_by_id(evaluator, std::string(address));
			return code ? true : false;
		}

		bool UvmChainApi::check_contract_exist(lua_State *L, const char *name)
		{
			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
			auto code = get_contract_code_by_name(evaluator, std::string(name));
			return code ? true : false;
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

			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
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
			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
			auto code = get_contract_code_by_id(evaluator, std::string(address));
			if (code && (code->code.size() <= LUA_MODULE_BYTE_STREAM_BUF_SIZE))
			{
				return get_bytestream_from_code(L, *code);
			}

			return NULL;
		}

		GluaStorageValue UvmChainApi::get_storage_value_from_uvm(lua_State *L, const char *contract_name, std::string name)
		{
			GluaStorageValue null_storage;
			null_storage.type = uvm::blockchain::StorageValueTypes::storage_value_null;

			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
			std::string contract_id = uvm::lua::lib::unwrap_any_contract_name(contract_name);
			auto code = get_contract_code_by_id(evaluator, contract_id);
			if (!code)
			{
				return null_storage;
			}
			try
			{
				return get_storage_value_from_uvm_by_address(L, contract_id.c_str(), name);
			}
			catch (fc::exception &e) {
				return null_storage;
			}
		}

		GluaStorageValue UvmChainApi::get_storage_value_from_uvm_by_address(lua_State *L, const char *contract_address, std::string name)
		{
			GluaStorageValue null_storage;
			null_storage.type = uvm::blockchain::StorageValueTypes::storage_value_null;

			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
			std::string contract_id(contract_address);
			auto code = get_contract_code_by_id(evaluator, contract_id);
			if (!code)
			{
				return null_storage;
			}
			try 
			{
				auto storage_data = evaluator->get_storage(contract_id, name);
				return StorageDataType::create_lua_storage_from_storage_data(L, storage_data);
			}
			catch (fc::exception &e) {
				return null_storage;
			}
		}

		static std::vector<char> json_to_chars(jsondiff::JsonValue json_value)
		{
			const auto &json_str = jsondiff::json_dumps(json_value);
			std::vector<char> data(json_str.size() + 1);
			memcpy(data.data(), json_str.c_str(), json_str.size());
			data[json_str.size()] = '\0';
			return data;
		}

		bool UvmChainApi::commit_storage_changes_to_uvm(lua_State *L, AllContractsChangesMap &changes)
		{
			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
			jsondiff::JsonDiff json_differ;

			for (auto all_con_chg_iter = changes.begin(); all_con_chg_iter != changes.end(); ++all_con_chg_iter)
			{
				// commit change to evaluator
				std::unordered_map<std::string, StorageDataChangeType> contract_storage_change;
				std::string contract_id = all_con_chg_iter->first;
				ContractChangesMap contract_change = *(all_con_chg_iter->second);

				for (auto con_chg_iter = contract_change.begin(); con_chg_iter != contract_change.end(); ++con_chg_iter)
				{
					std::string contract_name = con_chg_iter->first;

					StorageDataChangeType storage_change;
					// storage_op存储的从before, after改成diff
					auto json_storage_before = uvm_storage_value_to_json(con_chg_iter->second.before);
					auto json_storage_after = uvm_storage_value_to_json(con_chg_iter->second.after);
					auto storage_after = StorageDataType::get_storage_data_from_lua_storage(con_chg_iter->second.after);
					con_chg_iter->second.diff = *(json_differ.diff(json_storage_before, json_storage_after));
					storage_change.storage_diff.storage_data = json_to_chars(con_chg_iter->second.diff.value());
					storage_change.after = storage_after;
					contract_storage_change[contract_name] = storage_change;
				}
				put_contract_storage_changes_to_evaluator(evaluator, contract_id, contract_storage_change);
				
			}

			return true;
		}

		//not use
		bool UvmChainApi::register_storage(lua_State *L, const char *contract_name, const char *name)
		{
			return true;
		}

		intptr_t UvmChainApi::register_object_in_pool(lua_State *L, intptr_t object_addr, GluaOutsideObjectTypes type)
		{
			auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
			// Map<type, Map<object_key, object_addr>>
			std::map<GluaOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
			if (node.type == GluaStateValueType::LUA_STATE_VALUE_nullptr)
			{
				node.type = GluaStateValueType::LUA_STATE_VALUE_POINTER;
				object_pools = new std::map<GluaOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>>();
				node.value.pointer_value = (void*)object_pools;
				uvm::lua::lib::set_lua_state_value(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY, node.value, node.type);
			}
			else
			{
				object_pools = (std::map<GluaOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
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

		intptr_t UvmChainApi::is_object_in_pool(lua_State *L, intptr_t object_key, GluaOutsideObjectTypes type)
		{
			auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
			// Map<type, Map<object_key, object_addr>>
			std::map<GluaOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
			if (node.type == GluaStateValueType::LUA_STATE_VALUE_nullptr)
			{
				return 0;
			}
			else
			{
				object_pools = (std::map<GluaOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
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
			std::map<GluaOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
			if (node.type == GluaStateValueType::LUA_STATE_VALUE_nullptr)
			{
				return;
			}
			object_pools = (std::map<GluaOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
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
					case GluaOutsideObjectTypes::OUTSIDE_STREAM_STORAGE_TYPE:
					{
						auto stream = (uvm::lua::lib::GluaByteStream*) object_addr;
						delete stream;
					} break;
					default: {
						continue;
					}
					}
				}
			}
			delete object_pools;
			GluaStateValue null_state_value;
			null_state_value.int_value = 0;
			uvm::lua::lib::set_lua_state_value(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY, null_state_value, GluaStateValueType::LUA_STATE_VALUE_nullptr);
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
                f_addr = address(contract_address,GRAPHENE_CONTRACT_ADDRESS_PREFIX);
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
			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
            try {

                asset transfer_amount = evaluator->asset_from_sting(asset_type, "0");
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
			auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
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
                c_addr = address(contract_address, GRAPHENE_CONTRACT_ADDRESS_PREFIX);
            }
            catch (...)
            {
                return -2;
            }
			try {

				auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
                try {
                    asset transfer_amount = evaluator->asset_from_sting(asset_symbol, "0");
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
				auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
				// TODO
				/*
				ChainInterface*  db_interface = NULL;
				if (!eval_state_ptr || !(db_interface = eval_state_ptr->_current_state))
				{
					FC_CAPTURE_AND_THROW(lua_executor_internal_error, (""));
				}

				Asset  fee = eval_state_ptr->_current_state->get_transaction_fee();
				oAssetEntry ass_res = db_interface->get_asset_entry(fee.asset_id);
				if (!ass_res.valid() || ass_res->precision == 0)
					return -1;
				return fee.amount;
				*/
				return 0;
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
				auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
				// TODO;
				/*
				uvm::blockchain::ChainInterface* cur_state;
				if (!eval_state_ptr || !(cur_state = eval_state_ptr->_current_state))
				{
					FC_CAPTURE_AND_THROW(lua_executor_internal_error, (""));
				}
				fc::time_point_sec time_stamp = cur_state->get_head_block_timestamp();
				return time_stamp.sec_since_epoch();
				*/
				return 0;
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
				auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
				// TODO
				/*
				uvm::blockchain::ChainInterface* cur_state;
				if (!eval_state_ptr || !(cur_state = eval_state_ptr->_current_state))
				{
					FC_CAPTURE_AND_THROW(lua_executor_internal_error, (""));
				}

				return eval_state_ptr->p_result_trx.id().hash(cur_state->get_current_random_seed())._hash[2];
				*/
				return 0;
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
			try {
				auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
				// TODO
				/*
				if (!eval_state_ptr)
					FC_CAPTURE_AND_THROW(lua_executor_internal_error, (""));
				return eval_state_ptr->trx.id().str();
				*/
				return "";
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
				auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
				// TODO
				/*
				if (!eval_state_ptr || !eval_state_ptr->_current_state)
					FC_CAPTURE_AND_THROW(lua_executor_internal_error, (""));
				return eval_state_ptr->_current_state->get_head_block_num();
				*/
				return 0;
			}
			catch (fc::exception e)
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
				auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
				// TODO
				/*
				if (!eval_state_ptr || !eval_state_ptr->_current_state)
					FC_CAPTURE_AND_THROW(lua_executor_internal_error, (""));

				uint32_t target = eval_state_ptr->_current_state->get_head_block_num() + next;
				if (target < next)
					return 0;
				return target;
				*/
				return 0;
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
				auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
				// TODO
				/*
				uvm::blockchain::ChainInterface* cur_state;
				if (!eval_state_ptr || !(cur_state = eval_state_ptr->_current_state))
					FC_CAPTURE_AND_THROW(lua_executor_internal_error, (""));
				if (cur_state->get_head_block_num() < num)
					return -1;
				BlockIdType id = cur_state->get_block_id(num);
				BlockHeader _header = cur_state->get_block_header(id);
				SecretHashType _hash = _header.previous_secret;
				auto default_id = BlockIdType();
				for (int i = 0; i < 50; i++)
				{
					if ((id = _header.previous) == default_id)
						break;
					_header = cur_state->get_block_header(id);
					_hash = _hash.hash(_header.previous_secret);
				}
				return _hash._hash[3] % (1 << 31 - 1);
				*/
				return 0;
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
				auto evaluator = common_contract_evaluator::get_contract_evaluator(L);
				// TODO
				/*
				if (evaluator == NULL)
					FC_CAPTURE_AND_THROW(uvm_executor_internal_error, (""));

				EventOperation event_op(Address(contract_id, AddressType::contract_address), std::string(event_name), std::string(event_param));
				eval_state_ptr->p_result_trx.push_event_operation(event_op);
				*/
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
			return address::is_valid(addr);
		}
		const char* UvmChainApi::get_system_asset_symbol(lua_State *L)
		{
			return GRAPHENE_SYMBOL;
		}

	}
}
