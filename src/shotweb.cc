
//#include <snoop/Snoop.hh>
#include "Server.hh"
#include "Utility.hh"
#include <iostream>
#include <iomanip>

int main(int argc, char **argv)
	{
	if(argc!=2) {
		std::cerr << "Usage: shotweb [configuration-file]" << std::endl;
		return -1;
		}

	std::string config_path(argv[1]);
	std::cerr << logstamp() << "shotweb starting, configuration file = " << config_path << std::endl;

	try {
		std::ifstream i(config_path);
		nlohmann::json cj;
		i >> cj;

		Server server(cj);
		server.start();
		}
	catch(std::exception& ex) {
		std::cerr << logstamp() << "shotweb failure: " << ex.what() << std::endl;;
		}
	sleep(2);
	}
