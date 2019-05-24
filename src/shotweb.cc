
//#include <snoop/Snoop.hh>
#include "Server.hh"
#include <iostream>

int main(int argc, char **argv)
	{
	if(argc<5 || (argc>5 && argc!=7)) {
		std::cerr << "Usage: shotweb [shotwell database file] [auth database] [html/css/js directory] [port] [oldroot (optional)] [newroot (optional)]" << std::endl;
		return -1;
		}

	std::string dbpath(argv[1]);
	std::string authpath(argv[2]);
	std::string htmlpath(argv[3]);
	int port(atoi(argv[4]));

	std::string oldroot, newroot;
	
	std::cerr << "shotweb starting, db path = " << dbpath << ", auth db path = " << authpath << ", html path = " << htmlpath << ", port = " << port << std::endl;

	if(argc>5) {
		oldroot=argv[5];
		newroot=argv[6];
		}
	
//		snoop::log.init("Shotweb", "1.01", "localhost:8083", "/home/kasper/snoop/shotweb.sql");
//	snoop::log.set_sync_immediately(true);

	try {
		Server server(dbpath, authpath, htmlpath, port, oldroot, newroot);
		server.start();
		}
	catch(std::exception& ex) {
//		snoop::log(snoop::error) << "Terminated with exception: " << ex.what() << snoop::flush;		
//		snoop::log.sync_with_server();
		}
//	snoop::log(snoop::warn) << "Exiting" << snoop::flush;
//	snoop::log.sync_with_server();
	sleep(2);
	}
