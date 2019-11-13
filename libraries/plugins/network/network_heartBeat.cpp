#include <graphene/network/network_heartBeat.hpp>
#include <fc/io/json.hpp>
namespace bpo = boost::program_options;
namespace graphene {
	namespace network_plugin {
		void network_heartBeat_plugin::plugin_initialize(const boost::program_options::variables_map& options)
		{
			try {
				if (options.count("url") > 0)
				{
					std::cout << "url is " << _url << std::endl;
					_url = options["url"].as<string>();
				}
					
				
			}FC_LOG_AND_RETHROW()
		}

		void network_heartBeat_plugin::plugin_startup()
		{
			if (!_url.empty()) {
				auto fut = _thread.async([&] {schedule_send_heartBeatMsg_loop(); }, "crosschain_record_plugin");
				fut.wait();
			}
			else {
				elog("No need to start this plugin.");
			}
		}
		
		void network_heartBeat_plugin::plugin_shutdown()
		{

		}

		void network_heartBeat_plugin::send_heartBeatMsg()
		{
			try
			{
				if (_listenIPport == fc::ip::endpoint())
				{
					_listenIPport = app().p2p_node()->get_actual_listening_endpoint();
				}
				if (_chainId.empty())
				{
					_chainId = database().get_chain_id().str();
				}
				fc::http::connection conn;
				std::ostringstream req_body;
				const auto& _db = database();

				std::string status;
				if (last_block != _db.head_block_num())
					status = "ok";
				else
					status = "error";
				last_block = _db.head_block_num();
				int num = app().p2p_node()->get_connection_count();

				HeartBeatMsg msg;
				msg.blockHeight = last_block;
				msg.id = "hx_node_"+fc::to_string(_listenIPport.port());
				msg.peerCount = num;
				msg.chainId = _chainId;
				msg.ip = _listenIPport.get_address();
				msg.p2pPort = _listenIPport.port();
				msg.status = status;
				conn.connect_to(fc::ip::endpoint().from_string(_url));
				fc::http::headers header;
				header.emplace_back(fc::http::header("Content-Type","application/json"));
				conn.request("POST", "http://" + _url+"/api/node/health_check",fc::json::to_string(msg),header);
			}
			catch (...)
			{

			}
			schedule_send_heartBeatMsg_loop();
		}


		void network_heartBeat_plugin::schedule_send_heartBeatMsg_loop()
		{
			fc::time_point now = fc::time_point::now();
			int64_t time_to_next_second = 60000000 - (now.time_since_epoch().count() % 60000000);
			if (time_to_next_second < 50000) {    // we must sleep for at least 50ms
				time_to_next_second += 1000000;
			}
			fc::time_point next_wakeup(now + fc::microseconds(time_to_next_second));

			fc::schedule([this] {send_heartBeatMsg(); },
				next_wakeup, "send heart beat msg. ");
		}


		std::string network_heartBeat_plugin::plugin_name() const {
			return "network hearBeat plugin";
		}

		void network_heartBeat_plugin::plugin_set_program_options(
			boost::program_options::options_description &command_line_options,
			boost::program_options::options_description &config_file_options
		)
		{
			command_line_options.add_options()
				("url,w", bpo::value<string>()->composing());
		}
	}

}