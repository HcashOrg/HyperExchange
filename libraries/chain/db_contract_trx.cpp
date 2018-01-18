#include <graphene/chain/database.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <uvm/uvm_api.h>

namespace graphene {
	namespace chain {
		StorageDataType database::get_contract_storage(const address& contract_id, const string& name)
		{
			try {
				auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
				auto itr = index.find(contract_id);
				if (itr == index.end())
				{
					std::string null_jsonstr("null");
					return StorageDataType(null_jsonstr);
				}
				auto &contract = *itr;
				auto storage_itr = contract.storages.find(name);
				if (storage_itr == contract.storages.end())
				{
					std::string null_jsonstr("null");
					return StorageDataType(null_jsonstr);
				}
				const auto &storage_data = storage_itr->second;
				StorageDataType storage;
				storage.storage_data = storage_data;
				return storage;
			} FC_CAPTURE_AND_RETHROW((contract_id)(name));
		}

		void database::set_contract_storage(const address& contract_id, const string& name, const StorageDataType &value)
		{
			try {
				auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
				auto itr = index.find(contract_id);
				FC_ASSERT(itr != index.end());
				auto &contract = *itr;
				auto& con_db = get_index_type<contract_object_index>().indices().get<by_contract_id>();
				auto con = con_db.find(contract.contract_address);
				FC_ASSERT(con != con_db.end());
				modify(contract, [&](contract_object& obj) {
					obj.storages[name] = value.storage_data;
				});
			} FC_CAPTURE_AND_RETHROW((contract_id)(name)(value));
		}

		void database::set_contract_storage_in_contract(const contract_object& contract, const string& name, const StorageDataType& value)
		{
			try {
				auto& con_db = get_index_type<contract_object_index>().indices().get<by_contract_id>();
				auto con = con_db.find(contract.contract_address);
				FC_ASSERT(con != con_db.end());
				modify(contract, [&](contract_object& obj) {
					obj.storages[name] = value.storage_data;
				});
			} FC_CAPTURE_AND_RETHROW((contract.contract_address)(name)(value));
		}

		void database::add_contract_storage_change(const address& contract_id, const string& name, const StorageDataType &diff)
		{
			try {
				transaction_contract_storage_diff_object obj;
				obj.contract_address = contract_id;
				obj.storage_name = name;
				obj.diff = diff.storage_data;
				auto& con_db = get_index_type<transaction_contract_storage_diff_index>().indices().get<by_contract_id>();
				auto con = con_db.find(contract_id);
				create<transaction_contract_storage_diff_object>([&](transaction_contract_storage_diff_object & o) {
						o = obj;
				});
			} FC_CAPTURE_AND_RETHROW((contract_id)(name)(diff));
		}

        void database::store_contract(const contract_object & contract)
        {
            try {
            auto& con_db = get_index_type<contract_object_index>().indices().get<by_contract_id>();
            auto con = con_db.find(contract.contract_address);
            if (con == con_db.end())
            {
                create<contract_object>([contract](contract_object & obj) {
                    obj.create_time = contract.create_time;
                    obj.code = contract.code; 
                    obj.name = contract.name;
                    obj.owner_address = contract.owner_address;
                    obj.contract_address = contract.contract_address;
                    obj.storages = obj.storages;
                });
            }
            else
            {
                FC_ASSERT( false,"contract exsited");
            }
            }FC_CAPTURE_AND_RETHROW((contract))
        }

        contract_object database::get_contract(const address & contract_address)
        {
            auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
            auto itr = index.find(contract_address);
            FC_ASSERT(itr != index.end());
            return *itr;
        }

        contract_object database::get_contract(const contract_id_type & id)
        {
            auto& index = get_index_type<contract_object_index>().indices().get<by_id>();
            auto itr = index.find(id);
            FC_ASSERT(itr != index.end());
            return *itr;
        }

		bool database::has_contract(const address& contract_address)
		{
			auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
			auto itr = index.find(contract_address);
			return itr != index.end();
		}

	}
}
