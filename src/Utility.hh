#pragma once

#include <string>

namespace httplib {
	class Request;
}

bool replace(std::string& str, const std::string& from, const std::string& to);
std::string logstamp(const httplib::Request *request=0);
