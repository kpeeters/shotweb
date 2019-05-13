
#include "Database.hh"
#include <iostream>

int main(int argc, char **argv)
	{
	Database db("photo.db", "", "");

	auto events = db.get_events();
	auto it=events.begin();
	while(it!=events.end()) {
		std::cout << (*it).id << "\t" << (*it).name << std::endl;
		auto photos = db.get_photos(it->id);
		auto it2 = photos.begin();
		while(it2!=photos.end()) {
			std::cout << "  " << it2->filename << std::endl;
			++it2;
			}
		++it;
		}
	}
