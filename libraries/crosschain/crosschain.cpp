#include <mutex>
#include "graphene/crosschain/crosschain.hpp"
#include "graphene/crosschain/crosschain_impl.hpp"
#include "graphene/crosschain/crosschain_interface_emu.hpp"

namespace graphene {
	namespace crosschain {
		abstract_crosschain_interface * crosschain_manager::get_crosschain_handle(std::string &name)
		{
			std::lock_guard<std::mutex> lgd(mutex);
			auto &itr = crosschain_handles.find(name);
			if (itr != crosschain_handles.end())
			{
				return itr->second;
			}
			else
			{
				if (name == "EMU")
				{
					auto &itr = crosschain_handles.insert(std::make_pair(name, new crosschain_interface_emu()));
					return itr.first->second;
				}
				else
				{
					return nullptr;
				}
			}
		}
	}
}

void stub(void)
{
	return;
}