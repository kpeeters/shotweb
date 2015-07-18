
// Http server to server Shotwell photo galleries.

#pragma once

#include "server_http.hpp"
#include "Database.hh"

namespace sw = SimpleWeb;

typedef sw::Server<sw::HTTP> HttpServer;

class Server {
	public:
		Server();

		void start();

	private:
		HttpServer server;
		
		void handle_json(HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request);
		void handle_default(HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request);
		void send_file(HttpServer::Response& response, const std::string& fn) const;
		void send_thumbnail(HttpServer::Response& response, const Database::Photo& photo, int max_size) const;
		void send_photo(HttpServer::Response& response, const Database::Photo&) const;
		bool file_exists(const std::string&) const;

		// Data structures for user -> event(s) authorisation mapping. These will 
		// eventually come from a database but we have them hardcoded right now
		// to get something up and running quickly.

		class Authorisation {
			public:
				std::string      name;
				std::string      password;
				bool             root;
				std::vector<int> events;
		};
		typedef std::string Token;

		std::vector<Authorisation>     users;
		std::map<Token, Authorisation> authorisations;

		// Initialise the authorisations map from a disk database.
		void        init_authorisations();

		// Validate a user, return an access token if valid.
		std::string validate_user(const std::string& user, const std::string& passwd);

		// Determine if access to the given event is allowed with the given authorisation token.
		bool        access_allowed(int event_id, const std::string& token) const;
		
		// Extract the authorisation token from the HTTP request header, if any.
		std::string extract_token(std::shared_ptr<HttpServer::Request> request);

		// Send an access denied page and log the request.
		void        denied(HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request);

		// Create a thumbnail file of the given photo and store at the given location on disk.
		void        create_thumbnail(const Database::Photo& photo, const std::string& loc, int max_size) const;

		Database db;
		std::vector<Database::Event> events;

		// Path to the html, css and javascrpipt files.
		std::string htmlpath;

		// Path to the thumbnail cache.
		std::string cachepath;
};
