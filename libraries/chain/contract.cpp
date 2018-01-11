#include <graphene/chain/contract.hpp>
#include <graphene/chain/contract_engine_builder.hpp>

#include <fc/array.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <boost/uuid/sha1.hpp>
#include <exception>

namespace graphene {
	namespace chain {


		void            contract_register_operation::validate()const
		{
			FC_ASSERT(fee.amount > 0);
			FC_ASSERT(init_cost > 0);
			FC_ASSERT(gas_price > 0);
		}
		share_type      contract_register_operation::calculate_fee(const fee_parameters_type& schedule)const
		{
			// base fee
			share_type core_fee_required = schedule.fee; // FIXME: contract base fee
			// bytes size fee
			core_fee_required += calculate_data_fee(fc::raw::pack_size(contract_code), schedule.price_per_kbyte);
			return core_fee_required;
		}

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

	}
}
