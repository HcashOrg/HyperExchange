#include <graphene/chain/database.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/contract_object.hpp>
#include <uvm/uvm_api.h>

namespace graphene {
	namespace chain {
		StorageDataType database::get_contract_storage(const address& contract_id, const string& name)
		{
			try {
				auto& storage_index = get_index_type<contract_storage_object_index>().indices().get<by_contract_id_storage_name>();
				auto storage_iter = storage_index.find(boost::make_tuple(contract_id, name));
				if (storage_iter == storage_index.end())
				{
					std::string null_jsonstr("null");
					return StorageDataType(null_jsonstr);
				}
				else
				{
					const auto &storage_data = *storage_iter;
					StorageDataType storage;
					storage.storage_data = storage_data.storage_value;
					return storage;
				}
			} FC_CAPTURE_AND_RETHROW((contract_id)(name));
		}

		void database::set_contract_storage(const address& contract_id, const string& name, const StorageDataType &value)
		{
			try {
				/*auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
				auto itr = index.find(contract_id);
				FC_ASSERT(itr != index.end());*/

				auto& storage_index = get_index_type<contract_storage_object_index>().indices().get<by_contract_id_storage_name>();
				auto storage_iter = storage_index.find(boost::make_tuple(contract_id, name));
				if (storage_iter == storage_index.end()) {
					create<contract_storage_object>([&](contract_storage_object & obj) {
						obj.contract_address = contract_id;
						obj.storage_name = name;
						obj.storage_value = value.storage_data;
					});
				}
				else {
					modify(*storage_iter, [&](contract_storage_object& obj) {
						obj.storage_value = value.storage_data;
					});
				}
			} FC_CAPTURE_AND_RETHROW((contract_id)(name)(value));
		}

		void database::set_contract_storage_in_contract(const contract_object& contract, const string& name, const StorageDataType& value)
		{
			try {
				set_contract_storage(contract.contract_address, name, value);
			} FC_CAPTURE_AND_RETHROW((contract.contract_address)(name)(value));
		}

		void database::add_contract_storage_change(const transaction_id_type& trx_id, const address& contract_id, const string& name, const StorageDataType &diff)
		{
			try {
				transaction_contract_storage_diff_object obj;
				obj.contract_address = contract_id;
				obj.storage_name = name;
				obj.diff = diff.storage_data;
				obj.trx_id = trx_id;
				create<transaction_contract_storage_diff_object>([&](transaction_contract_storage_diff_object & o) {
					o.contract_address = obj.contract_address;
					o.diff = obj.diff;
					o.storage_name = obj.storage_name;
					o.trx_id = obj.trx_id;
				});
			} FC_CAPTURE_AND_RETHROW((trx_id)(contract_id)(name)(diff));
		}

