#include <graphene/chain/database.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/contract_object.hpp>
#include <fc/crypto/hex.hpp>
#include <uvm/uvm_api.h>
#include <cbor_diff/cbor_diff.h>
#include <fc/crypto/ripemd160.hpp>
#include <iostream>
namespace graphene {
	namespace chain {
		StorageDataType database::get_contract_storage(const address& contract_id, const string& name)
		{
			try {
				//auto& storage_index = get_index_type<contract_storage_object_index>().indices().get<by_contract_id_storage_name>();
				//auto storage_iter = storage_index.find(boost::make_tuple(contract_id, name));
				//if (storage_iter == storage_index.end())
				//{
				//	auto cbor_null = cbor::CborObject::create_null();
				//	const auto& cbor_null_bytes = cbor_diff::cbor_encode(cbor_null);
				//	StorageDataType storage;
				//	storage.storage_data = cbor_null_bytes;
				//	return storage;
				//	// std::string null_jsonstr("null");
				//	// return StorageDataType(null_jsonstr);
				//}
				//else
				//{
				//	const auto &storage_data = *storage_iter;
				//	StorageDataType storage;
				//	storage.storage_data = storage_data.storage_value;
				//	return storage;
				//}

				auto contract_obj_op = get_contract_storage_object(contract_id, name);
				if (!contract_obj_op.valid())
				{
					auto cbor_null = cbor::CborObject::create_null();
					const auto& cbor_null_bytes = cbor_diff::cbor_encode(cbor_null);
					StorageDataType storage;
					storage.storage_data = cbor_null_bytes;
					return storage;
				}
				const auto &storage_data = *contract_obj_op;
				StorageDataType storage;
				storage.storage_data = storage_data.storage_value;
				return storage;
			} FC_CAPTURE_AND_RETHROW((contract_id)(name));
		}

		std::map<std::string, StorageDataType> database::get_contract_all_storages(const address& contract_id) {
			try {
				std::map<std::string, StorageDataType> result;
				auto& storage_index = get_index_type<contract_storage_object_index>().indices().get<by_storage_contract_id>();
				auto storage_iter = storage_index.find(contract_id);
				while (storage_iter != storage_index.end() && storage_iter->contract_address == contract_id)
				{
					StorageDataType storage;
					storage.storage_data = storage_iter->storage_value;
					result[storage_iter->storage_name] = storage;
					++storage_iter;
				}
				return result;
				/*leveldb::Iterator* it = get_contract_db()->NewIterator(leveldb::ReadOptions());
				auto start = contract_id.address_to_string()+"|set_storage|";
				for (it->Seek(start); it->Valid(); it->Next()) {
					if (!it->key().starts_with(start))
						break;
					auto value = it->value().ToString();
					if (it->key().ToString() == start)
						continue;
					vector<char> vec(value.begin(),value.end());
					contract_storage_object obj = fc::raw::unpack<contract_storage_object>(vec);
					StorageDataType storage;
					storage.storage_data = obj.storage_value;
					result[obj.storage_name] = storage;
				}
				delete it;
				return result;*/

			} FC_CAPTURE_AND_RETHROW((contract_id));
		}

		optional<contract_storage_object> database::get_contract_storage_object(const address& contract_id, const string& name)
		{
			try {
				auto& storage_index = get_index_type<contract_storage_object_index>().indices().get<by_contract_id_storage_name>();
				auto storage_iter = storage_index.find(boost::make_tuple(contract_id, name));
				if (storage_iter == storage_index.end())
				{
					return optional<contract_storage_object>();
				}
				else
				{
					return *storage_iter;
				}
				/*auto start = contract_id.address_to_string()+"|set_storage|"+name;
				string value;
				leveldb::ReadOptions read_options;
				leveldb::Status sta = get_contract_db()->Get(leveldb::ReadOptions(), start, &value);
				if (sta.ok())
				{
					vector<char> vec(value.begin(),value.end());
					contract_storage_object obj = fc::raw::unpack<contract_storage_object>(vec);
					return obj;
				}*/
				return optional<contract_storage_object>();
			} FC_CAPTURE_AND_RETHROW((contract_id)(name));
		}

