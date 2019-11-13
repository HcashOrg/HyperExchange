/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/app/application.hpp>
#include <fc/thread/future.hpp>
#include <mutex>
namespace graphene { namespace network_plugin {

namespace block_production_condition
{
   enum block_production_condition_enum
   {
      produced = 0,
      not_synced = 1,
      not_my_turn = 2,
      not_time_yet = 3,
      no_private_key = 4,
      low_participation = 5,
      lag = 6,
      consecutive = 7,
      exception_producing_block = 8,
	  stopped = 9
   };
}

struct HeartBeatMsg
{
	string id;
	vector<string> labels;
	fc::ip::address ip;
	string status;
	string chainId;
	uint32_t blockHeight;
	uint32_t p2pPort;
	uint32_t connectNum ;
};



class network_heartBeat_plugin :public graphene::app::plugin {
public:
	network_heartBeat_plugin() {}
   ~network_heartBeat_plugin() {}

   std::string plugin_name()const override;

   virtual void plugin_set_program_options(
      boost::program_options::options_description &command_line_options,
      boost::program_options::options_description &config_file_options
      ) override;
   virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

private:
	void send_heartBeatMsg();
	void schedule_send_heartBeatMsg_loop();
	fc::thread                 _thread;
	std::string                _url;
	fc::ip::endpoint           _ip;
	fc::ip::endpoint           _listenIPport;
	std::string                _chainId;
	uint32_t                        last_block = 0;
};

} } //graphene::miner_plugin
FC_REFLECT(graphene::network_plugin::HeartBeatMsg, (id)(labels)(ip)(status)(chainId)(blockHeight)(p2pPort)(connectNum))