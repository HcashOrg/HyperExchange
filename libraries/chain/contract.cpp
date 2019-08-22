#include <graphene/chain/contract.hpp>
#include <graphene/chain/contract_engine_builder.hpp>
#include <uvm/uvm_lib.h>

#include <fc/array.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>
#include <boost/uuid/sha1.hpp>
#include <exception>
#include <graphene/chain/contract_evaluate.hpp>
#include <boost/filesystem/operations.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <graphene/chain/contract_object.hpp>

namespace graphene {
	namespace chain {

		using namespace uvm::blockchain;
		using namespace jsondiff;

		void contract_invoke_result::reset()
		{
			api_result.clear();
			storage_changes.clear();
			contract_withdraw.clear();
			contract_balances.clear();
			deposit_to_address.clear();
			deposit_contract.clear();
            transfer_fees.clear();
            events.clear();
            exec_succeed = true;
		}

        void contract_invoke_result::set_failed(const share_type & fee)
        {
            api_result.clear();
            storage_changes.clear();
            contract_withdraw.clear();
            contract_balances.clear();
            deposit_to_address.clear();
            deposit_contract.clear();
            transfer_fees.clear();
            events.clear();
            exec_succeed = false;
            acctual_fee = fee;
        }

	void contract_invoke_result::validate() {
		// in >= out + fee
		std::map<asset_id_type, share_type> in_totals;
		std::map<asset_id_type, share_type> out_totals;
		for(const auto& p : contract_withdraw) {
			auto asset = p.first.second;
			auto change = p.second;
			if(change == 0)
				continue;
			// in
			share_type total = 0;
			if(in_totals.find(asset) != in_totals.end())
				total = in_totals[asset];
			total += change;
			in_totals[asset] = total;
		}
                for(const auto& p : deposit_contract) {
                        auto asset = p.first.second;
                        auto change = p.second;
                        if(change == 0)
                                continue;
                        // out
                        share_type total = 0;
                        if(out_totals.find(asset) != out_totals.end())
                                total = out_totals[asset];
                        total += change;
                        out_totals[asset] = total;
                }
                for(const auto& p : deposit_to_address) {
                        auto asset = p.first.second;
                        auto change = p.second;
                        if(change == 0)
                                continue;
                        // out
                        share_type total = 0;
                        if(out_totals.find(asset) != out_totals.end())
                                total = out_totals[asset];
                        total += change;
                        out_totals[asset] = total;
                }
		// TODO: add caller deposit amodeposit amounts to in. need split caller's deposit from contract_withdraws

		// add fees to out
		for (const auto& p : transfer_fees) {
			auto asset = p.first;
			auto change = p.second;
			share_type total = 0;
			if (out_totals.find(asset) != out_totals.end())
				total = out_totals[asset];
			total += change;
		}
		// each asset in out must have large in
		for (const auto& p : out_totals) {
			auto asset = p.first;
			auto out_total = p.second;
			if (out_total == 0) {
				continue;
			}
			if (in_totals.find(asset) == in_totals.end() || in_totals[asset] < out_total) {
				throw uvm::core::UvmException("contract evaluate result must in >= out + fee");
			}
		}	
	}

	void contract_invoke_result::transfer_from_obj(const contract_invoke_result_object& obj)
	{
		api_result = obj.api_result;
		events = obj.events;
		exec_succeed = obj.exec_succeed;
		acctual_fee = obj.acctual_fee;
		invoker = obj.invoker;
		contract_registed = obj.contract_registed;

		for (auto it = obj.contract_withdraw.begin(); it != obj.contract_withdraw.end(); it++)
		{
			contract_withdraw.insert(make_pair(it->first, it->second));
		}
		for (auto it = obj.contract_balances.begin(); it != obj.contract_balances.end(); it++)
		{
			contract_balances.insert(make_pair(it->first, it->second));
		}
		for (auto it = obj.deposit_contract.begin(); it != obj.deposit_contract.end(); it++)
		{
			deposit_contract.insert(make_pair(it->first, it->second));
		}
		for (auto it = obj.deposit_to_address.begin(); it != obj.deposit_to_address.end(); it++)
		{
			deposit_to_address.insert(make_pair(it->first, it->second));
		}
		for (auto it = obj.transfer_fees.begin(); it != obj.transfer_fees.end(); it++)
		{
			transfer_fees.insert(make_pair(it->first, it->second));
		}

	}


