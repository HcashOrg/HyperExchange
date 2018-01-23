#include <graphene/chain/native_contract.hpp>

namespace graphene {
	namespace chain {
		// TODO: more native contracts
		// TODO: balance and storage changes in native contracts
		// TODO: invoke native contract api

		demo_native_contract::~demo_native_contract() {

		}
		std::string demo_native_contract::contract_key() const
		{
			return demo_native_contract::native_contract_key();
		}
		address demo_native_contract::contract_address() const {
			return contract_id;
		}
		std::set<std::string> demo_native_contract::apis() const {
			return { "init", "hello" };
		}
		std::set<std::string> demo_native_contract::offline_apis() const {
			return {};
		}
		std::set<std::string> demo_native_contract::events() const {
			return {};
		}

		contract_invoke_result demo_native_contract::invoke(const std::string& api_name, const std::string& api_arg) {
			contract_invoke_result result;
			printf("demo native contract called\n");
			printf("api %s called with arg %s\n", api_name.c_str(), api_arg.c_str());
			return result;
		}

		bool native_contract_finder::has_native_contract_with_key(const std::string& key)
		{
			std::vector<std::string> native_contract_keys = {
				demo_native_contract::native_contract_key()
			};
			return std::find(native_contract_keys.begin(), native_contract_keys.end(), key) != native_contract_keys.end();
		}
		shared_ptr<abstract_native_contract> native_contract_finder::create_native_contract_by_key(const std::string& key, const address& contract_address)
		{
			if (key == demo_native_contract::native_contract_key())
			{
				return std::make_shared<demo_native_contract>(contract_address);
			}
			else {
				return nullptr;
			}
		}

		void            native_contract_register_operation::validate()const
		{
			FC_ASSERT(init_cost > 0 && init_cost <= BLOCKLINK_MAX_GAS_LIMIT);
			// FC_ASSERT(fee.amount == 0 & fee.asset_id == asset_id_type(0));
			FC_ASSERT(gas_price >= BLOCKLINK_MIN_GAS_PRICE);
			FC_ASSERT(contract_id == calculate_contract_id());
			FC_ASSERT(native_contract_finder::has_native_contract_with_key(native_contract_key));
		}
		share_type      native_contract_register_operation::calculate_fee(const fee_parameters_type& schedule)const
		{
			// base fee
			share_type core_fee_required = schedule.fee;
			core_fee_required += calculate_data_fee(100, schedule.price_per_kbyte); // FIXME: native contract base fee
			return core_fee_required;
		}
		address native_contract_register_operation::calculate_contract_id() const
		{
			address id;
			fc::sha512::encoder enc;
			std::pair<address, fc::time_point> info_to_digest(owner_addr, register_time);
			fc::raw::pack(enc, info_to_digest);
			id.addr = fc::ripemd160::hash(enc.result());
			return id;
		}
	}
}