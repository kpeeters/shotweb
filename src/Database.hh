
// C++ interface to Shotwell's photo database stored in an SQLite file.
// This hides all SQL logic and table structure behind type-safe
// function calls. We only ever open databases in read-only mode,
// this stuff is supposed to face the world and we do not want anyone
// messing with our photos.

#pragma once

#include <string>
#include <vector>
#include <sqlite3.h>

class Database {
	public:
		Database(const std::string& db);
		~Database();
		
		// Retrieve a list of all events.
		class Event {
			public:
				int         id;
				std::string name;
				std::string comment;
		};
		std::vector<Event> get_events();

		// The following set filters on the indicated event.
		void select_event(int event_num);

	private:
		sqlite3 *db;
};
