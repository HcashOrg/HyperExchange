#include <boost/test/unit_test.hpp>
#include <fc/variant_object.hpp>
#include <iostream>
#include <graphene/crosschain/crosschain.hpp>
#include <graphene/crosschain/crosschain_impl.hpp>
/*
need a normal account
need a multisig account

*/

static struct
{
	graphene::crosschain::crosschain_manager manager;
	//_wallet;
	std::string normal_address;
	std::string multi_sig_address;
	
} plugin_data;
static void setup_env()
{
	
}

BOOST_AUTO_TEST_SUITE(plugin_test)

BOOST_AUTO_TEST_CASE(wallet_create_operation)
{
	//create wallet
	auto manager = graphene::crosschain::crosschain_manager::get_instance();
	auto hdl = manager.get_crosschain_handle(std::string("EMU"));
	hdl->create_wallet("test", "12345678");
	//open unlock lock close wallet

	//auto _wallet = create_wallet();
}


BOOST_AUTO_TEST_CASE(account_operation)
{
	//create normal account
	auto manager = graphene::crosschain::crosschain_manager::get_instance();
	auto hdl = manager.get_crosschain_handle(std::string("EMU"));
	hdl->create_normal_account("test_account");
}

BOOST_AUTO_TEST_CASE(transfer)
{
	auto manager = graphene::crosschain::crosschain_manager::get_instance();
	auto hdl = manager.get_crosschain_handle(std::string("EMU"));
	//transfer normal trx
	auto trx = hdl->transfer(std::string("test_account"), std::string("to_account"), std::string("1"), std::string("mBTC"), std::string(""), true);
	//check balance of account
	hdl->query_account_balance(std::string("to_account"));
}


BOOST_AUTO_TEST_CASE(create_multi_account)
{
	//create multi_account
	auto manager = graphene::crosschain::crosschain_manager::get_instance();
	auto hdl = manager.get_crosschain_handle(std::string("EMU"));
	std::vector<std::string> vec{"str1","str2"};


    auto addr = hdl->create_multi_sig_account(vec); //n/m 
	plugin_data.multi_sig_address = addr;

}

BOOST_AUTO_TEST_CASE(transfer_multi)
{
	auto manager = graphene::crosschain::crosschain_manager::get_instance();
	auto hdl = manager.get_crosschain_handle(std::string("EMU"));

	auto trx = hdl->create_multisig_transaction(std::string("fromaccount"),std::string("toaccount"),std::string("10"),std::string("mBTC"),std::string(""),true);
	//sign
	auto signature = hdl->sign_multisig_transaction(trx,std::string("sign_account"),true);
	
	std::vector<fc::variant_object> vec;
	vec.push_back(signature);
	//merge
	hdl->merge_multisig_transaction(trx,vec);

}

BOOST_AUTO_TEST_CASE(transfer_history)
{
	//get history of transactions
	auto manager = graphene::crosschain::crosschain_manager::get_instance();
	auto hdl = manager.get_crosschain_handle(std::string("EMU"));
	hdl->transaction_history(std::string("test_account"), 0, 10);
}

BOOST_AUTO_TEST_SUITE_END()
