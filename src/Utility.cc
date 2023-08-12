
#include "Utility.hh"
#include <ostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include "httplib.h"

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

std::string logstamp(const httplib::Request *request) 
	{
	static std::mutex mx;

	std::lock_guard<std::mutex> lock(mx);
	
	std::ostringstream str;
	std::time_t time_now = std::time(nullptr);
	str << std::put_time(std::localtime(&time_now), "%y-%m-%d %OH:%OM:%OS") << ", " << std::this_thread::get_id() << ", ";
	if(request!=0)
		str << std::setw(16) << request->get_header_value("REMOTE_ADDR") << ", " << std::setw(16) << request->get_header_value("X-Forwarded-For") << ": ";
	else
		str << std::setw(16) << "0.0.0.0" << ", " << std::setw(16) << " " << ": ";
	return str.str();
	}

