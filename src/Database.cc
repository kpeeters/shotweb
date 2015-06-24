
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

std::vector<Database::Event> Database::get_events()
	{
	std::vector<Database::Event> results;

	sqlite3_stmt *statement=0;
	std::string query = "select id,name,comment from EventTable";
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


std::vector<Database::Photo> Database::get_photos(int event_id)
	{
	std::vector<Database::Photo> results;

	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	ss << "select filename from PhotoTable where event_id='" << event_id << "'";
	std::string query = ss.str();
	std::cout << query << std::endl;
	int ret = sqlite3_prepare_v2(db, query.c_str(), -1, &statement, NULL);
	if(ret!=SQLITE_OK) {
		throw std::logic_error("Error preparing query");
		}
	while(true) {
		ret = sqlite3_step(statement);
		switch(ret) {
			case SQLITE_ROW: {
				Photo photo;
				if(sqlite3_column_type(statement, 0)==SQLITE_TEXT) {
					const unsigned char *name = sqlite3_column_text(statement, 0);
					photo.filename=reinterpret_cast<const char*>(name);
					}
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


