
#include "server_http.hpp"

namespace sw = SimpleWeb;

int main(int argc, char **argv)
	{
	sw::Server<sw::HTTP> server(8080, 3);

	server.default_resource.type = sw::CallbackType::GET;

	server.default_resource.functions[static_cast<size_t>(sw::CallbackType::GET)] 
		= [](std::ostream& response, std::shared_ptr<sw::Request> request) 
		{
			std::cout << request->path << std::endl;
			
			std::ostringstream ss;
			ss << "<html><body>hello</body></html>";
			std::string content=ss.str();

			response << "HTTP/1.1 200 OK\r\nContent-Length: " 
			<< content.length() << "\r\n\r\n" << content;
		};



	server.start();
	}
