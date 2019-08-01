#include "builder_api.hpp"

#include <graphene/app/full_account.hpp>

#include <graphene/chain/protocol/types.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/confidential_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/referendum_object.hpp>
#include <graphene/chain/worker_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/lockbalance_object.hpp>
#include <graphene/chain/guard_lock_balance_object.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <graphene/chain/coldhot_transfer_object.hpp>
#include <graphene/market_history/market_history_plugin.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/contract_object.hpp>
#include <graphene/chain/pay_back_object.hpp>
using namespace graphene::chain;

		int main()
		{
			address::testnet_mode = true;
			auto api = make_shared<graphene::builder::transaction_builder_api>(graphene::builder::transaction_builder_api());
			auto _websocket_server = std::make_shared<fc::http::websocket_server>();
			_websocket_server->on_connection([&](const fc::http::websocket_connection_ptr& c) {
				auto wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);
				fc::api<graphene::builder::transaction_builder_api> ap(api);
				wsc->register_api(ap);
				c->set_session_data(wsc);
			});
			_websocket_server->listen(fc::ip::endpoint::from_string(ENDPOINT));
			_websocket_server->start_accept();
			while (1)
			{
				fc::usleep(fc::milliseconds(1000));
			}
		}