		void database::add_contract_event_notify(const transaction_id_type& trx_id, const address& contract_id, const string& event_name, const string& event_arg)
		{
			try {
				contract_event_notify_object obj;
				obj.trx_id = trx_id;
				obj.contract_address = contract_id;
				obj.event_name = event_name;
				obj.event_arg = event_arg;
				auto& conn_db = get_index_type<contract_event_notify_index>().indices().get<by_contract_id>();
				create<contract_event_notify_object>([&](contract_event_notify_object & o) {
					o.contract_address = obj.contract_address;
					o.trx_id = obj.trx_id;
					o.event_name = obj.event_name;
					o.event_arg = obj.event_arg;
				});
			} FC_CAPTURE_AND_RETHROW((trx_id)(contract_id)(event_name)(event_arg));
		}
        bool object_id_type_comp(const object& obj1, const object& obj2)
        {
            return obj1.id < obj2.id;
        }
        vector<contract_event_notify_object> database::get_contract_event_notify(const address & contract_id, const transaction_id_type & trx_id, const string& event_name)
        {
            try {
                bool trx_id_check = true;
                if (trx_id == transaction_id_type())
                    trx_id_check = false;
                bool event_check = true;
                if (event_name == "")
                    event_check = false;
                vector<contract_event_notify_object> res;
                auto& conn_db=get_index_type<contract_event_notify_index>().indices().get<by_contract_id>();
                auto lb=conn_db.lower_bound(contract_id);
                auto ub = conn_db.upper_bound(contract_id);
                while (lb!=ub)
                {
                    if (trx_id_check&&lb->trx_id != trx_id)
                    {
                        lb++;
                        continue;
                    }
                    if (event_check&&lb->event_name != event_name)
                    {
                        lb++;
                        continue;
                    }
                    res.push_back(*lb);
                    lb++;
                }
                std::sort(res.begin(), res.end(), object_id_type_comp);
                return res;
            } FC_CAPTURE_AND_RETHROW((contract_id)(trx_id)(event_name));
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
					obj.contract_name = contract.contract_name;
					obj.contract_desc = contract.contract_desc;
                    obj.contract_address = contract.contract_address;
					obj.type_of_contract = contract.type_of_contract;
					obj.native_contract_key = contract.native_contract_key;
                });
            }
            else
            {
                FC_ASSERT( false,"contract exsited");
            }
            }FC_CAPTURE_AND_RETHROW((contract))
        }

		void database::update_contract(const contract_object& contract)
		{
			try {
				auto& con_db = get_index_type<contract_object_index>().indices().get<by_contract_id>();
				auto con = con_db.find(contract.contract_address);
				if (con != con_db.end())
				{
					modify(*con, [&](contract_object& obj) {
						obj.create_time = contract.create_time;
						obj.code = contract.code;
						obj.name = contract.name;
						obj.owner_address = contract.owner_address;
						obj.contract_name = contract.contract_name;
						obj.contract_desc = contract.contract_desc;
						obj.contract_address = contract.contract_address;
						obj.type_of_contract = contract.type_of_contract;
						obj.native_contract_key = contract.native_contract_key;
					});
				}
				else
				{
					FC_ASSERT(false, "contract not exsited");
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

		contract_object database::get_contract_of_name(const string& contract_name)
		{
			auto& index = get_index_type<contract_object_index>().indices().get<by_contract_name>();
			auto itr = index.find(contract_name);
			FC_ASSERT(itr != index.end());
			return *itr;
		}

		bool database::has_contract(const address& contract_address)
		{
			auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
			auto itr = index.find(contract_address);
			return itr != index.end();
		}

        void database::set_min_gas_price(const share_type min_price)
        {
            _min_gas_price = min_price;
        }
        share_type database::get_min_gas_price() const
        {
            return _min_gas_price;
        }
		bool database::has_contract_of_name(const string& contract_name)
		{
			auto& index = get_index_type<contract_object_index>().indices().get<by_contract_name>();
			auto itr = index.find(contract_name);
			return itr != index.end();
		}

        asset database::get_contract_balance(const address & addr, const asset_id_type & asset_id)
        {
            try {
                auto contract_idx = get_contract(addr);
            }
            catch (...)
            {
                FC_CAPTURE_AND_THROW(blockchain::contract_engine::contract_not_exsited, (addr));
            }

            auto& bal_idx = get_index_type<contract_balance_index>();
            const auto& by_owner_idx = bal_idx.indices().get<by_owner>();
            //subscribe_to_item(addr);
            auto itr = by_owner_idx.find(boost::make_tuple(addr, asset_id));
            asset result(0, asset_id);
            if (itr != by_owner_idx.end() && itr->owner == addr)
            {
                result += (*itr).balance;
            }
            return result;
        }


        void database::adjust_contract_balance(const address & addr, const asset & delta)
        {
            try {
                if (delta.amount == 0)
                    return;
                auto& by_owner_idx = get_index_type<contract_balance_index>().indices().get<by_owner>();
                auto itr = by_owner_idx.find(boost::make_tuple(addr, delta.asset_id));
                fc::time_point_sec now = head_block_time();
                if (itr == by_owner_idx.end())
                {
                    FC_ASSERT(delta.amount > 0, "Insufficient Balance: ${a}'s balance of ${b} is less than required ${r}",
                        ("a", addr)
                        ("b", to_pretty_string(asset(0, delta.asset_id)))
                        ("r", to_pretty_string(-delta)));
                    create<contract_balance_object>([addr, delta, now](contract_balance_object& b) {
                        b.owner = addr;
                        b.balance = delta;
                        b.last_claim_date = now;
                    });
                }
                else
                {
                    if (delta.amount < 0)
                        FC_ASSERT(itr->balance >= -delta, "Insufficient Balance: ${a}'s balance of ${b} is less than required ${r}", ("a", addr)("b", to_pretty_string(itr->balance))("r", to_pretty_string(-delta)));
                    modify(*itr, [delta, now](contract_balance_object& b) {
                        b.adjust_balance(delta, now);
                    });

                }


            }FC_CAPTURE_AND_RETHROW((addr)(delta))
        }
        address database::get_account_address(const string& name)
        {
            auto& db = get_index_type<account_index>().indices().get<by_name>();
            auto it = db.find(name);
            if (it != db.end())
            {
                return it->addr;
            }
            return address();
        }


	}
}
