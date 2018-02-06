/**
* Author: wengqiang (email: wens.wq@gmail.com  site: qiangweng.site)
*
* Copyright © 2015--2018 . All rights reserved.
*
* File: main.cpp
* Date: 2018-01-11
*/

#include <graphene/privatekey_management/private_key.hpp>
#include <graphene/privatekey_management/database_privatekey.hpp>


#include <string> 
#include <vector> 
#include <iostream> 


int main(int argc, char** argv)
{
	using namespace graphene::privatekey_management;
	
	// test private key generation
// 	btc_privatekey btc_priv;
// 	auto btc_wif_key = btc_priv.get_wif_key(btc_priv.get_private_key());
// 	printf("btc wif key: %s\n", btc_wif_key.c_str());
// 	auto btc_addr = btc_priv.get_address(btc_priv.get_private_key());
// 	printf("btc address: %s\n", btc_addr.c_str());
// 	auto import_btc_priv_key = btc_priv.import_private_key(btc_wif_key);
// 	btc_wif_key = btc_priv.get_wif_key(*import_btc_priv_key);
// 	printf("imported btc wif key: %s\n", btc_wif_key.c_str());
// 
// 
// 	ltc_privatekey ltc_priv;
// 	auto ltc_wif_key = ltc_priv.get_wif_key(ltc_priv.get_private_key());
// 	printf("ltc wif key: %s\n", ltc_wif_key.c_str());
// 	auto ltc_addr = ltc_priv.get_address(ltc_priv.get_private_key());
// 	printf("ltc address: %s\n", ltc_addr.c_str());
// 	auto import_ltc_priv_key = ltc_priv.import_private_key(ltc_wif_key);
// 	ltc_wif_key = ltc_priv.get_wif_key(*import_ltc_priv_key);
// 	printf("imported ltc wif key: %s\n", ltc_wif_key.c_str());



	database_privatekey db_priv;
	db_priv.open("C:\\Users\\wensw\\Desktop\\blocklink\\data_dir");
	std::string password = "123456";
	auto checksum = fc::sha512::hash(password.c_str(), password.size());

	for (auto i = 5; i < 10; i++)
	{
		printf("current index: %d\n", i + 1);

		btc_privatekey btc_priv;
		auto btc_wif_key = btc_priv.get_wif_key(btc_priv.get_private_key());
		printf("btc wif key: %s\n", btc_wif_key.c_str());
		auto btc_addr = btc_priv.get_address(btc_priv.get_private_key());
		printf("btc address: %s\n", btc_addr.c_str());
		auto import_btc_priv_key = btc_priv.import_private_key(btc_wif_key);
		btc_wif_key = btc_priv.get_wif_key(*import_btc_priv_key);
		printf("imported btc wif key: %s\n", btc_wif_key.c_str());



		crosschain_privatekey_data data;
		data.id = i+1;
		data.addr = btc_priv.get_address(btc_priv.get_private_key());
		data.wif_key = btc_priv.get_wif_key(btc_priv.get_private_key());

	

		db_priv.store(data, checksum);
	}	

	printf("\n");
	auto result = db_priv.fetch_by_id(3, checksum);
	printf("%s\n", result->wif_key.c_str());

	auto max_id = db_priv.fetch_current_max_id();
	printf("%d\n", max_id);

	


// 
// 	std::string priv_hex = "fabe2beb53866e0f9d3568577810034b26ad965bc57f0a833305442a1272aba9";
// 
// 	std::string script = "\"dup hash160 [b5843e180a4360bdbf4bcd9cdd37c311bb9fcb64] equalverify checksig\"";
// 	std::string raw_trx = "02000000019463a8d3eb08b33ea75526510a2f88b63deb599021c92f1d5f78dce888262ab10000000000ffffffff01605af405000000001976a914f04d7bc2c9c2ce2ccefd9ebcd24c60a73caa1df588ac00000000";
// 	graphene::privatekey_management::btc_sign_trx(script, raw_trx, priv_hex);


	getchar();
	return 0;
}



