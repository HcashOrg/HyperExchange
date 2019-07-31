#include "builder_api.hpp"
#include <graphene/chain/asset_object.hpp>
namespace graphene {
	namespace builder
	{
		transaction_builder_api::transaction_builder_api()
		{

		}

		transaction_builder_api::~transaction_builder_api()
		{

		}
		chain::signed_transaction transaction_builder_api::build_transfer_transaction(const chain::address & from, const chain::address & to, const string & amount, const asset_id_type & asset_id, const int precision, const string & fee_amount, const string & memo) const
		{
			asset_object ao,fao;
			fao.id = asset_id_type();
			fao.precision = GRAPHENE_BLOCKCHAIN_PRECISION;
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
			return tx;
		}

		graphene::chain::signed_transaction transaction_builder_api::sign_transaction_with_key(const signed_transaction& trx, const string&key) const
		{
			return signed_transaction();
		}
	}
}


