
#include <graphene/crosschain/crosschain_interface_ub.hpp>
#include <fc/network/ip.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>
#include <iostream>

namespace graphene {
	namespace crosschain {


		void crosschain_interface_ub::initialize_config(fc::variant_object &json_config)
		{
			_config = json_config;
			_rpc_method = "POST";
			_rpc_url = "http://";
			_rpc_url = _rpc_url + _config["ip"].as_string() + ":" + std::string(_config["port"].as_string()) + "/api";
		}
		bool crosschain_interface_ub::valid_config()
		{
			if (_config.contains("ip") && _config.contains("port"))
				return true;
			return false;
		}
		bool crosschain_interface_ub::unlock_wallet(std::string wallet_name, std::string wallet_passprase, uint32_t duration)
		{
			return false;
		}

		bool crosschain_interface_ub::open_wallet(std::string wallet_name)
		{
			return false;
		}

		void crosschain_interface_ub::close_wallet()
		{

		}

		std::vector<std::string> crosschain_interface_ub::wallet_list()
		{
			return std::vector<std::string>();
		}

		bool graphene::crosschain::crosschain_interface_ub::create_wallet(std::string wallet_name, std::string wallet_passprase)
		{
			return false;
		}

		std::string crosschain_interface_ub::create_normal_account(std::string account_name)
		{

			std::string json_str = "{ \"jsonrpc\": \"2.0\", \
				\"params\" : {\"chainId\":\"ub\" \
			}, \
				\"id\" : \"45\", \
				\"method\" : \"Zchain.Address.Create\" \
			}";
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, json_str, _rpc_headers);
			std::cout << std::string(response.body.begin(), response.body.end()) << std::endl;
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end()));
				auto ret = resp["result"].get_object()["address"];
				return ret.as_string();
			}
			else if (response.status == fc::http::reply::BadRequest)
				throw(fc::http::reply::BadRequest);
		}

		std::map<std::string, std::string> crosschain_interface_ub::create_multi_sig_account(std::string account_name, std::vector<std::string> addresses, uint32_t nrequired)
		{
			std::ostringstream req_body;
			req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Multisig.Create\" ,\
				\"params\" : {\"chainId\":\"ub\" ,\"amount\": " << nrequired << ",\"" << "addrs\":[";

			for (int i = 0; i < addresses.size(); ++i)
			{
				req_body << "\"" << addresses[i] << "\"";
				if (i < addresses.size() - 1)
				{
					req_body << ",";
				}

			}
			req_body << "]}}";
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			std::cout << response.status << std::endl;
			if (response.status == fc::http::reply::OK)
			{
				std::cout << std::string(response.body.begin(), response.body.end()) << std::endl;
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end()));
				auto ret = resp.get_object();
				if (ret.contains("result"))
				{
					std::map<std::string, std::string> map;
					auto result = ret["result"].get_object();
					FC_ASSERT(result.contains("address"));
					map["address"] = result["address"].as_string();
					FC_ASSERT(result.contains("redeemScript"));
					map["redeemScript"] = result["redeemScript"].as_string();
					return map;
				}
			}

			FC_THROW(account_name);
			//return std::string();
		}

		std::vector<graphene::crosschain::hd_trx> crosschain_interface_ub::deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit)
		{
			return std::vector<graphene::crosschain::hd_trx>();
		}

		fc::variant_object crosschain_interface_ub::transaction_query(std::string trx_id)
		{
			std::ostringstream req_body;
			req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Trans.queryTrans\" ,\
				\"params\" : {\"chainId\":\"ub\" ,\"trxid\": \"" << trx_id << "\"}}";
			std::cout << req_body.str() << std::endl;
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).get_object();

				FC_ASSERT(resp.contains("result"));
				FC_ASSERT(resp["result"].get_object().contains("data"));
				return resp["result"].get_object()["data"].get_object();
			}
			else
				FC_THROW(trx_id);
		}

		fc::variant_object crosschain_interface_ub::transfer(const std::string &from_account, const std::string &to_account, uint64_t amount, const std::string &symbol, const std::string &memo, bool broadcast /*= true*/)
		{
			return fc::variant_object();
		}

		fc::variant_object crosschain_interface_ub::create_multisig_transaction(const std::string &from_account, const std::map<std::string, std::string> dest_info, const std::string &symbol, const std::string &memo, const std::string& prk)
		{
			std::ostringstream req_body;
			req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Trans.createTrx\" ,\
				\"params\" : {\"chainId\":\"ub\" ,\"from_addr\": \"" << from_account << "\",\"dest_info\":{";// << to_account << "\",\"amount\":" << amount << "}}";
			for (auto iter = dest_info.begin(); iter != dest_info.end(); ++iter)
			{
				if (iter != dest_info.begin())
					req_body << ",";
				req_body << "\"" << iter->first << "\":" << iter->second;
			}
			req_body << "}}}";
			std::cout << req_body.str() << std::endl;
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto str = std::string(response.body.begin(), response.body.end());
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end()));
				auto ret = resp.get_object()["result"].get_object();
				FC_ASSERT(ret.contains("data"));
				return ret["data"].get_object();
			}
			else
				FC_THROW("TODO");
			return fc::variant_object();

		}

		fc::variant_object crosschain_interface_ub::create_multisig_transaction(std::string &from_account, std::string &to_account, const std::string& amount, std::string &symbol, std::string &memo, bool broadcast /*= true*/)
		{
			std::ostringstream req_body;
			req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Trans.createTrx\" ,\
				\"params\" : {\"chainId\":\"ub\" ,\"from_addr\": \"" << from_account << "\",\"to_addr\":\"" << to_account << "\",\"amount\":" << amount << "}}";
			std::cout << req_body.str() << std::endl;
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto str = std::string(response.body.begin(), response.body.end());
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end()));
				auto ret = resp.get_object()["result"].get_object();
				FC_ASSERT(ret.contains("data"));
				return ret["data"].get_object();
			}
			else
				FC_THROW("TODO");
			return fc::variant_object();
		}

		std::string crosschain_interface_ub::sign_multisig_transaction(fc::variant_object trx, const std::string &sign_account, const std::string& redeemScript, bool broadcast /*= true*/)
		{

			std::ostringstream req_body;
			req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Trans.Sign\" ,\
				\"params\" : {\"chainId\":\"ub\" ,\"addr\": \"" << sign_account << "\",\"trx_hex\":\"" << trx["hex"].as_string() << "\"," << "\"redeemScript" << "\":" << "\"" << redeemScript << "\"}}";
			std::cout << req_body.str() << std::endl;
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).get_object();
				std::cout << std::string(response.body.begin(), response.body.end()) << std::endl;
				FC_ASSERT(resp.contains("result"));
				auto ret = resp["result"].get_object();
				FC_ASSERT(ret.contains("data"));
				return ret["data"].get_object()["hex"].as_string();
			}
			else
				FC_THROW("TODO");
			return std::string();
		}

		fc::variant_object crosschain_interface_ub::merge_multisig_transaction(fc::variant_object &trx, std::vector<std::string> signatures)
		{
			std::ostringstream req_body;
			req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Trans.CombineTrx\" ,\
				\"params\" : {\"chainId\":\"ub\" ,\"transactions\": [";
			for (auto itr = signatures.begin(); itr != signatures.end(); ++itr)
			{
				req_body << "\"" << *itr << "\"";
				if (itr != signatures.end() - 1)
				{
					req_body << ",";
				}
			}
			req_body << "]}}";
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).get_object();
				FC_ASSERT(resp.contains("result"));
				auto ret = resp["result"].get_object();
				FC_ASSERT(ret.contains("data"));
				return ret["data"].get_object();
			}
			else
				FC_THROW(std::string(response.body.begin(), response.body.end()));
			return fc::variant_object();
		}

		bool crosschain_interface_ub::validate_link_trx(const hd_trx &trx)
		{
			return false;
		}

		bool crosschain_interface_ub::validate_link_trx(const std::vector<hd_trx> &trx)
		{
			return false;
		}

		bool crosschain_interface_ub::validate_other_trx(const fc::variant_object &trx)
		{
			return true;
		}
		bool crosschain_interface_ub::validate_address(const std::string& addr)
		{
			std::ostringstream req_body;
			req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Address.validate\" ,\
				\"params\" : {\"chainId\":\"ub\" ,\"addr\": " << "\"" << addr << "}}";
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end()));
				auto ret = resp.get_object();
				if (ret.contains("result"))
				{
					auto result = ret["result"].get_object();
					return result["valid"].as_bool();
				}
				else
				{
					return false;
				}

			}
			else
				FC_THROW(addr);
		}
		bool crosschain_interface_ub::validate_signature(const std::string &account, const std::string &content, const std::string &signature)
		{
			std::ostringstream req_body;
			req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Crypt.VerifyMessage\" ,\
				\"params\" : {\"chainId\":\"ub\" ,\"addr\": " << "\"" << account << "\"," << "\"message\":" << "\""  \
				<< content << "\"," << "\"" << "signature" << "\":\"" << signature << "\"" << "}}";
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end()));
				auto ret = resp.get_object();
				if (ret.contains("result"))
				{
					auto result = ret["result"].get_object();
					return result["data"].as_bool();
				}
				else
				{
					return false;
				}

			}
			else
				FC_THROW(signature);
		}

		bool crosschain_interface_ub::create_signature(const std::string &account, const std::string &content, std::string &signature, const std::string& prk)
		{
			std::ostringstream req_body;
			req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Crypt.Sign\" ,\
				\"params\" : {\"chainId\":\"ub\" ,\"addr\": " << "\"" << account << "\"," << "\"message\":" << "\"" << content << "\"" << "}}";
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end()));
				auto result = resp.get_object();
				if (result.contains("result"))
				{
					signature = result["result"].get_object()["data"].as_string();
					return true;
				}
				return false;
			}
			else
				FC_THROW(signature);
		}

		std::map<std::string, graphene::crosschain::hd_trx> crosschain_interface_ub::turn_trxs(const fc::variant_object & trx)
		{
			hd_trx hdtx;
			std::map<std::string, graphene::crosschain::hd_trx> hdtxs;
			try {
				auto tx = trx["trx"].get_object();
				hdtx.asset_symbol = chain_type;
				hdtx.trx_id = tx["hash"].as_string();
				const std::string to_addr = tx["vout"].get_array()[0].get_object()["scriptPubKey"].get_object()["addresses"].get_array()[0].as_string();
				const std::string from_trx_id = tx["vin"].get_array()[0].get_object()["txid"].as_string();
				const auto index = tx["vin"].get_array()[0].get_object()["vout"].as_uint64();
				auto from_trx = transaction_query(from_trx_id);
				const std::string from_addr = from_trx["vout"].get_array()[index].get_object()["scriptPubKey"].get_object()["addresses"].get_array()[0].as_string();
				hdtx.from_account = from_addr;
				for (auto vouts : tx["vout"].get_array())
				{
					auto addrs = vouts.get_object()["scriptPubKey"].get_object()["addresses"].get_array();
					for (auto addr : addrs)
					{
						if (addr.as_string() == from_addr)
							continue;
						hdtx.to_account = addr.as_string();
						auto amount = vouts.get_object()["value"].as_double();
						char temp[1024];
						std::sprintf(temp, "%g", amount);
						hdtx.amount = temp;
						hdtxs[hdtx.to_account] = hdtx;

					}

				}

			}
			FC_CAPTURE_AND_RETHROW((trx));
			return hdtxs;
		}

		graphene::crosschain::hd_trx crosschain_interface_ub::turn_trx(const fc::variant_object & trx)
		{
			hd_trx hdtx;
			try {
				hdtx.asset_symbol = chain_type;
				hdtx.trx_id = trx["txid"].as_string();

				hdtx.from_account = trx["from_account"].as_string();
				hdtx.to_account = trx["to_account"].as_string();
				hdtx.block_num = trx["blockNum"].as_uint64();
				hdtx.amount = trx["amount"].as_string();


			}
			FC_CAPTURE_AND_RETHROW((trx));
			return hdtx;
		}

		void crosschain_interface_ub::broadcast_transaction(const fc::variant_object &trx)
		{
			try {
				std::ostringstream req_body;
				req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Trans.broadcastTrx\" ,\
				\"params\" : {\"chainId\":\"ub\" ,\"trx\": " << "\"" << trx["hex"].as_string() << "\"" << "}}";
				fc::http::connection conn;
				conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
				auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
				if (response.status == fc::http::reply::OK)
				{
					auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end()));
					auto result = resp.get_object();
					if (result.contains("result"))
					{
						auto hex = result["result"].get_object()["data"].as_string();
					}
				}
			}FC_CAPTURE_AND_LOG((trx));
		}

		std::vector<fc::variant_object> crosschain_interface_ub::query_account_balance(const std::string &account)
		{
			return std::vector<fc::variant_object>();
		}

		std::vector<fc::variant_object> crosschain_interface_ub::transaction_history(std::string symbol, const std::string &user_account, uint32_t start_block, uint32_t limit, uint32_t& end_block_num)
		{
			std::vector<fc::variant_object> return_value;
			std::string local_symbol = "ub";
			std::ostringstream req_body;
			req_body << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Transaction.Deposit.History\" ,\
				\"params\" : {\"chainId\":\"" << local_symbol << "\",\"account\": \"\" ,\"limit\": 0 ,\"blockNum\": " << start_block << "}}";
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end()));
				//std::cout << std::string(response.body.begin(), response.body.end());
				auto result = resp.get_object();
				if (result.contains("result"))
				{
					end_block_num = result["result"].get_object()["blockNum"].as_uint64();
					for (auto one_data : result["result"].get_object()["data"].get_array())
					{
						//std::cout << one_data.get_object()["txid"].as_string();
						return_value.push_back(one_data.get_object());
					}
				}
			}
			std::ostringstream req_body1;
			req_body1 << "{ \"jsonrpc\": \"2.0\", \
                \"id\" : \"45\", \
				\"method\" : \"Zchain.Transaction.Withdraw.History\" ,\
				\"params\" : {\"chainId\":\"" << local_symbol << "\",\"account\": \"\" ,\"limit\": 0 ,\"blockNum\": " << start_block << "}}";
			fc::http::connection conn1;
			conn1.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response1 = conn1.request(_rpc_method, _rpc_url, req_body1.str(), _rpc_headers);
			std::cout << req_body1.str() << std::endl;
			if (response1.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response1.body.begin(), response1.body.end()));
				//std::cout << std::string(response.body.begin(), response.body.end());
				auto result = resp.get_object();
				if (result.contains("result"))
				{
					end_block_num = std::max(uint32_t(result["result"].get_object()["blockNum"].as_uint64()), end_block_num);
					for (auto one_data : result["result"].get_object()["data"].get_array())
					{
						//std::cout << one_data.get_object()["txid"].as_string();
						return_value.push_back(one_data.get_object());
					}
				}
			}

			return return_value;
		}

		std::string crosschain_interface_ub::export_private_key(std::string &account, std::string &encrypt_passprase)
		{
			std::ostringstream req_body;
			req_body << "{ \"id\": 1, \"method\": \"dumpprivkey\", \"params\": [\""
				<< account << "\"]}";
			fc::http::connection conn;
			conn.connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
			auto response = conn.request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).as<fc::mutable_variant_object>();
				return resp["result"].as_string();
			}
			else
				FC_THROW(account);
		}

		std::string crosschain_interface_ub::import_private_key(std::string &account, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_ub::backup_wallet(std::string &wallet_name, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_ub::recover_wallet(std::string &wallet_name, std::string &encrypt_passprase)
		{
			return std::string();
		}
	}
}
