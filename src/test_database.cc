
#include "Database.hh"
#include <iostream>

int main(int argc, char **argv)
	{
	Database db("photo.db");

	std::vector<Database::Event> events = db.get_events();
	auto it=events.begin();
	while(it!=events.end()) {
		std::cout << (*it).id << "\t" << (*it).name << std::endl;
		++it;
		}
	}