	int64_t contract_invoke_result::count_storage_gas() const {
		cbor_diff::CborDiff differ;
		int64_t storage_gas = 0;
		uvm::lua::lib::UvmStateScope scope;
		for (auto all_con_chg_iter = storage_changes.begin(); all_con_chg_iter != storage_changes.end(); ++all_con_chg_iter)
		{
			// commit change to evaluator
			contract_storage_changes_type contract_storage_change;
			std::string contract_id = all_con_chg_iter->first;
			auto contract_change = all_con_chg_iter->second;
			cbor::CborMapValue nested_changes;
			jsondiff::JsonObject json_nested_changes;

			for (auto con_chg_iter = contract_change.begin(); con_chg_iter != contract_change.end(); ++con_chg_iter)
			{
				std::string contract_name = con_chg_iter->first;

				StorageDataChangeType storage_change;
				// storage_op from before and after to diff
				auto storage_after = con_chg_iter->second.after;
				auto cbor_storage_before = uvm_storage_value_to_cbor(StorageDataType::create_lua_storage_from_storage_data(scope.L(), con_chg_iter->second.before));
				auto cbor_storage_after = uvm_storage_value_to_cbor(StorageDataType::create_lua_storage_from_storage_data(scope.L(), con_chg_iter->second.after));
				auto change_cbor_diff = *(differ.diff(cbor_storage_before, cbor_storage_after));
				auto cbor_diff_value = std::make_shared<cbor::CborObject>(change_cbor_diff.value());
				const auto& cbor_diff_chars = cbor_diff::cbor_encode(cbor_diff_value);
				storage_change.storage_diff.storage_data = cbor_diff_chars;
				storage_change.after = storage_after;
				contract_storage_change[contract_name] = storage_change;
				nested_changes[contract_name] = cbor_diff_value;
			}
			// count gas by changes size
			size_t changes_size = 0;
			auto nested_changes_cbor = cbor::CborObject::create_map(nested_changes);
			const auto& changes_parsed_to_array = uvm::util::nested_cbor_object_to_array(nested_changes_cbor.get());
			changes_size = cbor_diff::cbor_encode(changes_parsed_to_array).size();
			// printf("changes size: %d bytes\n", changes_size);
			storage_gas += changes_size * 10; // 1 byte storage cost 10 gas

			if (storage_gas < 0) {
				return -1;
			}
		}
		dlog("storage gas: " + std::to_string(storage_gas));
		return storage_gas;
	}

	int64_t contract_invoke_result::count_event_gas() const {
		int64_t total_event_gas = 0;
		for (const auto& event_change : events) {
			auto event_size = event_change.event_name.size() + event_change.event_arg.size();
			auto event_gas = event_size * 2; // 1 byte event cost 2 gas
			total_event_gas += event_gas;
			if (total_event_gas < 0) {
				return -1;
			}
		}
		dlog("event gas: " + std::to_string(total_event_gas));
		return total_event_gas;
	}

