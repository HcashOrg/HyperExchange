#include <graphene/crosschain/crosschain_transaction_record_plugin.hpp>
#include <graphene/crosschain/crosschain.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <fc/thread/thread.hpp>
namespace bpo = boost::program_options;
namespace graphene {
	namespace crosschain {
		using namespace graphene::chain;
		void crosschain_record_plugin::schedule_acquired_record_loop() {
			if (!started)
				return;
			fc::time_point now = fc::time_point::now();
			int64_t time_to_next_second = 20000000 - (now.time_since_epoch().count() % 20000000);
			if (time_to_next_second < 50000) {    // we must sleep for at least 50ms
				time_to_next_second += 1000000;
			}
			fc::time_point next_wakeup(now + fc::microseconds(time_to_next_second));
			if (_all_plugin){
				_acquire_crosschain_task = fc::schedule([this] {acquired_all_crosschain_record_loop(); },
					next_wakeup, "Acquire crosschain record ");
			}
			else {
			_acquire_crosschain_task = fc::schedule([this] {acquired_crosschain_record_loop(); },
				next_wakeup, "Acquire crosschain record ");
		}
			
		}
		void crosschain_record_plugin::acquired_all_crosschain_record_loop() {
			try {
				std::vector<fc::mutable_variant_object> history_params;
				std::string emu_asset_symbol = "";
				for (const auto& asset_symbol : _asset_symbols) {
					uint32_t start_num;

					chain::database& db = database();
					auto asset_obj = db.get_asset(asset_symbol);
					if (!asset_obj.valid())
						continue;
					if (!asset_obj->allow_withdraw_deposit)
						continue;
					//auto sess = db._undo_db.start_undo_session();
					auto& trx_iters = db.get_index_type<graphene::chain::transaction_history_count_index>().indices().get<graphene::chain::by_history_asset_symbol>();
					auto trx_iter = trx_iters.find(asset_symbol);
					auto db_count = trx_iters.size();
					if (trx_iter != trx_iters.end()) {
						start_num = trx_iter->local_count;
					}
					else {
						db.create<crosschain_transaction_history_count_object>([&](crosschain_transaction_history_count_object& obj) {
							obj.asset_symbol = asset_symbol;
							obj.local_count = 0;
						});
						start_num = 0;
						continue;
					}
					fc::mutable_variant_object multi_history_param_obj;
					multi_history_param_obj.set("chainId", asset_symbol);
					multi_history_param_obj.set("account", "");
					multi_history_param_obj.set("blockNum", start_num);
					multi_history_param_obj.set("limit", -1);
					history_params.push_back(multi_history_param_obj);
					emu_asset_symbol = asset_symbol;
				}
				if (emu_asset_symbol != "")
				{
					auto& manager = graphene::crosschain::crosschain_manager::get_instance();
					auto hdl = manager.get_crosschain_handle(std::string(emu_asset_symbol));
					auto pending_trxs = hdl->transaction_history_all(history_params);
					for (auto pending_trx_obj : pending_trxs)
					{
						try {
							std::string asset_symbol = pending_trx_obj["chainId"].as_string();
							uint32_t return_block_num = pending_trx_obj["blockNum"].as_uint64();
							auto pending_trx = pending_trx_obj["data"].get_array();
							chain::database& db = database();
							auto& last_trx_iters = db.get_index_type<graphene::chain::transaction_history_count_index>().indices().get<graphene::chain::by_history_asset_symbol>();
							auto last_trx_iter = last_trx_iters.find(asset_symbol);
							assert(last_trx_iter != last_trx_iters.end());
							hdl = manager.get_crosschain_handle(asset_symbol);
							db.modify(*last_trx_iter, [&](crosschain_transaction_history_count_object& obj) { obj.local_count = return_block_num; });
							for (const auto & trx_varient : pending_trx) {
								auto trx = trx_varient.get_object();
								hd_trx handle_trx;
								std::string real_trx;
								if (asset_symbol == "ETH" || asset_symbol.find("ERC") != asset_symbol.npos)
								{
									handle_trx = hdl->turn_trx(fc::variant_object("plugin_get_trx", trx));
									real_trx = handle_trx.trx_id;
									if (handle_trx.trx_id.find('|') != handle_trx.trx_id.npos)
									{
										auto pos = handle_trx.trx_id.find('|');
										real_trx = handle_trx.trx_id.substr(pos + 1);
									}
								}
								else {
									handle_trx = hdl->turn_trx(trx);
									real_trx = handle_trx.trx_id;
								}

								fc::scoped_lock<std::mutex> _lock(db.db_lock);
								auto& trx_iters = db.get_index_type<graphene::chain::acquired_crosschain_index>().indices().get<graphene::chain::by_acquired_trx_id>();
								auto trx_iter = trx_iters.find(real_trx);
								if (trx_iter != trx_iters.end()) {
									continue;
								}
								db.create<acquired_crosschain_trx_object>([&](acquired_crosschain_trx_object& obj) {
									obj.handle_trx = handle_trx;
									obj.handle_trx_id = real_trx;
									obj.acquired_transaction_state = acquired_trx_uncreate;
								});
							}
						}
						catch (fc::exception ex)
						{
							std::string err_log = ex.to_detail_string();
							ulog("maybe all transaction error! ${error} ", ("error", err_log.c_str()));
							continue;

						}
						catch (...)
						{
							ulog("maybe unkown error!");
							continue;
						}

					}
				}
			}
			catch (fc::exception ex)
			{
				std::string err_log = ex.to_detail_string();
				ulog("maybe something error! ${error} ", ("error", err_log.c_str()));

			}
			catch (...)
			{
				ulog("maybe unkown error!");
			}

			schedule_acquired_record_loop();
		}
		void crosschain_record_plugin::acquired_crosschain_record_loop() {
			try {
				/*std::vector<fc::mutable_variant_object> history_params;
				std::string emu_asset_symbol = "";
				for (const auto& asset_symbol : _asset_symbols) {
					uint32_t start_num;

					chain::database& db = database();
					auto asset_obj = db.get_asset(asset_symbol);
					if (!asset_obj.valid())
						continue;
					if (!asset_obj->allow_withdraw_deposit)
						continue;
					//auto sess = db._undo_db.start_undo_session();
					auto& trx_iters = db.get_index_type<graphene::chain::transaction_history_count_index>().indices().get<graphene::chain::by_history_asset_symbol>();
					auto trx_iter = trx_iters.find(asset_symbol);
					auto db_count = trx_iters.size();
					if (trx_iter != trx_iters.end()) {
						start_num = trx_iter->local_count;
					}
					else {
						db.create<crosschain_transaction_history_count_object>([&](crosschain_transaction_history_count_object& obj) {
							obj.asset_symbol = asset_symbol;
							obj.local_count = 0;
						});
						start_num = 0;
						continue;
					}
					fc::mutable_variant_object multi_history_param_obj;
					multi_history_param_obj.set("chainId", asset_symbol);
					multi_history_param_obj.set("account", "");
					multi_history_param_obj.set("blockNum", start_num);
					multi_history_param_obj.set("limit", -1);
					history_params.push_back(multi_history_param_obj);
					emu_asset_symbol = asset_symbol;
				}
				if (emu_asset_symbol != "")
				{
					auto& manager = graphene::crosschain::crosschain_manager::get_instance();
					auto hdl = manager.get_crosschain_handle(std::string(emu_asset_symbol));
					auto pending_trxs = hdl->transaction_history_all(history_params);
					for (auto pending_trx_obj : pending_trxs)
					{
						try {
							std::string asset_symbol = pending_trx_obj["chainId"].as_string();
							uint32_t return_block_num = pending_trx_obj["blockNum"].as_uint64();
							auto pending_trx = pending_trx_obj["data"].get_array();
							chain::database& db = database();
							auto& last_trx_iters = db.get_index_type<graphene::chain::transaction_history_count_index>().indices().get<graphene::chain::by_history_asset_symbol>();
							auto last_trx_iter = last_trx_iters.find(asset_symbol);
							assert(last_trx_iter != last_trx_iters.end());
							hdl = manager.get_crosschain_handle(asset_symbol);
							db.modify(*last_trx_iter, [&](crosschain_transaction_history_count_object& obj) { obj.local_count = return_block_num; });
							for (const auto & trx_varient : pending_trx) {
								auto trx = trx_varient.get_object();
								hd_trx handle_trx;
								std::string real_trx;
								if (asset_symbol == "ETH" || asset_symbol.find("ERC") != asset_symbol.npos)
								{
									handle_trx = hdl->turn_trx(fc::variant_object("plugin_get_trx", trx));
									real_trx = handle_trx.trx_id;
									if (handle_trx.trx_id.find('|') != handle_trx.trx_id.npos)
									{
										auto pos = handle_trx.trx_id.find('|');
										real_trx = handle_trx.trx_id.substr(pos + 1);
									}
								}
								else {
									handle_trx = hdl->turn_trx(trx);
									real_trx = handle_trx.trx_id;
								}

								fc::scoped_lock<std::mutex> _lock(db.db_lock);
								auto& trx_iters = db.get_index_type<graphene::chain::acquired_crosschain_index>().indices().get<graphene::chain::by_acquired_trx_id>();
								auto trx_iter = trx_iters.find(real_trx);
								if (trx_iter != trx_iters.end()) {
									continue;
								}
								db.create<acquired_crosschain_trx_object>([&](acquired_crosschain_trx_object& obj) {
									obj.handle_trx = handle_trx;
									obj.handle_trx_id = real_trx;
									obj.acquired_transaction_state = acquired_trx_uncreate;
								});
							}
						}
						catch (fc::exception ex)
						{
							std::string err_log = ex.to_detail_string();
							ulog("maybe all transaction error! ${error} ", ("error", err_log.c_str()));
							continue;

						}
						catch (...)
						{
							ulog("maybe unkown error!");
							continue;
						}
						
					}
				}*/
					
				for (const auto& asset_symbol : _asset_symbols) {
					auto& manager = graphene::crosschain::crosschain_manager::get_instance();
					auto hdl = manager.get_crosschain_handle(std::string(asset_symbol));
					if (hdl == nullptr)
					{
						continue;
					}
					//TODO:Change Magic Num to Macro
					std::string multi_sign_account;
					uint32_t return_block_num;
					uint32_t start_num;

					chain::database& db = database();
					auto asset_obj = db.get_asset(asset_symbol);
					if (!asset_obj.valid())
						continue;
					if (!asset_obj->allow_withdraw_deposit)
						continue;
					//auto sess = db._undo_db.start_undo_session();
					auto& trx_iters = db.get_index_type<graphene::chain::transaction_history_count_index>().indices().get<graphene::chain::by_history_asset_symbol>();
					auto trx_iter = trx_iters.find(asset_symbol);
					auto db_count = trx_iters.size();
					if (trx_iter != trx_iters.end()) {
						start_num = trx_iter->local_count;
					}
					else {
						db.create<crosschain_transaction_history_count_object>([&](crosschain_transaction_history_count_object& obj) {
							obj.asset_symbol = asset_symbol;
							obj.local_count = 0;
						});
						start_num = 0;
						continue;
					}

					auto pending_trx = hdl->transaction_history(asset_symbol, multi_sign_account, start_num, -1, return_block_num);

					auto& last_trx_iters = db.get_index_type<graphene::chain::transaction_history_count_index>().indices().get<graphene::chain::by_history_asset_symbol>();
					auto last_trx_iter = last_trx_iters.find(asset_symbol);
					assert(last_trx_iter != last_trx_iters.end());
					db.modify(*last_trx_iter, [&](crosschain_transaction_history_count_object& obj) { obj.local_count = return_block_num; });


					for (const auto & trx : pending_trx) {
						hd_trx handle_trx;
						std::string real_trx;
						if (asset_symbol == "ETH" || asset_symbol.find("ERC")!= asset_symbol.npos)
						{
							handle_trx = hdl->turn_trx(fc::variant_object("plugin_get_trx", trx));
							real_trx = handle_trx.trx_id;
							if (handle_trx.trx_id.find('|') != handle_trx.trx_id.npos)
							{
								auto pos = handle_trx.trx_id.find('|');
								real_trx = handle_trx.trx_id.substr(pos + 1);
							}
						}
						else {
							handle_trx = hdl->turn_trx(trx);
							real_trx = handle_trx.trx_id;
						}
						
						fc::scoped_lock<std::mutex> _lock(db.db_lock);
						auto& trx_iters = db.get_index_type<graphene::chain::acquired_crosschain_index>().indices().get<graphene::chain::by_acquired_trx_id>();
						auto trx_iter = trx_iters.find(real_trx);
						if (trx_iter != trx_iters.end()) {
							continue;
						}
						db.create<acquired_crosschain_trx_object>([&](acquired_crosschain_trx_object& obj) {
							obj.handle_trx = handle_trx;
							obj.handle_trx_id = real_trx;
							obj.acquired_transaction_state = acquired_trx_uncreate;
						});
					}

					//sess.commit();
				};
			}
			catch (fc::exception ex)
			{
				std::string err_log = ex.to_detail_string();
				ulog("maybe something error! ${error} ", ("error" ,err_log.c_str() ));

			}
			catch (...)
			{
				ulog("maybe unkown error!");
			}
			
			schedule_acquired_record_loop();
		}
		void crosschain_record_plugin::add_acquire_plugin(const std::string& asset_symbol) {
			_asset_symbols.insert(asset_symbol);
		}
		std::string crosschain_record_plugin::plugin_name()const {
			return "crosschain record";
		}

