
// C++ interface to Shotwell's photo database stored in an SQLite file.
// This hides all SQL logic and table structure behind type-safe
// function calls. We only ever open databases in read-only mode,
// this stuff is supposed to face the world and we do not want anyone
// messing with our photos.

#pragma once

#include <string>
#include <vector>
#include <sqlite_modern_cpp.h>

class Database {
	public:
		Database(const std::string& db, const std::string& oldroot, const std::string& newroot);
		~Database();
		
		// Retrieve a list of all events.
		class Event {
			public:
				int         id;
				std::string name;
				std::string primary_photo_filename;
				int         cover_photo_id;
				std::string comment;
				uint32_t    start_timestamp;
				uint32_t    end_timestamp;
		};
		std::vector<Event> get_events(int event_id=-1);
		Event              get_event(int event_id);

		// Retrieve a list of all photos in an event.
		class Photo {
			public:
				int         id;
				std::string filename;
				int         orientation;
				int         event_id;
				bool        is_video;
		};
		std::vector<Photo> get_photos(int event_id);
		Photo              get_photo(int photo_id);
		Photo              get_video(int video_id);

	private:
		sqlite::sqlite_config             config;
		std::unique_ptr<sqlite::database> db;

		std::string oldroot, newroot;
};

