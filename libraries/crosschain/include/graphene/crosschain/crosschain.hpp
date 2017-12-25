#pragma once

#include <map>
#include <string>
#include <mutex>
#include <fc/reflect/reflect.hpp>
namespace graphene {
	namespace crosschain {
		class abstract_crosschain_interface;
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

			abstract_crosschain_interface * get_crosschain_handle(const std::string &name);
	   private:
			std::map<std::string, abstract_crosschain_interface *> crosschain_handles;
		};

	}
}
//FC_REFLECT(graphene::crosschain::crosschain_manager, (crosschain_handles))