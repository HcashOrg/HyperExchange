#include <fc/network/http/connection.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/io/sstream.hpp>
#include <fc/io/iostream.hpp>
#include <fc/exception/exception.hpp>
#include <fc/network/ip.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/url.hpp>
#include <boost/algorithm/string.hpp>
#include <fc/asio.hpp>

#include <iostream>
#include <boost/shared_ptr.hpp>

class fc::http::connection::impl
{
public:
	fc::tcp_socket sock;
	fc::ip::endpoint ep;
	impl() {
	}

	int read_until(char* buffer, char* end, char c = '\n') {
		char* p = buffer;
		// try {
		while (p < end && 1 == sock.readsome(p, 1)) {
			if (*p == c) {
				*p = '\0';
				return (p - buffer) - 1;
			}
			++p;
		}
		// } catch ( ... ) {
		//   elog("%s", fc::current_exception().diagnostic_information().c_str() );
		   //elog( "%s", fc::except_str().c_str() );
		// }
		return (p - buffer);
	}

	fc::http::reply parse_reply() {
		fc::http::reply rep;
		try {
			std::vector<char> line(1024 * 8);
			int s = read_until(line.data(), line.data() + line.size(), ' '); // HTTP/1.1
			s = read_until(line.data(), line.data() + line.size(), ' '); // CODE
			rep.status = static_cast<int>(to_int64(fc::string(line.data())));
			s = read_until(line.data(), line.data() + line.size(), '\n'); // DESCRIPTION

			while ((s = read_until(line.data(), line.data() + line.size(), '\n')) > 1) {
				fc::http::header h;
				char* end = line.data();
				while (*end != ':')++end;
				h.key = fc::string(line.data(), end);
				++end; // skip ':'
				++end; // skip space
				char* skey = end;
				while (*end != '\r') ++end;
				h.val = fc::string(skey, end);
				rep.headers.push_back(h);
				if (boost::iequals(h.key, "Content-Length")) {
					rep.body.resize(static_cast<size_t>(to_uint64(fc::string(h.val))));
				}
			}
			if (rep.body.size()) {
				sock.read(rep.body.data(), rep.body.size());
			}
			return rep;
		}
		catch (fc::exception& e) {
			elog("${exception}", ("exception", e.to_detail_string()));
			sock.close();
			rep.status = http::reply::InternalServerError;
			return rep;
		}
	}
};



namespace fc {
	namespace http {

		connection::connection()
			:my(new connection::impl()) {}
		connection::~connection() {}


		// used for clients
		void       connection::connect_to(const fc::ip::endpoint& ep) {
			my->sock.close();
			my->sock.connect_to(my->ep = ep);
		}

		http::reply connection::request(const fc::string& method,
			const fc::string& url,
			const fc::string& body, const headers& he) {

			fc::url parsed_url(url);
			if (!my->sock.is_open()) {
				wlog("Re-open socket!");
				my->sock.connect_to(my->ep);
			}
			try {
				fc::stringstream req;
				req << method << " " << parsed_url.path()->generic_string() << " HTTP/1.1\r\n";
				req << "Host: " << *parsed_url.host() << "\r\n";
				req << "Content-Type: application/json\r\n";
				for (auto i = he.begin(); i != he.end(); ++i)
				{
					req << i->key << ": " << i->val << "\r\n";
				}
				if (body.size()) req << "Content-Length: " << body.size() << "\r\n";
				req << "\r\n";
				fc::string head = req.str();

				my->sock.write(head.c_str(), head.size());
				//  fc::cerr.write( head.c_str() );

				if (body.size()) {
					my->sock.write(body.c_str(), body.size());
					//      fc::cerr.write( body.c_str() );
				}
				//  fc::cerr.flush();

				return my->parse_reply();
			}
			catch (...) {
				my->sock.close();
				FC_THROW_EXCEPTION(exception, "Error Sending HTTP Request"); // TODO: provide more info
			 //  return http::reply( http::reply::InternalServerError ); // TODO: replace with connection error
			}
		}

		// used for servers
		fc::tcp_socket& connection::get_socket()const {
			return my->sock;
		}

