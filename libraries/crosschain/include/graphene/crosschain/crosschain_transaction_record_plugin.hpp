#pragma once

#include <graphene/app/application.hpp>
#include <graphene/app/plugin.hpp>
#include <set>
#include <string>
namespace graphene {
	namespace crosschain {
		class crosschain_record_plugin :public graphene::app::plugin {
		public:
			crosschain_record_plugin();
			~crosschain_record_plugin() {};

			std::string plugin_name()const override;

			virtual void plugin_set_program_options(
				boost::program_options::options_description &command_line_options,
				boost::program_options::options_description &config_file_options
			) override;

			virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
			virtual void plugin_startup() override;
			void startup_whatever();
			bool running();
			virtual void plugin_shutdown() override;

			void add_acquire_plugin(const std::string&);
		private:
			void schedule_acquired_record_loop();
			void acquired_crosschain_record_loop();
			void acquired_all_crosschain_record_loop();
			fc::future<void> _acquire_crosschain_task;
			fc::future<void> _fut;
			std::set<std::string> _asset_symbols;
			boost::program_options::variables_map _options;
			std::set<chain::miner_id_type> _miners;
			std::set<chain::guard_member_id_type> _guard;
			fc::thread                            _thread;
			bool started = false;
			bool _all_plugin = false;
		};
	}
}