        string contract_invoke_result::ordered_digest() const
		{
			JsonArray result_array;
			result_array.push_back(api_result);

			JsonArray storage_changes_json_array;
			for (auto it = storage_changes.cbegin(); it != storage_changes.cend(); it++)
			{
				for (auto it2 = it->second.cbegin(); it2 != it->second.cend(); it2++)
				{
					JsonArray item;
					item.push_back(it->first);
					item.push_back(it2->first);
					item.push_back(fc::sha256::hash(it2->second.after.storage_data.data(), it2->second.after.storage_data.size()).str());
					item.push_back(fc::sha256::hash(it2->second.storage_diff.storage_data.data(), it2->second.storage_diff.storage_data.size()).str());
					storage_changes_json_array.push_back(item);
				}
			}
			result_array.push_back(storage_changes_json_array);

			JsonArray contract_withdraw_array;
			for (auto it = contract_withdraw.cbegin(); it != contract_withdraw.cend(); it++)
			{
				JsonArray item;
				item.push_back(it->first.first.operator fc::string());
				item.push_back(uint64_t(it->first.second));
				item.push_back(it->second.value);
				contract_withdraw_array.push_back(item);
			}
			result_array.push_back(contract_withdraw_array);

			JsonArray contract_balances_array;
			for (auto it = contract_balances.cbegin(); it != contract_balances.cend(); it++)
			{
				JsonArray item;
				item.push_back(it->first.first.operator fc::string());
				item.push_back(uint64_t(it->first.second));
				item.push_back(it->second.value);
				contract_balances_array.push_back(item);
			}
			result_array.push_back(contract_balances_array);

			JsonArray deposit_to_address_array;
			for (auto it = deposit_to_address.cbegin(); it != deposit_to_address.cend(); it++)
			{
				JsonArray item;
				item.push_back(it->first.first.address_to_string());
				item.push_back(uint64_t(it->first.second));
				item.push_back(it->second.value);
				deposit_to_address_array.push_back(item);
			}
			result_array.push_back(deposit_to_address_array);

			JsonArray deposit_contract_array;
			for (auto it = deposit_contract.cbegin(); it != deposit_contract.cend(); it++)
			{
				JsonArray item;
				item.push_back(it->first.first.operator fc::string());
				item.push_back(uint64_t(it->first.second));
				item.push_back(it->second.value);
				deposit_contract_array.push_back(item);
			}
			result_array.push_back(deposit_contract_array);

            JsonArray transfer_fees_array;
            for (auto it= transfer_fees.begin(); it != transfer_fees.end(); it++)
            {
                JsonArray item;
                item.push_back(fc::json::to_string(it->first));
                item.push_back(it->second.value);
                transfer_fees_array.push_back(item);
            }
            result_array.push_back(transfer_fees_array);


			auto array_json_str = json_dumps(result_array);
			return fc::sha256::hash(array_json_str.c_str(), array_json_str.size()).str();
		}

		void            contract_register_operation::validate()const
		{

			FC_ASSERT(contract_id.version == addressVersion::CONTRACT);
			FC_ASSERT(init_cost > 0 && init_cost <= BLOCKLINK_MAX_GAS_LIMIT);
			// FC_ASSERT(fee.amount == 0 & fee.asset_id == asset_id_type(0));
			FC_ASSERT(gas_price >= BLOCKLINK_MIN_GAS_PRICE);
			FC_ASSERT(contract_id == calculate_contract_id());
            FC_ASSERT(owner_addr != address());
            FC_ASSERT(address(owner_pubkey) == owner_addr);
            FC_ASSERT(contract_id != address());
            FC_ASSERT((contract_code != Code()) || (inherit_from != address()));
		}
		share_type      contract_register_operation::calculate_fee(const fee_parameters_type& schedule)const
		{
			// base fee
            share_type core_fee_required = 0;
			// bytes size fee
            core_fee_required += count_contract_register_fee(contract_code);//calculate_data_fee(fc::raw::pack_size(contract_code), schedule.price_per_kbyte);
            core_fee_required += count_gas_fee(gas_price, init_cost);
			return core_fee_required+schedule.fee;
		}

		address contract_register_operation::get_first_contract_id()
		{
			address id;
			fc::sha512::encoder enc;
			Code co;
			time_point_sec regtm;
			std::pair<uvm::blockchain::Code, fc::time_point> info_to_digest(co,regtm );
			fc::raw::pack(enc, info_to_digest);
			id.addr = fc::ripemd160::hash(enc.result());
			id.version = addressVersion::CONTRACT;
			return id;
		}
        address contract_register_operation::calculate_contract_id() const
		{ 
            address id;
			fc::sha512::encoder enc;
            FC_ASSERT((contract_code != Code()) || (inherit_from != address()));
            if (contract_code != Code())
            {
                std::pair<uvm::blockchain::Code, fc::time_point> info_to_digest(contract_code, register_time);
                fc::raw::pack(enc, info_to_digest);
            }
            else if(inherit_from!= address())
            {
                std::pair<address, fc::time_point> info_to_digest(inherit_from, register_time);
                fc::raw::pack(enc, info_to_digest);
            }

			id.addr = fc::ripemd160::hash(enc.result());
			id.version = addressVersion::CONTRACT;
			return id;
		}
        
