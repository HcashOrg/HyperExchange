#include "builder_api.hpp"
#include <graphene/chain/asset_object.hpp>
#include <graphene/utilities/key_conversion.hpp>
namespace graphene {
	namespace builder
	{
		transaction_builder_api::transaction_builder_api()
		{

		}

		transaction_builder_api::~transaction_builder_api()
		{

		}
		chain::signed_transaction transaction_builder_api::build_transfer_transaction(const chain::address & from, const chain::address & to, const string & amount, const asset_id_type & asset_id, const int precision, const string & fee_amount, const string & memo,const block_id_type& ref_blk) const
		{
			asset_object ao,fao;
			fao.id = asset_id_type();
			fao.precision = GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS;
			asset tfee = fao.amount_from_string(fee_amount);
			ao.id = asset_id;
			ao.precision = precision;
			asset transfer_amount = ao.amount_from_string(amount);
			transfer_operation xfer_op;
			xfer_op.from_addr = from;
			xfer_op.to_addr = to;
			FC_ASSERT(xfer_op.to_addr.version != addressVersion::CONTRACT, "address should not be a contract address.");
			xfer_op.amount = transfer_amount;
			if (memo.size())
			{
				xfer_op.memo = memo_data();
				xfer_op.memo->from = public_key_type();
				xfer_op.memo->to = public_key_type();
				xfer_op.memo->set_message(private_key_type(),
					public_key_type(), memo);

			}
			xfer_op.fee = tfee;
			signed_transaction tx;
			tx.operations.push_back(xfer_op);
			tx.set_expiration(fc::time_point::now()+fc::seconds(3600));
			tx.set_reference_block(ref_blk);
			return tx;
		}

		chain::signed_transaction transaction_builder_api::sign_transaction_with_key(const signed_transaction& trx, const string&key, const string& chain_id) const
		{
			signed_transaction new_tx = trx;
			fc::optional<fc::ecc::private_key> optional_private_key = utilities::wif_to_key(key);
			if (!optional_private_key)
				FC_THROW("Invalid private key");
			
			new_tx.sign(*optional_private_key, chain_id_type(chain_id));
			return new_tx;
		}
	}
}


 