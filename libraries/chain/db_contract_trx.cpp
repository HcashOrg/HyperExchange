#include <graphene/chain/database.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>

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

	}
}