		void            contract_invoke_operation::validate()const
		{
			if (!offline)
			{
				FC_ASSERT(contract_id.version == addressVersion::CONTRACT);
                FC_ASSERT(caller_addr != address());
                FC_ASSERT(address(caller_pubkey) == caller_addr);
                FC_ASSERT(contract_id != address());
				//FC_ASSERT(contract_api_check(*this));
				FC_ASSERT(invoke_cost > 0 && invoke_cost <= BLOCKLINK_MAX_GAS_LIMIT);
				// FC_ASSERT(fee.amount == 0 & fee.asset_id == asset_id_type(0));
				FC_ASSERT(gas_price >= BLOCKLINK_MIN_GAS_PRICE);
			}
		}
		share_type contract_invoke_operation::calculate_fee(const fee_parameters_type& schedule)const
		{
			// base fee
			share_type core_fee_required =  count_gas_fee(gas_price, invoke_cost);
			return core_fee_required+schedule.fee;
		}

        bool contract_upgrade_operation::contract_name_check(const string & contract_name)
        {
            FC_ASSERT(contract_name.length() >= 2 && contract_name.length() <= 30);
            int pos = -1;
            for(auto c: contract_name)
            {
                ++pos;
                if ((c >= 'a'&&c <= 'z') || (c >= 'A'&&c <= 'Z'))
                    continue;
                if((c>='0'&&c<='9')&&(pos!=0))
                    continue;
                FC_CAPTURE_AND_THROW(blockchain::contract_engine::invalid_contract_name,(contract_name));
            }
            return true;
        }

        void          contract_upgrade_operation::validate()const
		{
			FC_ASSERT(invoke_cost > 0 && invoke_cost <= BLOCKLINK_MAX_GAS_LIMIT);
			// FC_ASSERT(fee.amount == 0 & fee.asset_id == asset_id_type(0));
			FC_ASSERT(gas_price >= BLOCKLINK_MIN_GAS_PRICE);
			// TODO: validate contract_name rule, eg. only letters and digits, underscoreed allowed; it can't start with digit. etc.
            contract_name_check(contract_name);
		    FC_ASSERT(contract_desc.length() <= 200);
            FC_ASSERT(caller_addr != address());
            FC_ASSERT(address(caller_pubkey) == caller_addr);
            FC_ASSERT(contract_id != address());
			FC_ASSERT(contract_id.version == addressVersion::CONTRACT);
		}

		share_type contract_upgrade_operation::calculate_fee(const fee_parameters_type& schedule)const
		{
			// base fee
			share_type core_fee_required = count_gas_fee(gas_price, invoke_cost);
			return core_fee_required + schedule.fee;
		}

        void            transfer_contract_operation::validate()const
        {

			FC_ASSERT(contract_id.version == addressVersion::CONTRACT);
            FC_ASSERT(invoke_cost > 0 && invoke_cost <= BLOCKLINK_MAX_GAS_LIMIT);
            // FC_ASSERT(fee.amount == 0 & fee.asset_id == asset_id_type(0));
            FC_ASSERT(gas_price >= BLOCKLINK_MIN_GAS_PRICE);
            FC_ASSERT(caller_addr!=address());
            FC_ASSERT(address(caller_pubkey) == caller_addr);
            FC_ASSERT(contract_id != address());
            FC_ASSERT(amount.amount>0);
        }
        share_type transfer_contract_operation::calculate_fee(const fee_parameters_type& schedule)const
        {
            share_type core_fee_required = count_gas_fee(gas_price, invoke_cost);
            return core_fee_required+schedule.fee;
        }

