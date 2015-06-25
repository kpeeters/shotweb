
// Http server to server Shotwell photo galleries.

#pragma once

#include "server_http.hpp"

namespace sw = SimpleWeb;

class Server {
	public:
		Server();

		void start();

	private:
		sw::Server<sw::HTTP> server;
		
		void handle_authenticate(std::ostream& response, std::shared_ptr<sw::Request> request);
		void handle_default(std::ostream& response, std::shared_ptr<sw::Request> request);
};
