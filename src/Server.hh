
// Http server to server Shotwell photo galleries.

#pragma once

#include "server_http.hpp"
#include "Database.hh"

namespace sw = SimpleWeb;

class Server {
	public:
		Server();

		void start();

	private:
		sw::Server<sw::HTTP> server;
		
		void handle_json(std::ostream& response, std::shared_ptr<sw::Request> request);
		void handle_default(std::ostream& response, std::shared_ptr<sw::Request> request);
		void send_file(std::ostream& response, const std::string& fn) const;
		void send_thumbnail(std::ostream& response, const std::string& fn, int max_size) const;
		void send_photo(std::ostream& response, const Database::Photo&) const;

		// Data structures for user -> event(s) authorisation mapping. These will 
		// eventually come from a database but we have them hardcoded right now
		// to get something up and running quickly.

		class Authorisation {
			public:
				std::string      name;
				std::string      password;
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
		std::string extract_token(std::shared_ptr<sw::Request> request);

		Database db;
		std::vector<Database::Event> events;
};