		int ContractHelper::common_fread_int(FILE* fp, int* dst_int)
		{
			int ret;
			unsigned char uc4, uc3, uc2, uc1;

			ret = (int)fread(&uc4, sizeof(unsigned char), 1, fp);
			if (ret != 1)
				return ret;
			ret = (int)fread(&uc3, sizeof(unsigned char), 1, fp);
			if (ret != 1)
				return ret;
			ret = (int)fread(&uc2, sizeof(unsigned char), 1, fp);
			if (ret != 1)
				return ret;
			ret = (int)fread(&uc1, sizeof(unsigned char), 1, fp);
			if (ret != 1)
				return ret;

			*dst_int = (uc4 << 24) + (uc3 << 16) + (uc2 << 8) + uc1;

			return 1;
		}

		int ContractHelper::common_fwrite_int(FILE* fp, const int* src_int)
		{
			int ret;
			unsigned char uc4, uc3, uc2, uc1;
			uc4 = ((*src_int) & 0xFF000000) >> 24;
			uc3 = ((*src_int) & 0x00FF0000) >> 16;
			uc2 = ((*src_int) & 0x0000FF00) >> 8;
			uc1 = (*src_int) & 0x000000FF;

			ret = (int)fwrite(&uc4, sizeof(unsigned char), 1, fp);
			if (ret != 1)
				return ret;
			ret = (int)fwrite(&uc3, sizeof(unsigned char), 1, fp);
			if (ret != 1)
				return ret;
			ret = (int)fwrite(&uc2, sizeof(unsigned char), 1, fp);
			if (ret != 1)
				return ret;
			ret = (int)fwrite(&uc1, sizeof(unsigned char), 1, fp);
			if (ret != 1)
				return ret;

			return 1;
		}

		int ContractHelper::common_fwrite_stream(FILE* fp, const void* src_stream, int len)
		{
			return (int)fwrite(src_stream, len, 1, fp);
		}

		int ContractHelper::common_fread_octets(FILE* fp, void* dst_stream, int len)
		{
			return (int)fread(dst_stream, len, 1, fp);
		}


#define PRINTABLE_CHAR(chr) \
if (chr >= 0 && chr <= 9)  \
    chr = chr + '0'; \
else \
    chr = chr + 'a' - 10; 

		std::string ContractHelper::to_printable_hex(unsigned char chr)
		{
			unsigned char high = chr >> 4;
			unsigned char low = chr & 0x0F;
			char tmp[16];

			PRINTABLE_CHAR(high);
			PRINTABLE_CHAR(low);

			snprintf(tmp, sizeof(tmp), "%c%c", high, low);
			return string(tmp);
		}