		http::request    connection::read_request()const {
			http::request req;
			req.remote_endpoint = fc::variant(get_socket().remote_endpoint()).as_string();
			std::vector<char> line(1024 * 8);
			int s = my->read_until(line.data(), line.data() + line.size(), ' '); // METHOD
			req.method = line.data();
			s = my->read_until(line.data(), line.data() + line.size(), ' '); // PATH
			req.path = line.data();
			s = my->read_until(line.data(), line.data() + line.size(), '\n'); // HTTP/1.0

			while ((s = my->read_until(line.data(), line.data() + line.size(), '\n')) > 1) {
				fc::http::header h;
				char* end = line.data();
				while (*end != ':')++end;
				h.key = fc::string(line.data(), end);
				++end; // skip ':'
				++end; // skip space
				char* skey = end;
				while (*end != '\r') ++end;
				h.val = fc::string(skey, end);
				req.headers.push_back(h);
				if (boost::iequals(h.key, "Content-Length")) {
					auto s = static_cast<size_t>(to_uint64(fc::string(h.val)));
					FC_ASSERT(s < 1024 * 1024);
					req.body.resize(static_cast<size_t>(to_uint64(fc::string(h.val))));
				}
				if (boost::iequals(h.key, "Host")) {
					req.domain = h.val;
				}
			}
			// TODO: some common servers won't give a Content-Length, they'll use 
			// Transfer-Encoding: chunked.  handle that here.

			if (req.body.size()) {
				my->sock.read(req.body.data(), req.body.size());
			}
			return req;
		}

		fc::string request::get_header(const fc::string& key)const {
			for (auto itr = headers.begin(); itr != headers.end(); ++itr) {
				if (boost::iequals(itr->key, key)) { return itr->val; }
			}
			return fc::string();
		}
		std::vector<header> parse_urlencoded_params(const fc::string& f) {
			int num_args = 0;
			for (size_t i = 0; i < f.size(); ++i) {
				if (f[i] == '=') ++num_args;
			}
			std::vector<header> h(num_args);
			int arg = 0;
			for (size_t i = 0; i < f.size(); ++i) {
				while (f[i] != '=' && i < f.size()) {
					if (f[i] == '%') {
						h[arg].key += char((fc::from_hex(f[i + 1]) << 4) | fc::from_hex(f[i + 2]));
						i += 3;
					}
					else {
						h[arg].key += f[i];
						++i;
					}
				}
				++i;
				while (i < f.size() && f[i] != '&') {
					if (f[i] == '%') {
						h[arg].val += char((fc::from_hex(f[i + 1]) << 4) | fc::from_hex(f[i + 2]));
						i += 3;
					}
					else {
						h[arg].val += f[i] == '+' ? ' ' : f[i];
						++i;
					}
				}
				++arg;
			}
			return h;
		}

		connection_sync::connection_sync() : _socket(fc::asio::default_io_service()), _deadline(fc::asio::default_io_service()) {
		}

		connection_sync::~connection_sync() {
			close_socket();
		}

		int connection_sync::connect_to_servers(const std::vector<fc::ip::endpoint>& eps, std::vector<int>& res)
		{
			int ep_count = eps.size();
			res.resize(ep_count,0);
			for (int i = 0; i < ep_count; i++) {
				try {
					auto& ep = eps[i];
					boost::asio::ip::tcp::endpoint p(boost::asio::ip::address_v4(ep.get_address()), ep.port());
					_socket.close();
					_socket.connect(p);
					return i;
				}
				catch (...) {
					res[i]++;
				}
			}
			FC_THROW_EXCEPTION(exception, "Error Connecting HTTP Server.");
		}
		void connection_sync::connect_to(const fc::ip::endpoint& ep)
		{
			for (int i = 0; i < 3; i++) {
				try {
					boost::asio::ip::tcp::endpoint p(boost::asio::ip::address_v4(ep.get_address()), ep.port());
					_socket.close();
					_socket.connect(p);
					return;

				}
				catch (...) {
					std::cout << "retry connection to" << std::endl;
				}
			}
			try {
				boost::asio::ip::tcp::endpoint p(boost::asio::ip::address_v4(ep.get_address()), ep.port());
				_socket.close();
				_socket.connect(p);

			}
			catch (fc::exception& e) {
				FC_THROW_EXCEPTION(exception, "Error Connecting HTTP Server.");
			}

		}

