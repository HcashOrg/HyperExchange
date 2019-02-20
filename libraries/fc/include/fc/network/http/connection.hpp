#pragma once
#include <fc/vector.hpp>
#include <fc/string.hpp>
#include <memory>
#include <boost/asio.hpp>
#include <mutex>
#include <condition_variable>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
namespace fc {
	namespace ip { class endpoint; }
	class tcp_socket;

	namespace http {

		struct header
		{
			header(fc::string k, fc::string v)
				:key(fc::move(k)), val(fc::move(v)) {}
			header() {}
			fc::string key;
			fc::string val;
		};

		typedef std::vector<header> headers;

		struct reply
		{
			enum status_code {
				OK = 200,
				RecordCreated = 201,
				BadRequest = 400,
				NotAuthorized = 401,
				NotFound = 404,
				Found = 302,
				InternalServerError = 500
			};
			reply(status_code c = OK) :status(c) {}
			int                     status;
			std::vector<header>      headers;
			std::vector<char>        body;
		};

		struct request
		{
			fc::string get_header(const fc::string& key)const;
			fc::string              remote_endpoint;
			fc::string              method;
			fc::string              domain;
			fc::string              path;
			std::vector<header>     headers;
			std::vector<char>       body;
		};

		std::vector<header> parse_urlencoded_params(const fc::string& f);

		/**
		 *  Connections have reference semantics, all copies refer to the same
		 *  underlying socket.
		 */
		class connection
		{
		public:
			connection();
			~connection();
			// used for clients
			void         connect_to(const fc::ip::endpoint& ep);
			http::reply  request(const fc::string& method, const fc::string& url, const fc::string& body = std::string(), const headers& = headers());

			// used for servers
			fc::tcp_socket& get_socket()const;

			http::request    read_request()const;

			class impl;
		private:
			std::unique_ptr<impl> my;
		};
		typedef std::shared_ptr<connection> connection_ptr;
		class connection_sync
		{
		public:
			connection_sync();
			~connection_sync();
			// used for clients
			void         connect_to(const fc::ip::endpoint& ep);
			int     connect_to_servers(const std::vector<fc::ip::endpoint>& eps, std::vector<fc::ip::endpoint>& res );
			http::reply  request(const fc::string& method, const fc::string& url, const fc::string& body = std::string(), const headers& = headers());

			// used for servers
			//boost::asio::ip::tcp::socket get_socket()const;

		 //http::request    read_request()const;
			http::reply parse_reply();
			void handle_reply(const boost::system::error_code & error);
			void handle_reply(const boost::system::error_code & error, size_t bytes_transferred);
			void check_deadline(const boost::system::error_code & error);
			void close_socket();
		private:
			std::mutex read_lock;
			std::condition_variable m_cond;
			bool is_done = false;
			bool is_timeout = false;
			bool is_release = false;
			boost::asio::deadline_timer _deadline;
			boost::asio::ip::tcp::socket _socket;
			boost::asio::streambuf line;
		};

	}
} // fc::http

#include <fc/reflect/reflect.hpp>
FC_REFLECT(fc::http::header, (key)(val))