		int ContractHelper::save_code_to_file(const string& name, UvmModuleByteStream *stream, char* err_msg)
		{
			boost::uuids::detail::sha1 sha;
			unsigned int digest[5];

			UvmModuleByteStream* p_new_stream = new UvmModuleByteStream();
			if (NULL == p_new_stream)
			{
				strcpy(err_msg, "malloc UvmModuleByteStream fail");
				return -1;
			}
			p_new_stream->is_bytes = stream->is_bytes;
			p_new_stream->buff = stream->buff;
			for (int i = 0; i < stream->contract_apis.size(); ++i)
			{
				int new_flag = 1;
				for (int j = 0; j < stream->offline_apis.size(); ++j)
				{
					if (stream->contract_apis[i] == stream->offline_apis[j])
					{
						new_flag = 0;
						continue;
					}
				}

				if (new_flag)
				{
					p_new_stream->contract_apis.push_back(stream->contract_apis[i]);
				}
			}
			p_new_stream->offline_apis = stream->offline_apis;
			p_new_stream->contract_emit_events = stream->contract_emit_events;
			p_new_stream->contract_storage_properties = stream->contract_storage_properties;

			p_new_stream->contract_id = stream->contract_id;
			p_new_stream->contract_name = stream->contract_name;
			p_new_stream->contract_level = stream->contract_level;
			p_new_stream->contract_state = stream->contract_state;

			FILE *f = fopen(name.c_str(), "wb");
			if (NULL == f)
			{
				delete (p_new_stream);
				strcpy(err_msg, strerror(errno));
				return -1;
			}

			sha.process_bytes(p_new_stream->buff.data(), p_new_stream->buff.size());
			sha.get_digest(digest);
			for (int i = 0; i < 5; ++i)
				common_fwrite_int(f, (int*)&digest[i]);

			int p_new_stream_buf_size = (int)p_new_stream->buff.size();
			common_fwrite_int(f, &p_new_stream_buf_size);
			p_new_stream->buff.resize(p_new_stream_buf_size);
			common_fwrite_stream(f, p_new_stream->buff.data(), p_new_stream->buff.size());

			int contract_apis_count = (int)p_new_stream->contract_apis.size();
			common_fwrite_int(f, &contract_apis_count);
			for (int i = 0; i < contract_apis_count; ++i)
			{
				int api_len = p_new_stream->contract_apis[i].length();
				common_fwrite_int(f, &api_len);
				common_fwrite_stream(f, p_new_stream->contract_apis[i].c_str(), api_len);
			}

			int offline_apis_count = (int)p_new_stream->offline_apis.size();
			common_fwrite_int(f, &offline_apis_count);
			for (int i = 0; i < offline_apis_count; ++i)
			{
				int offline_api_len = p_new_stream->offline_apis[i].length();
				common_fwrite_int(f, &offline_api_len);
				common_fwrite_stream(f, p_new_stream->offline_apis[i].c_str(), offline_api_len);
			}

			int contract_emit_events_count = p_new_stream->contract_emit_events.size();
			common_fwrite_int(f, &contract_emit_events_count);
			for (int i = 0; i < contract_emit_events_count; ++i)
			{
				int event_len = p_new_stream->contract_emit_events[i].length();
				common_fwrite_int(f, &event_len);
				common_fwrite_stream(f, p_new_stream->contract_emit_events[i].c_str(), event_len);
			}

			int contract_storage_properties_count = p_new_stream->contract_storage_properties.size();
			common_fwrite_int(f, &contract_storage_properties_count);
			for (const auto& storage_info : p_new_stream->contract_storage_properties)
			{
				int storage_len = storage_info.first.length();
				common_fwrite_int(f, &storage_len);
				common_fwrite_stream(f, storage_info.first.c_str(), storage_len);
				int storage_type = storage_info.second;
				common_fwrite_int(f, &storage_type);
			}

			fclose(f);
			delete (p_new_stream);
			return 0;
		}


#define INIT_API_FROM_FILE(dst_set, except_1, except_2, except_3)\
{\
read_count = common_fread_int(f, &api_count); \
if (read_count != 1)\
{\
fclose(f); \
throw except_1(); \
}\
for (int i = 0; i < api_count; i++)\
{\
int api_len = 0; \
read_count = common_fread_int(f, &api_len); \
if (read_count != 1)\
{\
fclose(f); \
throw except_2(); \
}\
api_buf = (char*)malloc(api_len + 1); \
if (api_buf == NULL) \
{ \
fclose(f); \
FC_ASSERT(api_buf == NULL, "malloc fail!"); \
}\
read_count = common_fread_octets(f, api_buf, api_len); \
if (read_count != 1)\
{\
fclose(f); \
free(api_buf); \
throw except_3(); \
}\
api_buf[api_len] = '\0'; \
dst_set.insert(std::string(api_buf)); \
free(api_buf); \
}\
}

#define INIT_STORAGE_FROM_FILE(dst_map, except_1, except_2, except_3, except_4)\
{\
read_count = common_fread_int(f, &storage_count); \
if (read_count != 1)\
{\
fclose(f); \
throw except_1(); \
}\
for (int i = 0; i < storage_count; i++)\
{\
int storage_name_len = 0; \
read_count = common_fread_int(f, &storage_name_len); \
if (read_count != 1)\
{\
fclose(f); \
throw except_2(); \
}\
storage_buf = (char*)malloc(storage_name_len + 1); \
if (storage_buf == NULL) \
{ \
fclose(f); \
FC_ASSERT(storage_buf == NULL, "malloc fail!"); \
}\
read_count = common_fread_octets(f, storage_buf, storage_name_len); \
if (read_count != 1)\
{\
fclose(f); \
free(storage_buf); \
throw except_3(); \
}\
storage_buf[storage_name_len] = '\0'; \
read_count = common_fread_int(f, (int*)&storage_type); \
if (read_count != 1)\
{\
fclose(f); \
free(storage_buf); \
throw except_4(); \
}\
dst_map.insert(std::make_pair(std::string(storage_buf), storage_type)); \
free(storage_buf); \
}\
}

