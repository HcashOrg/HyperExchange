#include <graphene/chain/storage.hpp>
#include <uvm/uvm_lib.h>

#include <fc/array.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <boost/uuid/sha1.hpp>
#include <exception>

namespace graphene {
	namespace chain {
		using namespace uvm::blockchain;

		void            storage_operation::validate()const
		{
		}
		share_type      storage_operation::calculate_fee(const fee_parameters_type& schedule)const
		{
			// base fee
			share_type core_fee_required = schedule.fee;
			// bytes size fee
			for (const auto &p : contract_change_storages) {
				core_fee_required += calculate_data_fee(fc::raw::pack_size(p.first), schedule.price_per_kbyte);
				core_fee_required += calculate_data_fee(fc::raw::pack_size(p.second), schedule.price_per_kbyte); // FIXME: if p.second is pointer, change to data pointed to's size
			}
			return core_fee_required;
		}

		std::map <StorageValueTypes, std::string> storage_type_map = \
		{
			make_pair(storage_value_null, std::string("nil")),
				make_pair(storage_value_int, std::string("int")),
				make_pair(storage_value_number, std::string("number")),
				make_pair(storage_value_bool, std::string("bool")),
				make_pair(storage_value_string, std::string("string")),
				make_pair(storage_value_unknown_table, std::string("Map<unknown type>")),
				make_pair(storage_value_int_table, std::string("Map<int>")),
				make_pair(storage_value_number_table, std::string("Map<number>")),
				make_pair(storage_value_bool_table, std::string("Map<bool>")),
				make_pair(storage_value_string_table, std::string("Map<string>")),
				make_pair(storage_value_unknown_array, std::string("Array<unknown type>")),
				make_pair(storage_value_int_array, std::string("Array<int>")),
				make_pair(storage_value_number_array, std::string("Array<number>")),
				make_pair(storage_value_bool_array, std::string("Array<bool>")),
				make_pair(storage_value_string_array, std::string("Array<string>"))
		};
		const uint8_t StorageNullType::type = storage_value_null;
		const uint8_t StorageIntType::type = storage_value_int;
		const uint8_t StorageNumberType::type = storage_value_number;
		const uint8_t StorageBoolType::type = storage_value_bool;
		const uint8_t StorageStringType::type = storage_value_string;
		const uint8_t StorageIntTableType::type = storage_value_int_table;
		const uint8_t StorageNumberTableType::type = storage_value_number_table;
		const uint8_t StorageBoolTableType::type = storage_value_bool_table;
		const uint8_t StorageStringTableType::type = storage_value_string_table;
		const uint8_t StorageIntArrayType::type = storage_value_int_array;
		const uint8_t StorageNumberArrayType::type = storage_value_number_array;
		const uint8_t StorageBoolArrayType::type = storage_value_bool_array;
		const uint8_t StorageStringArrayType::type = storage_value_string_array;
		StorageDataType StorageDataType::get_storage_data_from_lua_storage(const GluaStorageValue& lua_storage)
		{
			StorageDataType storage_data;
			if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_null)
				storage_data = StorageDataType(StorageNullType());
			else if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_int)
				storage_data = StorageDataType(StorageIntType(lua_storage.value.int_value));
			else if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_number)
				storage_data = StorageDataType(StorageNumberType(lua_storage.value.number_value));
			else if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_bool)
				storage_data = StorageDataType(StorageBoolType(lua_storage.value.bool_value));
			else if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_string)
				storage_data = StorageDataType(StorageStringType(string(lua_storage.value.string_value)));
			else if (uvm::blockchain::is_any_array_storage_value_type(lua_storage.type)
				|| uvm::blockchain::is_any_table_storage_value_type(lua_storage.type))
			{
				if (get_storage_base_type(lua_storage.type) == uvm::blockchain::StorageValueTypes::storage_value_int)
				{
					std::map<std::string, LUA_INTEGER> int_map;
					std::for_each(lua_storage.value.table_value->begin(), lua_storage.value.table_value->end(),
						[&](const std::pair<std::string, struct GluaStorageValue>& item)
					{
						int_map.insert(std::make_pair(item.first, item.second.value.int_value));
					});
					if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_int_table)
						storage_data = StorageDataType(StorageIntTableType(int_map));
					else if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_int_array)
						storage_data = StorageDataType(StorageIntArrayType(int_map));
				}
				else if (get_storage_base_type(lua_storage.type) == uvm::blockchain::StorageValueTypes::storage_value_number)
				{
					std::map<std::string, double> number_map;
					std::for_each(lua_storage.value.table_value->begin(), lua_storage.value.table_value->end(),
						[&](const std::pair<std::string, struct GluaStorageValue>& item)
					{
						number_map.insert(std::make_pair(item.first, item.second.value.number_value));
					});
					if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_number_table)
						storage_data = StorageDataType(StorageNumberTableType(number_map));
					else if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_number_array)
						storage_data = StorageDataType(StorageNumberArrayType(number_map));
				}
				else if (get_storage_base_type(lua_storage.type) == uvm::blockchain::StorageValueTypes::storage_value_bool)
				{
					std::map<std::string, bool> bool_map;
					std::for_each(lua_storage.value.table_value->begin(), lua_storage.value.table_value->end(),
						[&](const std::pair<std::string, struct GluaStorageValue>& item)
					{
						bool_map.insert(std::make_pair(item.first, item.second.value.bool_value));
					});
					if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_bool_table)
						storage_data = StorageDataType(StorageBoolTableType(bool_map));
					else if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_bool_array)
						storage_data = StorageDataType(StorageBoolArrayType(bool_map));
				}
				else if (get_storage_base_type(lua_storage.type) == uvm::blockchain::StorageValueTypes::storage_value_string)
				{
					std::map<std::string, string> string_map;
					std::for_each(lua_storage.value.table_value->begin(), lua_storage.value.table_value->end(),
						[&](const std::pair<std::string, struct GluaStorageValue>& item)
					{
						string_map.insert(std::make_pair(item.first, item.second.value.string_value));
					});
					if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_string_table)
						storage_data = StorageDataType(StorageStringTableType(string_map));
					else if (lua_storage.type == uvm::blockchain::StorageValueTypes::storage_value_string_array)
						storage_data = StorageDataType(StorageStringArrayType(string_map));
				}
			}
			return storage_data;
		}

		static jsondiff::JsonValue json_from_chars(std::vector<char> data_chars)
		{
			std::vector<char> data(data_chars.size() + 1);
			memcpy(data.data(), data_chars.data(), sizeof(char) * data_chars.size());
			data[data_chars.size()] = '\0';
			std::string storage_json_str(data.data());
			return fc::json::from_string(storage_json_str);
		}

		GluaStorageValue StorageDataType::create_lua_storage_from_storage_data(lua_State *L, const StorageDataType& storage)
		{
			auto json_value = json_from_chars(storage.storage_data);
			auto value = json_to_uvm_storage_value(L, json_value);
			return value;
		}
	}
}