		void connection_sync::handle_reply(const boost::system::error_code & error) {
			
			if (error&&error.value() != 2) {
				std::cout << "handle_reply error" << error.value() << error.message() << std::endl;
				return;
			}
			if (is_timeout)
				return;

			if (is_release)
				return;
			std::lock_guard<std::mutex> lk(read_lock);
			//is_timeout = false;
			is_done = true;

			if (!is_release) {
				is_release = true;
				_deadline.get_io_service().dispatch([&]() { try { _deadline.cancel(); }
				catch (...) {} });
			}
			m_cond.notify_all();

		}
		void connection_sync::handle_reply(const boost::system::error_code & error, size_t bytes_transferred) {
			if (is_timeout)
				return;
			if (error) {
				std::cout << "handle_reply error" << error.message() << std::endl;
				return;
			}
			std::lock_guard<std::mutex> lk(read_lock);
			//is_timeout = false;

			//std::cout << error.message() << "          " << bytes_transferred << std::endl;
			is_done = true;
			if (!is_release) {
				is_release = true;
				_deadline.get_io_service().dispatch([&]() { try { _deadline.cancel(); }
				catch (...) {} });
			}

			m_cond.notify_all();
		}
		http::reply connection_sync::request(const fc::string& method, const fc::string& url, const fc::string& body, const headers& he)
		{
			try {
				fc::url parsed_url(url);
				fc::stringstream req;
				req << method << " " << parsed_url.path()->generic_string() << " HTTP/1.1\r\n";
				req << "Host: " << *parsed_url.host() << "\r\n";
				req << "Content-Type: application/json\r\n";
				req << "Accept: */*\r\n";
				for (auto i = he.begin(); i != he.end(); ++i)
				{
					req << i->key << ": " << i->val << "\r\n";
				}
				boost::system::error_code ec;
				if (body.size()) req << "Content-Length: " << body.size() << "\r\n";
				req << "\r\n";
				fc::string head = req.str();

				_socket.write_some(boost::asio::buffer(head), ec);
				//  fc::cerr.write( head.c_str() );

				if (body.size()) {
					_socket.write_some(boost::asio::buffer(body), ec);
					//      fc::cerr.write( body.c_str() );
				}
				//  fc::cerr.flush();
				const auto& ret = parse_reply();
				return ret;
				//return parse_reply();
			}
			catch (...) {
				FC_THROW_EXCEPTION(exception, "Error Sending HTTP Request"); // TODO: provide more info
			}

		}



		void connection_sync::close_socket() {
			is_release = true;
			try {
				_socket.cancel();



			}
			catch (...) {

			}
			try {
				_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);

			}
			catch (...) {

			}

			try {

				_socket.close();




			}
			catch (...) {

			}

		}

		void connection_sync::check_deadline(const boost::system::error_code & error)
		{
			if (error) {
				//std::cout << "check_deadline error" << error.value() << "  " << error.message() << std::endl;
				return;
			}
			if (is_timeout || is_done || is_release)
				return;
			

			if (_deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now())
			{
				std::lock_guard<std::mutex> lk(read_lock);
				is_timeout = true;

				close_socket();

				boost::this_thread::sleep(boost::posix_time::seconds(3));

				//close_socket();

				m_cond.notify_all();

			}



		}


