
#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>
#include <fc/network/http/websocket.hpp>
#include <graphene/chain/protocol/transaction.hpp>
#include <string>
#include <fc/api.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/optional.hpp>
#define ENDPOINT "127.0.0.1:8099"
namespace graphene
{
	using namespace chain;
	namespace builder
	{
		class transaction_builder_api
		{
		public:
			transaction_builder_api();
			~transaction_builder_api();
			
			chain::signed_transaction build_transfer_transaction(const chain::address& from, const chain::address& to, const string& amount,const asset_id_type& asset_id, const int precision, const string & fee_amount, const string&memo)const;
			chain::signed_transaction sign_transaction_with_key(const signed_transaction& trx, const string&key) const;
			int test() { return 0; }

		};
	}
}

FC_API(graphene::builder::transaction_builder_api,(build_transfer_transaction)(sign_transaction_with_key)(test))