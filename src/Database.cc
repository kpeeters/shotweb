
#include "Database.hh"
#include <stdexcept>
#include <iostream>
#include <sstream>

Database::Database(const std::string& name)
	: db(0)
	{
	int ret = sqlite3_open_v2(name.c_str(), &db, SQLITE_OPEN_READONLY, NULL);
	if(ret) 
		throw std::logic_error("Cannot open database");
	}

Database::~Database()
	{
	if(db!=0) {
		sqlite3_close(db);
		db=0;
		}
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

	ss << "select distinct EventTable.id,name,EventTable.comment,EventTable.primary_source_id from EventTable";
	ss << " join PhotoTable on (EventTable.id=event_id) ";
	if(event_id>0) {
		ss << " where EventTable.id='" << event_id << "'";
		}
	ss << " order by PhotoTable.timestamp desc";
	std::string query = ss.str();
	// std::cout << query << std::endl;
	int ret = sqlite3_prepare_v2(db, query.c_str(), -1, &statement, NULL);
	if(ret!=SQLITE_OK) {
		throw std::logic_error("Error preparing query");
		}
	while(true) {
		ret = sqlite3_step(statement);
		switch(ret) {
			case SQLITE_ROW: {
				Event event;
				if(sqlite3_column_type(statement, 1)==SQLITE_TEXT) {
					const unsigned char *name = sqlite3_column_text(statement, 1);
					event.name=reinterpret_cast<const char*>(name);
					}
				int id = sqlite3_column_int(statement, 0);
				event.id=id;
				if(sqlite3_column_type(statement, 3)==SQLITE_TEXT) {
					const unsigned char *filename = sqlite3_column_text(statement, 3);
					std::string primary_source_id=reinterpret_cast<const char*>(filename);
					size_t numpos = primary_source_id.find("0");
					event.cover_photo_id = std::stoul(primary_source_id.substr(numpos), 0, 16);
					}
				results.push_back(event);
				break;
				}
			case SQLITE_DONE:
				sqlite3_finalize(statement);
				return results;
			case SQLITE_ERROR:
			case SQLITE_MISUSE:
			case SQLITE_BUSY: {
				int errcode = sqlite3_errcode(db);
				std::string errmsg = sqlite3_errstr(errcode);
				sqlite3_finalize(statement);				
				throw std::logic_error(errmsg);
				}
			default:
				throw std::logic_error("Failed to execute query");
			}
		} 

	return results;
	}

Database::Photo Database::get_photo(int photo_id) 
	{
	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	ss << "select id,filename,orientation,event_id from PhotoTable where id='" << photo_id << "'";
	std::string query = ss.str();
	// std::cout << query << std::endl;
	int ret = sqlite3_prepare_v2(db, query.c_str(), -1, &statement, NULL);
	if(ret!=SQLITE_OK) {
		throw std::logic_error("Error preparing query");
		}
	Photo photo;
	while(true) {
		ret = sqlite3_step(statement);
		switch(ret) {
			case SQLITE_ROW: {
				photo.id=sqlite3_column_int(statement, 0);
				if(sqlite3_column_type(statement, 1)==SQLITE_TEXT) {
					const unsigned char *name = sqlite3_column_text(statement, 1);
					photo.filename=reinterpret_cast<const char*>(name);
					}
				photo.orientation=sqlite3_column_int(statement, 2);
				photo.event_id=sqlite3_column_int(statement, 3);
				break;
				}
			case SQLITE_DONE:
				sqlite3_finalize(statement);
				return photo;
			case SQLITE_ERROR:
			case SQLITE_MISUSE:
			case SQLITE_BUSY: {
				int errcode = sqlite3_errcode(db);
				std::string errmsg = sqlite3_errstr(errcode);
				sqlite3_finalize(statement);
				throw std::logic_error(errmsg);
				}
			default:
				throw std::logic_error("Failed to execute query");
			}
		} 
	}


std::vector<Database::Photo> Database::get_photos(int event_id)
	{
	std::vector<Database::Photo> results;

	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	ss << "select id,filename,orientation,event_id from PhotoTable where event_id='" << event_id << "'";
	std::string query = ss.str();
	// std::cout << query << std::endl;
	int ret = sqlite3_prepare_v2(db, query.c_str(), -1, &statement, NULL);
	if(ret!=SQLITE_OK) {
		throw std::logic_error("Error preparing query");
		}
	while(true) {
		ret = sqlite3_step(statement);
		switch(ret) {
			case SQLITE_ROW: {
				Photo photo;
				photo.id=sqlite3_column_int(statement, 0);
				if(sqlite3_column_type(statement, 1)==SQLITE_TEXT) {
					const unsigned char *name = sqlite3_column_text(statement, 1);
					photo.filename=reinterpret_cast<const char*>(name);
					}
				photo.orientation=sqlite3_column_int(statement, 2);
				photo.event_id=sqlite3_column_int(statement, 3);
				results.push_back(photo);
				break;
				}
			case SQLITE_DONE:
				sqlite3_finalize(statement);
				return results;
			case SQLITE_ERROR:
			case SQLITE_MISUSE:
			case SQLITE_BUSY: {
				int errcode = sqlite3_errcode(db);
				std::string errmsg = sqlite3_errstr(errcode);
				sqlite3_finalize(statement);
				throw std::logic_error(errmsg);
				}
			default:
				throw std::logic_error("Failed to execute query");
			}
		} 

	return results;
	}


