#include <graphene/crosschain/crosschain_interface_btc.hpp>
#include <fc/network/ip.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>


namespace graphene {
	namespace crosschain {


		void crosschain_interface_btc::initialize_config(fc::variant_object &json_config)
		{
			_config = json_config;
			_rpc_method = "POST";
			_rpc_url = "/";
			std::string rpc_user_pass = _config["rpcuser"].as_string() + ":" + _config["rpcpassword"].as_string();
			fc::http::header auth("Authorization", fc::base64_encode(rpc_user_pass));
			_rpc_headers.push_back(auth);

			_connection->connect_to(fc::ip::endpoint(fc::ip::address(_config["ip"].as_string()), _config["port"].as_uint64()));
		}

		bool crosschain_interface_btc::unlock_wallet(std::string wallet_name, std::string wallet_passprase, uint32_t duration)
		{
			return false;
		}

		bool crosschain_interface_btc::open_wallet(std::string wallet_name)
		{
			if (_connection->get_socket().is_open())
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		void crosschain_interface_btc::close_wallet()
		{
			
		}

		std::vector<std::string> crosschain_interface_btc::wallet_list()
		{
			return std::vector<std::string>();
		}

		bool graphene::crosschain::crosschain_interface_btc::create_wallet(std::string wallet_name, std::string wallet_passprase)
		{
			return false;
		}

		std::string crosschain_interface_btc::create_normal_account(std::string account_name)
		{
			auto response = _connection->request(_rpc_method, _rpc_url, "{ \
				\"id\": 1, \
				\"method\" : \"getnewaddress\", \
				\"params\" : [] \
			}",	_rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).as<fc::mutable_variant_object>();
				return resp["result"].as_string();
			}
			else
				FC_THROW(account_name) ;
		}
		