		void database::set_contract_storage(const address& contract_id, const string& name, const StorageDataType &value)
		{
			try {
				auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
				auto itr = index.find(contract_id);
				FC_ASSERT(itr != index.end());
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
				/*contract_storage_object obj;
				obj.contract_address = contract_id;
				obj.storage_name = name;
				obj.storage_value = value.storage_data;
				leveldb::WriteOptions write_options;
				leveldb::ReadOptions read_options;
				string value;
				leveldb::Status sta = get_contract_db()->Get(leveldb::ReadOptions(), contract_id.address_to_string() + "|set_storage|", &value);
				if (!sta.ok())
				{
					sta = get_contract_db()->Put(write_options, contract_id.address_to_string() + "|set_storage|", "set_storage");
					FC_ASSERT(sta.ok(),"put data in contract db failed.");
				}
				const auto& vec = fc::raw::pack(obj);
				sta = get_contract_db()->Put(write_options, contract_id.address_to_string() + "|set_storage|"+ name, leveldb::Slice(vec.data(),vec.size()));
				if (!sta.ok())
				{
					elog("Put error: ${error}", ("error", (contract_id.address_to_string() + "|set_storage|" + name + ":" + sta.ToString()).c_str()));
					FC_ASSERT(false, "Put Data to contract db failed");
				}*/



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
				/*transaction_contract_storage_diff_object obj;
				obj.contract_address = contract_id;
				obj.storage_name = name;
				obj.diff = diff.storage_data;
				obj.trx_id = trx_id;
				create<transaction_contract_storage_diff_object>([&](transaction_contract_storage_diff_object & o) {
					o.contract_address = obj.contract_address;
					o.diff = obj.diff;
					o.storage_name = obj.storage_name;
					o.trx_id = obj.trx_id;
				});*/
				leveldb::WriteOptions write_options;
				leveldb::ReadOptions read_options;
				string value;
				transaction_contract_storage_diff_object obj;
				obj.contract_address = contract_id;
				obj.storage_name = name;
				obj.diff = diff.storage_data;
				obj.trx_id = trx_id;
				//leveldb::Status sta = get_contract_db()->Get(leveldb::ReadOptions(), trx_id.str()+"|storage_diff_object|", &value);
				leveldb::Status sta = l_db.Get(leveldb::ReadOptions(), trx_id.str() + "|storage_diff_object|", &value,get_contract_db());
				if (!sta.ok())
				{
					//sta = get_contract_db()->Put(write_options,trx_id.str()+"|storage_diff_object|", "storage_diff_object");
					sta = l_db.Put(trx_id.str() + "|storage_diff_object|", "storage_diff_object");
					FC_ASSERT(sta.ok(), "put data in contract db failed.");
				}
				const auto& vec = fc::raw::pack(obj);
				//sta = get_contract_db()->Put(write_options, trx_id.str() + "|storage_diff_object|" \
				//	+ contract_id.address_to_string() + "|" \
				//	+ name, leveldb::Slice(vec.data(),vec.size()));
				sta = l_db.Put(trx_id.str() + "|storage_diff_object|" \
					+ contract_id.address_to_string() + "|" \
					+ name, leveldb::Slice(vec.data(),vec.size()));
				if (!sta.ok())
				{
					elog("Put error: ${error}", ("error", (trx_id.str()+"|storage_diff_object|"+contract_id.address_to_string() + "|" + name + ":" + sta.ToString()).c_str()));
					FC_ASSERT(false, "Put Data to contract failed");
				}

			} FC_CAPTURE_AND_RETHROW((trx_id)(contract_id)(name)(diff));
		}
        void  database::store_contract_storage_change_obj(const address& contract, uint32_t block_num)
		{
            try {
                auto& conn_db = get_index_type<contract_storage_change_index>().indices().get<by_contract_id>();
                auto it = conn_db.find(contract);
                if (it == conn_db.end())
                {
                    create<contract_storage_change_object>([&](contract_storage_change_object& o)
                    {
                        o.contract_address = contract;
                        o.block_num = block_num;
                    });
                }else
                {
                    modify<contract_storage_change_object>(*it, [&](contract_storage_change_object& obj) {obj.block_num = block_num; });
                }
            }FC_CAPTURE_AND_RETHROW((contract)(block_num));
		}
        vector<contract_blocknum_pair> database::get_contract_changed(uint32_t block_num, uint32_t duration)
		{
            try
            {
                contract_blocknum_pair tmp;
                vector<contract_blocknum_pair> res;
                auto& conn_db = get_index_type<contract_storage_change_index>().indices().get<by_block_num>();
                auto lb = conn_db.lower_bound(block_num);
                uint32_t last_block_num = block_num + duration;
                if(duration==0)
                {
                    last_block_num = head_block_num();
                }
                auto ub = conn_db.lower_bound(last_block_num);
                while(lb!=ub)
                {
                    tmp.block_num = lb->block_num;
                    tmp.contract_address = lb->contract_address.operator fc::string();
                    res.push_back(tmp);
                    lb++;
                }
                return res;
            }FC_CAPTURE_AND_RETHROW((block_num)(duration));
		}
		void database::add_contract_event_notify(const transaction_id_type& trx_id, const address& contract_id, const string& event_name, const string& event_arg, uint64_t block_num, uint64_t
		                                         op_num)
		{
			try {
				contract_event_notify_object obj;
				obj.trx_id = trx_id;
				obj.contract_address = contract_id;
				obj.event_name = event_name;
				obj.event_arg = event_arg;
                obj.block_num = block_num;
                obj.op_num = op_num;
				auto& conn_db = get_index_type<contract_event_notify_index>().indices().get<by_contract_id>();
				create<contract_event_notify_object>([&](contract_event_notify_object & o) {
					o.contract_address = obj.contract_address;
					o.trx_id = obj.trx_id;
					o.event_name = obj.event_name;
					o.event_arg = obj.event_arg;
                    o.block_num = obj.block_num;
                    o.op_num = obj.op_num;

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
		void database::remove_contract_invoke_result(const transaction_id_type& trx_id, const uint32_t op_num)
		{
			try {
				leveldb::WriteOptions write_ops;
				auto start = trx_id.str() + "|invoke_result|" + fc::variant(op_num).as_string();
				//get_contract_db()->Delete(write_ops,start);
				l_db.Delete(start);
			}FC_CAPTURE_AND_RETHROW((trx_id)(op_num));
		}

		optional<contract_invoke_result_object> database::get_contract_invoke_result(const transaction_id_type& trx_id, const uint32_t op_num) const
		{
			try {
				leveldb::ReadOptions read_options;
				auto start = trx_id.str() + "|invoke_result|" + fc::variant(op_num).as_string();
				string value;
				leveldb::Status sta = get_cache_contract_db()->Get(read_options, start, &value, get_contract_db());
				if (sta.ok())
				{
					vector<char> vec (value.begin(),value.end());
					contract_invoke_result_object obj = fc::raw::unpack<contract_invoke_result_object>(vec);
					return obj;
				}
				return optional<contract_invoke_result_object>();
			}FC_CAPTURE_AND_RETHROW((trx_id)(op_num));
		}

        vector<contract_invoke_result_object> database::get_contract_invoke_result(const transaction_id_type& trx_id)const
        {
            try {
                vector<contract_invoke_result_object> res;
				/*auto& res_db = get_index_type<contract_invoke_result_index>().indices().get<by_trxid>();
				auto it = res_db.lower_bound(trx_id);
				auto end_it= res_db.upper_bound(trx_id);
				while(it!=end_it)
				{
					res.push_back(*it);
					it++;
				}*/
				auto start = trx_id.str() + "|invoke_result|";
				leveldb::ReadOptions read_options;
				auto temp_to_return = get_cache_contract_db()->GetToDelete(read_options, start, get_contract_db(),false);
				for (auto value : temp_to_return) {
					vector<char> vec(value.begin(), value.end());
					contract_invoke_result_object obj = fc::raw::unpack<contract_invoke_result_object>(vec);
					res.push_back(obj);
				}
				/*
				leveldb::Iterator* it = get_contract_db()->NewIterator(leveldb::ReadOptions());
				auto start = trx_id.str()+"|invoke_result|";
				for (it->Seek(start); it->Valid(); it->Next()) {
					if (!it->key().starts_with(start))
						break;
					auto value = it->value().ToString();
					if (it->key().ToString() == start)
						continue;
					vector<char> vec(value.begin(),value.end());
					contract_invoke_result_object obj = fc::raw::unpack<contract_invoke_result_object>(vec);
					res.push_back(obj);
				}
				delete it;*/
                std::sort(res.begin(), res.end());
                return res;
            }FC_CAPTURE_AND_RETHROW((trx_id))
        }
		void graphene::chain::database::store_contract_related_transaction(const transaction_id_type& tid, const address& contract_id)
		{
			auto block_num = head_block_num()+1;
			create<contract_history_object>([tid, contract_id, block_num](contract_history_object& obj) {
				obj.block_num = block_num;
				obj.contract_id = contract_id;
				obj.trx_id = tid;
			});
		}
		void database::contract_packed(const signed_transaction& trx, const uint32_t num)
		{
			try {
				auto trx_id = trx.id();
				leveldb::ReadOptions read_options;
				leveldb::WriteOptions write_options;
				//neeed to packed trx
				string value;
				//leveldb::Status sta = get_contract_db()->Get(read_options, trx_id.str() + "|invoke_result|", &value);
				leveldb::Status sta = l_db.Get(read_options, trx_id.str() + "|invoke_result|", &value, get_contract_db());
				if (!sta.ok())
					return;
				std::cout << "contract_packed ..." << std::endl;
				vector<string> need_to_erase;
				vector<string> contract_ids;
				vector<string> storage_names;
				vector<transaction_contract_storage_diff_object> diff_objs;
				auto start = trx_id.str() + "|invoke_result|";
				auto temp_to_erase = l_db.GetToDelete(read_options, start, get_contract_db());
				need_to_erase.insert(need_to_erase.end(), temp_to_erase.begin(), temp_to_erase.end());
				start = trx_id.str() + "|storage_diff_object|";
				temp_to_erase.clear();
				vector<string>().swap(temp_to_erase);
				temp_to_erase = l_db.GetToDelete(read_options, start, get_contract_db());
				need_to_erase.insert(need_to_erase.end(), temp_to_erase.begin(), temp_to_erase.end());
					/*
				leveldb::Iterator* it = get_contract_db()->NewIterator(read_options);
				
				for (it->Seek(start); it->Valid(); it->Next()) {
					if (!it->key().starts_with(start))
						break;
					if (it->key().ToString() == start)
						continue;
					std::cout << "invoke_result " << it->key().ToString() << ":" << it->value().ToString() << std::endl;
					need_to_erase.push_back(it->key().ToString());
				}
				
				for (it->Seek(start); it->Valid(); it->Next()) {
					if (!it->key().starts_with(start))
						break;
					if (it->key().ToString() == start)
						continue;
					need_to_erase.push_back(it->key().ToString());*/
					/*auto key = it->key().ToString();
					auto value = it->value().ToString();
					std::cout <<"storage diff object " << key << ":" << value << std::endl;
					vector<char> vec(value.begin(),value.end());
					transaction_contract_storage_diff_object obj = fc::raw::unpack<transaction_contract_storage_diff_object>(vec);
					diff_objs.push_back(obj);
					auto contract_name = key.assign(key.data(), start.length());
					std::cout << "contract name " << contract_name << std::endl;*/
					/*auto contract = contract_name.assign(contract_name, 0, contract_name.find_first_of("|"));
					auto name = contract_name.assign(contract_name, contract_name.find_first_of("|") + 1);*/
					//contract_ids.push_back(contract_name);
					//storage_names.push_back(name);
				//}
				//delete it;
				for (const auto key : need_to_erase)
				{
					//get_contract_db()->Delete(write_options, key);
					l_db.Delete(key);
				}
				
				/*for (const auto obj : diff_objs)
				{
					auto diff = obj.diff;
					auto c_diff = cbor_diff::cbor_decode(diff);
					auto obj_op = get_contract_storage_object(obj.contract_address, obj.storage_name);
					if (!obj_op.valid())
						continue;
					auto storage_obj = *obj_op;
					auto storage_new = cbor_diff::cbor_decode(storage_obj.storage_value);
					cbor_diff::CborDiff differ;
					auto c_old = differ.rollback(storage_new, std::make_shared<cbor_diff::DiffResult>(*c_diff));
					StorageDataType data;
					data.storage_data = cbor_diff::cbor_encode(c_old);
					set_contract_storage(obj.contract_address, obj.storage_name, data);
				}*/
			}FC_CAPTURE_AND_RETHROW((trx)(num))
		}
		std::vector<graphene::chain::transaction_id_type> database::get_contract_related_transactions(const address& contract_id, uint64_t start, uint64_t end)
		{
			std::vector<graphene::chain::transaction_id_type> res;
			if (end < start)
				return res;
			auto head_num = head_block_num();
			if (head_num<end)
			{
				end = head_num;
			}
			auto& res_db = get_index_type<contract_history_object_index>().indices().get<by_contract_id_and_block_num>();
			auto start_it = res_db.lower_bound(std::make_tuple(contract_id,start));
			auto end_it = res_db.upper_bound(std::make_tuple(contract_id, end));
			while (start_it != end_it)
			{
				res.push_back(start_it->trx_id);
				start_it++;
			}
			return res;

		}
        void database::store_invoke_result(const transaction_id_type& trx_id, int op_num, const contract_invoke_result& res, const share_type& gas)
        {
            try {
				/* auto& con_db = get_index_type<contract_invoke_result_index>().indices().get<by_trxid_and_opnum>();
				 auto con = con_db.find(boost::make_tuple(trx_id,op_num));*/
                auto block_num=head_block_num();
                address invoker = res.invoker;
				_current_gas_in_block += gas;
				/*if (con == con_db.end())
				{
					create<contract_invoke_result_object>([res,trx_id, op_num, block_num, invoker](contract_invoke_result_object & obj) {
						obj.acctual_fee = res.acctual_fee;
						obj.api_result = res.api_result;
						obj.exec_succeed = res.exec_succeed;
						obj.events = res.events;
						obj.trx_id = trx_id;
						obj.block_num = block_num+1;
						obj.op_num = op_num;
						obj.invoker = invoker;
						obj.contract_registed = res.contract_registed;
						for (auto it = res.contract_withdraw.begin(); it != res.contract_withdraw.end(); it++)
						{
							obj.contract_withdraw.insert( make_pair(it->first,it->second));
						}
						for (auto it = res.contract_balances.begin(); it != res.contract_balances.end(); it++)
						{
							obj.contract_balances.insert(make_pair(it->first, it->second));
						}
						for (auto it = res.deposit_contract.begin(); it != res.deposit_contract.end(); it++)
						{
							obj.deposit_contract.insert(make_pair(it->first, it->second));
						}
						for (auto it = res.deposit_to_address.begin(); it != res.deposit_to_address.end(); it++)
						{
							obj.deposit_to_address.insert(make_pair(it->first, it->second));
						}
						for (auto it = res.transfer_fees.begin(); it != res.transfer_fees.end(); it++)
						{
							obj.transfer_fees.insert(make_pair(it->first, it->second));
						}
					});
				}
				else
				{
					FC_ASSERT(false, "result exsited");
				}*/

				leveldb::WriteOptions write_options;
				leveldb::ReadOptions read_options;
				contract_invoke_result_object obj;
				obj.acctual_fee = res.acctual_fee;
				obj.gas = gas;
				obj.api_result = res.api_result;
				obj.exec_succeed = res.exec_succeed;
				/*if (get_global_properties().event_need)*/
				obj.events = res.events;
				obj.trx_id = trx_id;
				obj.block_num = block_num + 1;
				obj.op_num = op_num;
				obj.invoker = invoker;
				obj.contract_registed = res.contract_registed;
				for (auto it = res.storage_changes.begin(); it != res.storage_changes.end(); it++)
				{
					obj.storage_changes.insert(make_pair(it->first,it->second));
				}
				for (auto it = res.contract_withdraw.begin(); it != res.contract_withdraw.end(); it++)
				{
					obj.contract_withdraw.insert(make_pair(it->first, it->second));
				}
				for (auto it = res.contract_balances.begin(); it != res.contract_balances.end(); it++)
				{
					obj.contract_balances.insert(make_pair(it->first, it->second));
				}
				for (auto it = res.deposit_contract.begin(); it != res.deposit_contract.end(); it++)
				{
					obj.deposit_contract.insert(make_pair(it->first, it->second));
				}
				for (auto it = res.deposit_to_address.begin(); it != res.deposit_to_address.end(); it++)
				{
					obj.deposit_to_address.insert(make_pair(it->first, it->second));
				}
				for (auto it = res.transfer_fees.begin(); it != res.transfer_fees.end(); it++)
				{
					obj.transfer_fees.insert(make_pair(it->first, it->second));
				}
				string value;
				//leveldb::Status sta = get_contract_db()->Get(read_options, trx_id.str()+"|invoke_result|", &value);
				leveldb::Status sta = l_db.Get(read_options, trx_id.str() + "|invoke_result|", &value, get_contract_db());
				if (!sta.ok())
				{
					const auto& vec = fc::raw::pack(op_num);
					//sta = get_contract_db()->Put(write_options, trx_id.str() + "|invoke_result|", leveldb::Slice(vec.data(),vec.size()));
					sta = l_db.Put( trx_id.str() + "|invoke_result|", leveldb::Slice(vec.data(), vec.size()));
					FC_ASSERT(sta.ok(), "Put Data to contract db failed");
				}
				const auto& vec = fc::raw::pack(obj);
				//sta = get_contract_db()->Put(write_options, trx_id.str()+"|invoke_result|"+fc::variant(op_num).as_string(), leveldb::Slice(vec.data(),vec.size()));
				sta = l_db.Put(trx_id.str() + "|invoke_result|" + fc::variant(op_num).as_string(), leveldb::Slice(vec.data(), vec.size()));
				if (!sta.ok())
				{
					elog("Put error: ${error}", ("error", (trx_id.str() + "|invoke_result|" + fc::variant(op_num).as_string()).c_str()));
					FC_ASSERT(false, "Put Data to contract db failed");
					return;
				}
            }FC_CAPTURE_AND_RETHROW((trx_id))
        }
        class event_compare
        {
		public:
            const database* _db;
            event_compare(const database* _db):_db(_db){}
            bool operator()(const contract_event_notify_object& obj1,const contract_event_notify_object& obj2)
            {
                if (obj1.block_num < obj2.block_num)
                    return true;
                if (obj1.block_num > obj2.block_num)
                    return false;
                auto blk=_db->fetch_block_by_number(obj1.block_num);
                FC_ASSERT(blk.valid());
                for(auto trx:blk->transactions)
                {
                    if (trx.id() == obj1.trx_id)
                        return true;
                    if (trx.id() == obj2.trx_id)
                        return false;
                }
                return false;
            }
        };
        vector<contract_event_notify_object> database::get_contract_events_by_contract_ordered(const address &addr) const
		{
            vector<contract_event_notify_object> res;
            auto& con_db = get_index_type<contract_event_notify_index>().indices().get<by_contract_id>();

            auto evit = con_db.lower_bound(addr);
            auto ubit = con_db.upper_bound(addr);
            while(evit != ubit )
            {
                res.push_back(*evit);
                evit++;
            }
            event_compare cmp(this);
            sort(res.begin(),res.end(), cmp);
            return res;
		}
		vector<contract_event_notify_object> database::get_contract_events_by_block_and_addr_ordered(const address &addr, uint64_t start, uint64_t range) const
		{
			vector<contract_event_notify_object> res;
			auto& con_db = get_index_type<contract_event_notify_index>().indices().get<by_block_num>();

			auto evit = con_db.lower_bound(start);
			auto ubit = con_db.upper_bound(start+ range);
			while (evit != ubit)
			{
				if(evit->contract_address==addr)
					res.push_back(*evit);
				evit++;
			}
			event_compare cmp(this);
			sort(res.begin(), res.end(), cmp);
			return res;
		}

        vector<contract_object> database::get_registered_contract_according_block(const uint32_t start_with, const uint32_t num)const
        {
            vector<contract_object> res;
            auto& con_db = get_index_type<contract_object_index>().indices().get<by_registered_block>();
            auto evit = con_db.lower_bound(start_with);

            auto ubit = con_db.upper_bound(start_with + num);
            if (num == 0|| start_with + num<start_with)
                ubit = con_db.upper_bound(head_block_num());
            while (evit != ubit)
            {
                res.push_back(*evit);
                evit++;
            }
            return res;
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
                    obj.inherit_from = contract.inherit_from;
                    obj.derived = contract.derived;
                    obj.registered_block = contract.registered_block;
					obj.registered_trx = contract.registered_trx;
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
                        obj.inherit_from = contract.inherit_from;
                        obj.derived = contract.derived;
                        obj.registered_block = contract.registered_block;
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
            contract_object res;
            auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
            auto itr = index.find(contract_address);
            FC_ASSERT(itr != index.end(), "database::get_contract contract not found");
            res =*itr;
            if(res.inherit_from!= address())
            {
                res.code = get_contract(res.inherit_from).code;
            }
            return res;
        }

        contract_object database::get_contract(const string& name_or_id)
        {
            auto& index_name = get_index_type<contract_object_index>().indices().get<by_contract_name>();
            auto itr = index_name.find(name_or_id);
            if(itr != index_name.end())
            {
                contract_object res;
                res = *itr;
                if (res.inherit_from != address())
                {
                    res.code = get_contract(res.inherit_from).code;
                }
                return res;
            }
            return get_contract(address(name_or_id));
        }
        contract_object database::get_contract(const contract_id_type & id)
        {
            auto& index = get_index_type<contract_object_index>().indices().get<by_id>();
            auto itr = index.find(id);
            FC_ASSERT(itr != index.end(), "contract id not found");
            return *itr;
        }

		contract_object database::get_contract_of_name(const string& contract_name)
		{
			auto& index = get_index_type<contract_object_index>().indices().get<by_contract_name>();
			auto itr = index.find(contract_name);
			FC_ASSERT(itr != index.end());
			return *itr;
		}
        vector<contract_object> database::get_contract_by_owner(const address& owner)
		{
            vector<contract_object> res;
            auto& index = get_index_type<contract_object_index>().indices().get<by_owner>();
            auto itr = index.lower_bound(owner);
            auto itr_end = index.upper_bound(owner);
            while(itr!= itr_end)
            {
                res.push_back(*itr);
                itr++;
            }
            return res;
		}
        vector<address> database::get_contract_address_by_owner(const address& owner)
		{
            vector<address> res;
            auto& index = get_index_type<contract_object_index>().indices().get<by_owner>();
            auto itr = index.lower_bound(owner);
            auto itr_end = index.upper_bound(owner);
            while (itr != itr_end)
            {
                res.push_back(itr->contract_address);
                itr++;
            }
            return res;
		}
		bool database::has_contract(const address& contract_address, const string& method/*=""*/)
		{
			auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
			auto itr = index.find(contract_address);
			if(method=="")
				return itr != index.end();
			if (itr->type_of_contract == contract_based_on_template)
			{
				auto itrbase=index.find(itr->inherit_from);
				FC_ASSERT(itrbase != index.end());
				auto& apis = itrbase->code.abi;
				auto& offline_apis = itrbase->code.offline_abi;
				return apis.find(method) != apis.end() || offline_apis.find(method) != offline_apis.end();
			}
			auto& apis = itr->code.abi;
			auto& offline_apis = itr->code.offline_abi;
			return apis.find(method) != apis.end()|| offline_apis.find(method)!= offline_apis.end();
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

        address database::get_account_address(const string& name) const
        {
            auto& db = get_index_type<account_index>().indices().get<by_name>();
            auto it = db.find(name);
            if (it != db.end())
            {
                return it->addr;
            }
            return address();
        }
		SecretHashType database::get_random_padding(bool is_random) {
			fc::ripemd160::encoder enc;
			if (is_random) {
				fc::raw::pack(enc, _current_secret_key);
				fc::raw::pack(enc, _current_trx_in_block);
				fc::raw::pack(enc, _current_op_in_trx);
				fc::raw::pack(enc, _current_contract_call_num);
				++_current_contract_call_num;
			}
			else {
				fc::raw::pack(enc, _current_secret_key);
			}
			
			return enc.result();

		}

	}
}
