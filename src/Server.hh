
#pragma once

class Server {
	public:
		static void run();

	private:
		static int event_handler(struct mg_connection *conn, enum mg_event ev);
};
