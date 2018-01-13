#include <graphene/chain/contract.hpp>
#include <graphene/chain/contract_engine_builder.hpp>
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

		address contract_register_operation::calculate_contract_id() const
		{
			address id;
			fc::sha512::encoder enc;
			fc::raw::pack(enc, contract_code);
			id.addr = fc::ripemd160::hash(enc.result());
			return id;
		}

	}
}
