
#include <snoop/Snoop.hh>
#include "Server.hh"

int main(int argc, char **argv)
	{
		snoop::log.init("Shotweb", "1.01", "localhost:8083", "/home/kasper/snoop/shotweb.sql");
	snoop::log.set_sync_immediately(true);

	try {
		Server server;
		server.start();
		}
	catch(std::exception& ex) {
		snoop::log(snoop::error) << "Terminated with exception: " << ex.what() << snoop::flush;		
		snoop::log.sync_with_server();
		}
	snoop::log(snoop::warn) << "Exiting" << snoop::flush;
	snoop::log.sync_with_server();
	sleep(2);
	}
