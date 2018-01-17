#include <graphene/chain/database.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/transaction_object.hpp>

namespace graphene {
	namespace chain {
		StorageDataType database::get_contract_storage(const address& contract_id, const string& name)
		{
			try {
				// TODO:
				StorageDataType storage_data(std::string("\"TODO\""));
				return storage_data;
			} FC_CAPTURE_AND_RETHROW((contract_id)(name));
		}

		void database::set_contract_storage(const address& contract_id, const string& name, const StorageDataType &value)
		{
			try {
				// TODO:
			} FC_CAPTURE_AND_RETHROW((contract_id)(name)(value));
		}

		void database::add_contract_storage_change(const address& contract_id, const string& name, const StorageDataType &diff)
		{
			try {
				// TODO:
			} FC_CAPTURE_AND_RETHROW((contract_id)(name)(diff));
		}

        void database::store_contract(const contract_object & contract)
        {
            contract_object obj = contract;
            auto& con_db = get_index_type<contract_object_index>().indices().get<by_contract_id>();
            auto con = con_db.find(contract.contract_address);
            if (con == con_db.end())
            {
                create<contract_object>([&](contract_object & obj) {
                    obj = contract; });
            }
            else
            {
                FC_ASSERT( false,"contract exsited");
            }
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
            auto& index = get_index_type<contract_object_index>().indices().get<by_contract_obj_id>();
            auto itr = index.find(id);
            FC_ASSERT(itr != index.end());
            return *itr;
        }

	}
}
