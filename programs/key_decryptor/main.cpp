#include <iostream>
#include <fstream>
#include <vector>
#include <boost/filesystem/operations.hpp>
#include <graphene/wallet/wallet.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/io/json.hpp>
using namespace std;

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		cout << "command error!\n cmd  file  decrypt_key\n";
		return 0;
	}
	if (argc == 3) {
		string out_key_file = argv[1];
		string encrypt_key = argv[2];
		map<string, graphene::wallet::crosschain_prkeys> keys;
		std::ifstream in(out_key_file, std::ios::in | std::ios::binary);
		if (in.is_open())
		{

			std::vector<char> key_file_data((std::istreambuf_iterator<char>(in)),
				(std::istreambuf_iterator<char>()));
			in.close();
			if (key_file_data.size() > 0)
			{
				const auto plain_text = fc::aes_decrypt(fc::sha512(encrypt_key.c_str(), encrypt_key.length()), key_file_data);
				keys = fc::raw::unpack<map<string, graphene::wallet::crosschain_prkeys>>(plain_text);
				cout << fc::json::to_pretty_string(keys) << std::endl;
			}
			else
			{
				cout << "Key file size zero\n";
			}
		}
		else
		{

			cout << "Key file not exsited\n";
			return 0;
		}
	}
	if (argc == 4) {
		string out_key_file = argv[1];
		string origin_string = argv[2];
		string encrypt_key = argv[3];
		map<string, graphene::wallet::crosschain_prkeys> keys;
		std::ofstream out(out_key_file, std::ios::out | std::ios::binary);
		if (out.is_open())
		{
			try {

				auto var = fc::json::from_string(origin_string);
				map<string, graphene::wallet::crosschain_prkeys> keys;
				fc::from_variant(var, keys);
				auto raw_data = fc::raw::pack<map<string, graphene::wallet::crosschain_prkeys>>(keys);
				const auto plain_text = fc::aes_encrypt(fc::sha512(encrypt_key.c_str(), encrypt_key.length()), raw_data);
				//keys = fc::raw::unpack<map<string, graphene::wallet::crosschain_prkeys>>(plain_text);
				out.write(plain_text.data(), plain_text.size());

				out.close();
			}
			catch (fc::exception ex) {
				std::cout << ex.to_detail_string() << std::endl;
			}

		}
		else
		{

			cout << "Key file not exsited\n";
			return 0;
		}
	}

}