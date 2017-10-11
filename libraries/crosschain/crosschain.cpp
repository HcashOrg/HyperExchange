#include <mutex>
#include "crosschain.hpp"
#include "crosschain_impl.hpp"

namespace graphene {
	namespace crosschain {
		abstract_cross_chain_interface * crosschain_manager::get_cross_chain_handle(std::string &name)
		{
			std::lock_guard<std::mutex> lgd(mutex);
			auto &itr = cross_chain_handles.find(name);
			if (itr != cross_chain_handles.end())
			{
				return itr->second;
			}
			else
			{
				/*if (name == "DMY")
				{
					auto &itr = cross_chain_handles.insert(std::make_pair(name, ))
				}*/
				return nullptr;
			}
		}
	}
}

void stub(void)
{
	return;
}