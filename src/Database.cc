
#include "Database.hh"
#include <stdexcept>
#include <iostream>
#include <sstream>

Database::Database(const std::string& name, const std::string& oldroot, const std::string& newroot)
	: oldroot(oldroot), newroot(newroot)
	{
	config.flags=sqlite::OpenFlags::READONLY;
	db = std::make_unique<sqlite::database>(name, config);	
	}

Database::~Database()
	{
	}

Database::Event Database::get_event(int event_id) 
	{
	auto events=get_events(event_id);
	if(events.size()>0) return events[0];
	else throw std::range_error("Event id "+std::to_string(event_id)+" does not exist");
	}

std::vector<Database::Event> Database::get_events(int event_id)
	{
	std::vector<Database::Event> results;

	sqlite3_stmt *statement=0;
	std::ostringstream ss;

	// The following query produces the correct list of event thumbnails, but they are not
	// yet sorted in the order of the first photo in the event. Rather, they are sorted in
	// the order of which the events were created, which is rather arbitrary.

	std::string query =
		"select distinct EventTable.id, name, EventTable.comment, EventTable.primary_source_id from EventTable "
		"join PhotoTable on (EventTable.id=event_id) ";
	if(event_id>0)
		query += "where EventTable.id='"+std::to_string(event_id)+"'";
	query+=" order by PhotoTable.timestamp desc";

	(*db) << query
		>> [&](int id, std::string name, std::string comment, std::string primary_source_id) {
				Event event;
				event.id=id;
				event.name=name;
				event.comment=comment;
				size_t numpos = primary_source_id.find("0");
				event.cover_photo_id = std::stoul(primary_source_id.substr(numpos), 0, 16);
				results.push_back(event);
		};

	return results;
	}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

Database::Photo Database::get_photo(int photo_id) 
	{
	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	ss << "select id,filename,orientation,event_id from PhotoTable where id='" << photo_id << "'";
	std::string query = ss.str();
	Database::Photo photo;
	
	(*db) << query
		>> [&](int id, std::string filename, int orientation, int event_id) {
		photo.id=id;
		photo.filename=filename;
		replace(photo.filename, oldroot, newroot);
		photo.orientation=orientation;
		photo.event_id=event_id;
		};

	return photo;
	}


std::vector<Database::Photo> Database::get_photos(int event_id)
	{
	std::vector<Database::Photo> results;

	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	ss << "select id,filename,orientation,event_id from PhotoTable where event_id='" << event_id << "' "
		<< "order by time_created, timestamp";
	std::string query = ss.str();

	(*db) << query
		>> [&](int id, std::string filename, int orientation, int event_id) {
		Photo photo;
		photo.id=id;
		photo.filename=filename;
		replace(photo.filename, oldroot, newroot);		
		photo.orientation=orientation;
		photo.event_id=event_id;
		results.push_back(photo);
		};

	return results;
	}


