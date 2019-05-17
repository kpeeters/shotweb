
//#include <snoop/Snoop.hh>
#include "Server.hh"
#include <iostream>

int main(int argc, char **argv)
	{
	if(argc<4 || (argc>4 && argc!=6)) {
		std::cerr << "Usage: shotweb [shotwell database file] [html/css/js directory] [port] [oldroot (optional)] [newroot (optional)]" << std::endl;
		return -1;
		}

	std::string dbpath(argv[1]);
	std::string htmlpath(argv[2]);
	int port(atoi(argv[3]));

	std::string oldroot, newroot;
	
	std::cerr << "shotweb starting, db path = " << dbpath << ", html path = " << htmlpath << ", port = " << port << std::endl;

	if(argc>4) {
		oldroot=argv[4];
		newroot=argv[5];
		}
	
//		snoop::log.init("Shotweb", "1.01", "localhost:8083", "/home/kasper/snoop/shotweb.sql");
//	snoop::log.set_sync_immediately(true);

	try {
		Server server(dbpath, htmlpath, port, oldroot, newroot);
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