		static fc::mutex file_mutex;
		uvm::blockchain::Code ContractHelper::load_contract_from_hex(const string& hex)
		{

			fc::scoped_lock<fc::mutex> lock(file_mutex);
			char s[101] = {0};

			char buf[1024] = { 0 };
			int i;
			srand(time(NULL));
			std::vector<char> bytes;
			bytes.resize(hex.length()/2);
			fc::from_hex(hex, bytes.data(), bytes.size());
			for (i = 0; i < 10; i++)
			{
				do
				{
					s[i] = 'a'+rand() % 26;
				} while (!isprint(s[i]));
			}
			string file_string = s;
			ofstream tmp(file_string,ios::trunc|ios::binary);
			tmp.write(bytes.data(),bytes.size());
			tmp.close();
			Code code;
			FILE* f = fopen(file_string.c_str(), "rb");

			if (f==NULL)
				FC_THROW_EXCEPTION(fc::file_not_found_exception, "Script file not found!");
			fseek(f, 0, SEEK_END);
			int fsize = ftell(f);
			fseek(f, 0, SEEK_SET);

			unsigned int digest[5];
			int read_count = 0;
			for (int i = 0; i < 5; ++i)
			{
				read_count = common_fread_int(f, (int*)&digest[i]);
				if (read_count != 1)
				{
					fclose(f);
					FC_THROW_EXCEPTION(blockchain::contract_engine::read_verify_code_fail, "Read verify code fail!");
				}
			}

			int len = 0;
			read_count = common_fread_int(f, &len);
			if (read_count != 1 || len < 0 || (len >= (fsize - ftell(f))))
			{
				fclose(f);
				FC_THROW_EXCEPTION(blockchain::contract_engine::read_bytescode_len_fail, "Read bytescode len fail!");
			}

			code.code.resize(len);
			read_count = common_fread_octets(f, code.code.data(), len);
			if (read_count != 1)
			{
				fclose(f);
				FC_THROW_EXCEPTION(blockchain::contract_engine::read_bytescode_fail, "Read bytescode fail!");
			}

			boost::uuids::detail::sha1 sha;
			unsigned int check_digest[5];
			sha.process_bytes(code.code.data(), code.code.size());
			sha.get_digest(check_digest);
			if (memcmp((void*)digest, (void*)check_digest, sizeof(unsigned int) * 5))
			{
				fclose(f);
				FC_THROW_EXCEPTION(blockchain::contract_engine::verify_bytescode_sha1_fail, "Verify bytescode SHA1 fail!");
			}

			for (int i = 0; i < 5; ++i)
			{
				unsigned char chr1 = (check_digest[i] & 0xFF000000) >> 24;
				unsigned char chr2 = (check_digest[i] & 0x00FF0000) >> 16;
				unsigned char chr3 = (check_digest[i] & 0x0000FF00) >> 8;
				unsigned char chr4 = (check_digest[i] & 0x000000FF);

				code.code_hash = code.code_hash + to_printable_hex(chr1) + to_printable_hex(chr2) +
					to_printable_hex(chr3) + to_printable_hex(chr4);
			}

			int api_count = 0;
			char* api_buf = nullptr;

			INIT_API_FROM_FILE(code.abi, blockchain::contract_engine::read_api_count_fail, blockchain::contract_engine::read_api_len_fail, blockchain::contract_engine::read_api_fail);
			INIT_API_FROM_FILE(code.offline_abi, blockchain::contract_engine::read_offline_api_count_fail, blockchain::contract_engine::read_offline_api_len_fail, blockchain::contract_engine::read_offline_api_fail);
			INIT_API_FROM_FILE(code.events, blockchain::contract_engine::read_events_count_fail, blockchain::contract_engine::read_events_len_fail, blockchain::contract_engine::read_events_fail);

			int storage_count = 0;
			char* storage_buf = nullptr;
			StorageValueTypes storage_type;

			INIT_STORAGE_FROM_FILE(code.storage_properties, blockchain::contract_engine::read_storage_count_fail, blockchain::contract_engine::read_storage_name_len_fail, blockchain::contract_engine::read_storage_name_fail, blockchain::contract_engine::read_storage_type_fail);

			fclose(f);
			boost::filesystem::remove(file_string);
			return code;
		}
		uvm::blockchain::Code ContractHelper::load_contract_from_file(const fc::path &path)
		{
			if (!fc::exists(path))
				FC_THROW_EXCEPTION(fc::file_not_found_exception, "Script file not found!");
			Code code;
			string file = path.string();
			FILE* f = fopen(file.c_str(), "rb");
			fseek(f, 0, SEEK_END);
			int fsize = ftell(f);
			fseek(f, 0, SEEK_SET);

			unsigned int digest[5];
			int read_count = 0;
			for (int i = 0; i < 5; ++i)
			{
				read_count = common_fread_int(f, (int*)&digest[i]);
				if (read_count != 1)
				{
					fclose(f);
					FC_THROW_EXCEPTION(blockchain::contract_engine::read_verify_code_fail, "Read verify code fail!");
				}
			}

			int len = 0;
			read_count = common_fread_int(f, &len);
			if (read_count != 1 || len < 0 || (len >= (fsize - ftell(f))))
			{
				fclose(f);
				FC_THROW_EXCEPTION(blockchain::contract_engine::read_bytescode_len_fail, "Read bytescode len fail!");
			}

			code.code.resize(len);
			read_count = common_fread_octets(f, code.code.data(), len);
			if (read_count != 1)
			{
				fclose(f);
				FC_THROW_EXCEPTION(blockchain::contract_engine::read_bytescode_fail, "Read bytescode fail!");
			}

			boost::uuids::detail::sha1 sha;
			unsigned int check_digest[5];
			sha.process_bytes(code.code.data(), code.code.size());
			sha.get_digest(check_digest);
			if (memcmp((void*)digest, (void*)check_digest, sizeof(unsigned int) * 5))
			{
				fclose(f);
				FC_THROW_EXCEPTION(blockchain::contract_engine::verify_bytescode_sha1_fail, "Verify bytescode SHA1 fail!");
			}

			for (int i = 0; i < 5; ++i)
			{
				unsigned char chr1 = (check_digest[i] & 0xFF000000) >> 24;
				unsigned char chr2 = (check_digest[i] & 0x00FF0000) >> 16;
				unsigned char chr3 = (check_digest[i] & 0x0000FF00) >> 8;
				unsigned char chr4 = (check_digest[i] & 0x000000FF);

				code.code_hash = code.code_hash + to_printable_hex(chr1) + to_printable_hex(chr2) +
					to_printable_hex(chr3) + to_printable_hex(chr4);
			}

			int api_count = 0;
			char* api_buf = nullptr;

			INIT_API_FROM_FILE(code.abi, blockchain::contract_engine::read_api_count_fail, blockchain::contract_engine::read_api_len_fail, blockchain::contract_engine::read_api_fail);
			INIT_API_FROM_FILE(code.offline_abi, blockchain::contract_engine::read_offline_api_count_fail, blockchain::contract_engine::read_offline_api_len_fail, blockchain::contract_engine::read_offline_api_fail);
			INIT_API_FROM_FILE(code.events, blockchain::contract_engine::read_events_count_fail, blockchain::contract_engine::read_events_len_fail, blockchain::contract_engine::read_events_fail);

			int storage_count = 0;
			char* storage_buf = nullptr;
			StorageValueTypes storage_type;

			INIT_STORAGE_FROM_FILE(code.storage_properties, blockchain::contract_engine::read_storage_count_fail, blockchain::contract_engine::read_storage_name_len_fail, blockchain::contract_engine::read_storage_name_fail, blockchain::contract_engine::read_storage_type_fail);

			fclose(f);

			return code;
		}

		void contract_transfer_fee_proposal_operation::validate() const
		{
			FC_ASSERT((fee_rate>=0)&&(fee_rate < CONTRACT_MAX_TRASACTION_FEE_RATE));
		}

		share_type contract_transfer_fee_proposal_operation::calculate_fee(const fee_parameters_type & k) const
		{
			return share_type(0);
		}

}
}
