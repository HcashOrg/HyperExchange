#pragma once
#include <graphene/crosschain/crosschain_impl.hpp>



namespace graphene {
	namespace app {
		class crosschain_demo_plugin :abstract_cross_chain_plugin
		{
			virtual bool create_wallet(string wallet_name, string wallet_passprase) override;
			virtual bool unlock_wallet(string wallet_name, string wallet_passprase, uint32_t duration) override;
			virtual void close_wallet() override;
			virtual vector<string>  wallet_list() override;
			virtual std::string create_normal_account(string account_name) override;
			virtual std::string create_multi_sig_account(std::string account_name, std::vector<std::string> addresses)override;







		};






	}
}