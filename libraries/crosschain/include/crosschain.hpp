#pragma once

#include <map>
#include <string>

namespace graphene {
	namespace crosschain {
		class abstract_cross_chain_interface;
		class crosschain_manager
		{
		public:
			crosschain_manager();
			~crosschain_manager();

			static crosschain_manager &get_instance()
			{
				static crosschain_manager mgr;
				return mgr;
			}

			abstract_cross_chain_interface * get_cross_chain_handle(std::string &name);

		private:
			std::mutex mutex;
			std::map<std::string, abstract_cross_chain_interface *> cross_chain_handles;
		};

		crosschain_manager::crosschain_manager()
		{
		}

		crosschain_manager::~crosschain_manager()
		{
		}
	}
}
void stub(void);