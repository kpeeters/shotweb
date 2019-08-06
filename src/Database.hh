
// C++ interface to Shotwell's photo database stored in an SQLite file.
// This hides all SQL logic and table structure behind type-safe
// function calls. We only ever open databases in read-only mode,
// this stuff is supposed to face the world and we do not want anyone
// messing with our photos.

#pragma once

#include <string>
#include <vector>
#include <sqlite_modern_cpp.h>

/// Class wrapping a Shotwell photo database, hiding all SQL
/// implementation details and allowing for simple retrieval of events
/// or single photo/video entries.

class Database {
	public:
		Database(const std::string& db, const std::string& oldroot, const std::string& newroot);
		~Database();

		/// Descriptor for a shotwell event.
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
		/// Retrieve a list of all events.
		std::vector<Event> get_events(int event_id=-1);

		/// Retrieve a single event given its id.
		Event              get_event(int event_id);

		/// Descriptor for a photo or video entry (not the actual photo
		/// or video).
		class Photo {
			public:
				int         id;
				std::string filename;
				int         orientation;
				int         event_id;
				bool        is_video;
		};
		/// Retrieve a list of all photos in an event.
		std::vector<Photo> get_photos(int event_id);

		/// Retrieve a single photo descriptor given its id.
		Photo              get_photo(int photo_id);

		/// Retrieve a single video descriptor given its id.
		Photo              get_video(int video_id);

	private:
		sqlite::sqlite_config             config;
		std::unique_ptr<sqlite::database> db;

		std::string oldroot, newroot;
};

