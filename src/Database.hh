
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

		// Retrieve a list of all photos in an event.
		class Photo {
			public:
				std::string filename;
		};
		std::vector<Photo> get_photos(int event_id);

	private:
		sqlite3 *db;
};
