
// Http server to server Shotwell photo galleries.

#pragma once

// https://github.com/yhirose/cpp-httplib
#include "httplib.h"

// https://github.com/nlohmann/json
#include "json.hpp"

#include "Database.hh"

#include <mutex>

class Server : public httplib::Server {
	public:
		Server(const std::string& db_path, const std::string& authdbpath, const std::string& htmlpath, int port,
				 const std::string& oldroot, const std::string& newroot);
		virtual ~Server();

		void start();

	private:
		void handle_json(const httplib::Request& request, httplib::Response& response);
		void handle_default(const httplib::Request& request, httplib::Response& response);
		void send_file(httplib::Response& response, const std::string& fn) const;
		void send_thumbnail(httplib::Response& response, const Database::Photo& photo, int max_size) const;
		void send_photo(httplib::Response& response, const Database::Photo&) const;
		void send_video(httplib::Response& response, const Database::Photo&) const;		
		bool file_exists(const std::string&) const;

		// Data structures for user -> event(s) authorisation mapping. These will 
		// eventually come from a database but we have them hardcoded right now
		// to get something up and running quickly.

		class Authorisation {
			public:
				int              id;
				std::string      name;
				std::string      password;
				bool             root;
				std::vector<int> events;
		};
		typedef std::string Token;

		// Information mapping a user ID to Authorisation data.
		std::map<int,   Authorisation> users;

		// Information mapping a UUID token to a user ID.
		std::map<Token, int>           authorisations;

		// Information mapping a UUID token to an event (for passwordless sharing).
		std::map<Token, int>           share_links;

		// Initialise the authorisations map from a disk database.
		void        init_authorisations();

		// Register a new user.
		void        register_user(const std::string& user, const std::string& passwd, bool admin);

		// Register a new token for sharing an event without password.
		Token       register_share_link(int event_id);
			
		// Validate a user, return an access token if valid.
		std::string validate_user(const std::string& user, const std::string& passwd);

		// Determine if access to the given event is allowed with the given authorisation token.
		bool        access_allowed(int event_id, const std::string& token) const;
		
		// Extract the authorisation token from the HTTP request header, if any.
		std::string extract_token(const httplib::Request& request);

		// Send an access denied page and log the request.
		void        denied(const httplib::Request& request, httplib::Response& response);

		// Create a thumbnail file of the given photo and store at the given location on disk.
		void        create_thumbnail(const Database::Photo& photo, const std::string& loc) const;

		Database db;
//		std::vector<Database::Event> events;

		// Path to the auth.db authorisation database file.
		std::string authdbpath;
		
		// Path to the html, css and javascrpipt files.
		std::string htmlpath;

		// Path to the thumbnail cache.
		std::string cachepath;

		// Port on which to listen.
		int port;

		mutable std::mutex video_mutex;

		class StreamHandler {
			public:
				StreamHandler(std::string);

				static const uint64_t chunk_size=32*1024;

				std::string    filename;
				std::ifstream  vf;
				bool           finished;
				char           data[chunk_size];
		};
		std::map<std::function<std::string(uint64_t)>, StreamHandler> stream_handlers;

	private:
		std::string logstamp(const httplib::Request&) const;
};
