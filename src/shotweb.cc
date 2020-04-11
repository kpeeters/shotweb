
//#include <snoop/Snoop.hh>
#include "Server.hh"
#include <iostream>
#include <iomanip>

std::string logstamp()
	{
	std::ostringstream str;
	std::time_t time_now = std::time(nullptr);
	str << std::put_time(std::localtime(&time_now), "%y-%m-%d %OH:%OM:%OS") << ", ";
	str << "0.0.0.0: ";
	return str.str();
	}

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