		std::string crosschain_interface_btc::create_multi_sig_account(std::string account_name, std::vector<std::string> addresses, uint32_t nrequired)
		{
			std::ostringstream req_body;
			req_body << "{ \"id\": 1, \"method\": \"createmultisig\", \"params\": ["
				<< nrequired << ", [";
			for (int i = 0; i < addresses.size(); ++i)
			{
				req_body << "\"" << addresses[i] << "\"";
				if (i < addresses.size() - 1)
				{
					req_body << ",";
				}

			}
			req_body << "]]}";
			auto response = _connection->request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).as<fc::mutable_variant_object>();
				return resp["result"].as_string();
			}
			else
				FC_THROW(account_name) ;
			return std::string();
		}

		std::vector<graphene::crosschain::hd_trx> crosschain_interface_btc::deposit_transaction_query(std::string user_account, uint32_t from_block, uint32_t limit)
		{
			return std::vector<graphene::crosschain::hd_trx>();
		}

		fc::variant_object crosschain_interface_btc::transaction_query(std::string trx_id)
		{
			std::ostringstream req_body;
			req_body << "{ \"id\": 1, \"method\": \"gettransaction\", \"params\": [\""
				<< trx_id << "\"]}";
			auto response = _connection->request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).as<fc::mutable_variant_object>();
				return resp;
			}
			else
				FC_THROW(trx_id);
		}

		fc::variant_object crosschain_interface_btc::transfer(std::string &from_account, std::string &to_account, uint64_t amount, std::string &symbol, std::string &memo, bool broadcast /*= true*/)
		{
			return fc::variant_object();
		}

		fc::variant_object crosschain_interface_btc::create_multisig_transaction(std::string &from_account, std::string &to_account, uint64_t amount, std::string &symbol, std::string &memo, bool broadcast /*= true*/)
		{
			std::ostringstream req_body;
			req_body << "{ \"id\": 1, \"method\": \"createrawtransaction\", \"params\": [\""
				<< "TODO" << "\"]}";
			auto response = _connection->request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).as<fc::mutable_variant_object>();
				return resp;
			}
			else
				FC_THROW("TODO");
			return fc::variant_object();
		}

		std::string crosschain_interface_btc::sign_multisig_transaction(fc::variant_object trx, std::string &sign_account, bool broadcast /*= true*/)
		{
			std::ostringstream req_body;
			req_body << "{ \"id\": 1, \"method\": \"createrawtransaction\", \"params\": [\""
				<< "TODO" << "\"]}";
			auto response = _connection->request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).as<fc::mutable_variant_object>();
				return resp["result"]["hex"].as_string();
			}
			else
				FC_THROW("TODO");
			return std::string();
		}

		fc::variant_object crosschain_interface_btc::merge_multisig_transaction(fc::variant_object &trx, std::vector<std::string> signatures)
		{
			std::ostringstream req_body;
			req_body << "{ \"id\": 1, \"method\": \"combinerawtransaction\", \"params\": [[";
			for (auto itr = signatures.begin(); itr != signatures.end(); ++itr)
			{
				req_body << "\"" << *itr << "\"";
				if (itr != signatures.end() - 1)
				{
					req_body << ",";
				}
			}
			req_body << "\"]]}";
				
			auto response = _connection->request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).as<fc::mutable_variant_object>();
				auto error = resp["error"];
				if (error.as_string() == "null")
				{
					return resp;
				}
			}
			else
				FC_THROW(signature);
			return fc::variant_object();
		}

		bool crosschain_interface_btc::validate_link_trx(const hd_trx &trx)
		{
			return false;
		}

		bool crosschain_interface_btc::validate_link_trx(const std::vector<hd_trx> &trx)
		{
			return false;
		}

		bool crosschain_interface_btc::validate_other_trx(const fc::variant_object &trx)
		{
			return false;
		}

		bool crosschain_interface_btc::validate_signature(const std::string &account, const std::string &content, const std::string &signature)
		{
			std::ostringstream req_body;
			req_body << "{ \"id\": 1, \"method\": \"verifymessage\", \"params\": [\""
				<< account << "\",\"" << signature << "\",\"" << content << "\"]}";
			auto response = _connection->request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).as<fc::mutable_variant_object>();
				auto error = resp["error"];
				if (error.as_string() == "null")
				{
					return resp["result"].as_bool();
				}
				else
				{
					return false;
				}
				
			}
			else
				FC_THROW(signature);
		}

		bool crosschain_interface_btc::create_signature(const std::string &account, const std::string &content, std::string &signature)
		{
			std::ostringstream req_body;
			req_body << "{ \"id\": 1, \"method\": \"signmessage\", \"params\": [\""
				<< account << "\",\"" << signature << "\",\"" << content << "\"]}";
			auto response = _connection->request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).as<fc::mutable_variant_object>();
				auto error = resp["error"];
				if (error.as_string() == "null")
				{
					signature = resp["result"].as_string();
					return true;
				}
				else
				{
					return false;
				}
				
			}
			else
				FC_THROW(signature);
		}

		graphene::crosschain::hd_trx crosschain_interface_btc::turn_trx(const fc::variant_object & trx)
		{
			return graphene::crosschain::hd_trx();
		}

		void crosschain_interface_btc::broadcast_transaction(const fc::variant_object &trx)
		{

		}

		std::vector<fc::variant_object> crosschain_interface_btc::query_account_balance(const std::string &account)
		{
			return std::vector<fc::variant_object>();
		}

		std::vector<fc::variant_object> crosschain_interface_btc::transaction_history(std::string &user_account, uint32_t start_block, uint32_t limit)
		{
			return std::vector<fc::variant_object>();
		}

		std::string crosschain_interface_btc::export_private_key(std::string &account, std::string &encrypt_passprase)
		{
			std::ostringstream req_body;
			req_body << "{ \"id\": 1, \"method\": \"dumpprivkey\", \"params\": [\""
				<< account << "\"]}";
			auto response = _connection->request(_rpc_method, _rpc_url, req_body.str(), _rpc_headers);
			if (response.status == fc::http::reply::OK)
			{
				auto resp = fc::json::from_string(std::string(response.body.begin(), response.body.end())).as<fc::mutable_variant_object>();
				return resp["result"].as_string();
			}
			else
				FC_THROW(account);
		}

		std::string crosschain_interface_btc::import_private_key(std::string &account, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_btc::backup_wallet(std::string &wallet_name, std::string &encrypt_passprase)
		{
			return std::string();
		}

		std::string crosschain_interface_btc::recover_wallet(std::string &wallet_name, std::string &encrypt_passprase)
		{
			return std::string();
		}



	}
}