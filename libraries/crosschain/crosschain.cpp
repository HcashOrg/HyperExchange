#include <mutex>
#include "graphene/crosschain/crosschain.hpp"
#include "graphene/crosschain/crosschain_impl.hpp"
#include "graphene/crosschain/crosschain_interface_emu.hpp"
#include <graphene/crosschain/crosschain_interface_btc.hpp>
#include <graphene/crosschain/crosschain_interface_ltc.hpp>
#include <graphene/crosschain/crosschain_interface_ub.hpp>
#include <graphene/crosschain/crosschain_interface_hc.hpp>
#include <graphene/crosschain/crosschain_interface_eth.hpp>
#include <graphene/crosschain/crosschain_interface_erc.hpp>
#include <graphene/crosschain/crosschain_interface_usdt.hpp>
namespace graphene {
	namespace crosschain {
		crosschain_manager::crosschain_manager()
		{
		}

		crosschain_manager::~crosschain_manager()
		{
		}
		abstract_crosschain_interface * crosschain_manager::get_crosschain_handle(const std::string &name)
		{
			//std::lock_guard<std::mutex> lgd(mutex);
			auto itr = crosschain_handles.find(name);
			if (itr != crosschain_handles.end())
			{
				return itr->second;
			}
			else
			{
				if (name == "EMU")
				{
					const auto &itr = crosschain_handles.insert(std::make_pair(name, new crosschain_interface_emu()));
					return itr.first->second;
				}
				else if (name == "BTC")
				{
					auto itr = crosschain_handles.insert(std::make_pair(name, new crosschain_interface_btc()));
					return itr.first->second;
				}
				else if (name == "USDT")
				{
					auto itr = crosschain_handles.insert(std::make_pair(name, new crosschain_interface_usdt()));
					return itr.first->second;
				}
				else if (name == "LTC")
				{
					auto itr = crosschain_handles.insert(std::make_pair(name, new crosschain_interface_ltc()));
					return itr.first->second;
				}
				else if (name == "UB")
				{
					auto itr = crosschain_handles.insert(std::make_pair(name, new crosschain_interface_ub()));
					return itr.first->second;
				}
				else if (name == "HC")
				{
					auto itr = crosschain_handles.insert(std::make_pair(name, new crosschain_interface_hc()));
					return itr.first->second;
				}
				else if (name == "ETH") {
					auto itr = crosschain_handles.insert(std::make_pair(name, new crosschain_interface_eth()));
					return itr.first->second;
				}
				else if (name.find("ERC") != name.npos){
					std::string chain_type;
					transform(name.begin(), name.end(), chain_type.begin(), ::tolower);
					auto itr = crosschain_handles.insert(std::make_pair(name, new crosschain_interface_erc(name)));
					return itr.first->second;
				}
			}
			return nullptr;
		}
		bool crosschain_manager::contain_crosschain_handles(const std::string& symbol)
		{
			return crosschain_handles.count(symbol)>=1;
		}
	}
}

void stub(void)
{
	return;
}
