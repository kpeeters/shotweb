
#include <snoop/Snoop.hh>
#include "Server.hh"

int main(int argc, char **argv)
	{
	snoop::log.init("Shotweb", "1.00", "/tmp/shotweb.sql", "localhost:8083");

	try {
		Server server;
		server.start();
		}
	catch(std::exception& ex) {
		snoop::log(snoop::error) << "Terminated with exception: " << ex.what() << snoop::flush;		
		snoop::log.sync_with_server();
		}
	}