		void crosschain_record_plugin::plugin_set_program_options(
			boost::program_options::options_description &command_line_options,
			boost::program_options::options_description &config_file_options
		) {
			command_line_options.add_options()
				("guard-id,w", bpo::value<vector<string>>()->composing()->multitoken())
				("all-plugin-start",bpo::bool_switch()->notifier([this](bool e) {_all_plugin = e; }),"wither start all types of coin-collector in one plugin")
				;
			config_file_options.add(command_line_options);
		}
		void crosschain_record_plugin::plugin_initialize(const boost::program_options::variables_map& options){
			try{
				_options = &options;
				LOAD_VALUE_SET(options, "miner-id", _miners, chain::miner_id_type);
				LOAD_VALUE_SET(options, "guard-id", _guard, chain::guard_member_id_type)
			}FC_LOG_AND_RETHROW()
		}
		void crosschain_record_plugin::plugin_startup(){
			try {
				if (!_miners.empty() || !_guard.empty()) {
					started = true;
					auto fut = _thread.async([&] {schedule_acquired_record_loop(); }, "crosschain_record_plugin");
					fut.wait();
				}
				else {
					elog("No miner or guard in this client");
				}
			}FC_CAPTURE_AND_RETHROW()
		}
		bool crosschain_record_plugin::running() {
			return started;
		}
		void crosschain_record_plugin::startup_whatever() {
			try {
				if (started)
					return;
				started = true;
				auto fut = _thread.async([&] {schedule_acquired_record_loop(); }, "crosschain_record_plugin");
				fut.wait();
				
			}FC_CAPTURE_AND_RETHROW()
		}
		void crosschain_record_plugin::plugin_shutdown(){
			try {
				started = false;
				if (!_acquire_crosschain_task.canceled())
				{
					_acquire_crosschain_task.cancel_and_wait();
				}
				_thread.quit();
				
			}FC_CAPTURE_AND_RETHROW()
			return;
		}
	}
}