		http::reply connection_sync::parse_reply() {
			fc::http::reply rep;
			try {
				{
					std::unique_lock<std::mutex> lk(read_lock);
					_deadline.expires_from_now(boost::posix_time::seconds(50));
					_deadline.async_wait(boost::bind(&connection_sync::check_deadline, this, boost::asio::placeholders::error));
					boost::asio::async_read_until(_socket, line, "\r\n\r\n", boost::bind(&connection_sync::handle_reply, this, boost::asio::placeholders::error));
					while (!(is_done || is_timeout)) {
						auto now = std::chrono::system_clock::now();
						m_cond.wait_until(lk, now+ std::chrono::seconds(2));
						//m_cond.wait(lk);
					}
					if (is_timeout) {
						std::cout << "query timeout" << std::endl;
						rep.status = reply::status_code::InternalServerError;
						return rep;
					}
				}
				is_release = false;
				is_done = false;
				is_timeout = false;

				//line.consume(s);
				//s = boost::asio::read_until(_socket, line, ' '); // COD
				std::istream response_stream(&line);
				std::string http_version;
				response_stream >> http_version;
				unsigned int status_code;
				response_stream >> status_code;
				std::string status_message;
				std::getline(response_stream, status_message);
				rep.status = status_code;
				std::string head;
				uint64_t content_length = 0;
				while (std::getline(response_stream, head) && head != "\r")
				{
					auto pos = head.find(':');
					string key;
					key.assign(head.c_str(), pos);
					string val;
					val.assign(head.c_str(), pos + 1, std::string::npos);
					val.erase(0, val.find_first_not_of(" "));
					val.erase(val.find_last_not_of("\r") + 1);
					header h(key, val);
					if (boost::iequals(h.key, "Content-Length")) {
						rep.body.resize(static_cast<size_t>(to_uint64(fc::string(h.val))));
						content_length = to_uint64(fc::string(h.val));
					}
					///rep.headers.push_back();
					rep.headers.push_back(h);
				}
				content_length -= line.size();
				boost::system::error_code error;
				std::unique_lock<std::mutex> lk(read_lock);
				_deadline.expires_from_now(boost::posix_time::seconds(50));
				_deadline.async_wait(boost::bind(&connection_sync::check_deadline, this, boost::asio::placeholders::error));

				if (content_length > 0) {
					boost::asio::async_read(_socket, line, boost::asio::transfer_at_least(content_length), boost::bind(&connection_sync::handle_reply, this, boost::asio::placeholders::error));
					//, boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred
				}

				while (!(is_done || is_timeout) && content_length>0) {
					auto now = std::chrono::system_clock::now();
					m_cond.wait_until(lk, now + std::chrono::seconds(2));
				}
				if (is_timeout) {
					std::cout << "query timeout" << std::endl;
					rep.status = reply::status_code::InternalServerError;
					return rep;
				}
				if (line.size())
				{
					std::istream response_stream1(&line);
					std::istreambuf_iterator<char> eos;
					auto reponse_data = string(std::istreambuf_iterator<char>(response_stream1), eos);
					rep.body.assign(reponse_data.begin(), reponse_data.end());
				}
				return rep;
				/*
				//response_stream>> status ;
				line.consume(s);
				rep.status = static_cast<int>(to_int64("200"));
				std::cout << rep.status << std::endl;
				s = boost::asio::read_until(_socket, line, '\n'); // DESCRIPTION
				line.consume(s);
				while ((s = boost::asio::read_until(_socket, line, '\n')) > 1) {
					fc::http::header h;
					string line_str;
					std::istream line_stream(&line);
					line_stream >> line_str;
					std::cout << line_str << std::endl;
					line.consume(s);
					const char* begin = line_str.c_str();
					const char* end = begin;
					while (*end != ':')++end;
					h.key = fc::string(begin, end);
					++end; // skip ':'
					++end; // skip space
					const char* skey = end;
					while (*end != '\r') ++end;
					h.val = fc::string(skey, end);
					rep.headers.push_back(h);
					if (boost::iequals(h.key, "Content-Length")) {
						rep.body.resize(static_cast<size_t>(to_uint64(fc::string(h.val))));
					}
				}
				if (rep.body.size()) {
					//sock.read(rep.body.data(), rep.body.size());
					_socket.read_some(boost::asio::buffer( rep.body.data(), rep.body.size()));
				}
				return rep;
				*/
			}
			catch (std::exception& ex) {
				std::cout << ex.what() << std::endl;
			}
			catch (fc::exception& e) {
				elog("${exception}", ("exception", e.to_detail_string()));
				is_done = true;
				is_timeout = true;
				if (!is_release) {
					is_release = true;
					_deadline.get_io_service().post([&]() {try { _deadline.cancel(); }
					catch (...) {} });
				}



				rep.status = http::reply::InternalServerError;
				return rep;
			}
		}
	} // fc::http
}
