
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

		Database db;
		std::vector<Database::Event> events;
};
