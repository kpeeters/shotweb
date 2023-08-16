
#include "Database.hh"
#include "Utility.hh"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <sys/inotify.h>

Database::Database(const std::string& name_, const std::string& oldroot, const std::string& newroot)
  : name(name_), oldroot(oldroot), newroot(newroot)
	{
	config.flags=sqlite::OpenFlags::READONLY;

	// FIXME: when we find a database modification through the file system
	// notificator, re-create the database object so it closes/reopens the
	// database and invalidates caches.
	open_or_reopen();

	// Setup an inotify watch to monitor changes to the database, so that
	// we can re-open it.
	inotfd = inotify_init();

	// size_t bufsiz = sizeof(struct inotify_event) + PATH_MAX + 1;
	// struct inotify_event* event = malloc(bufsiz);
	
	}

Database::~Database()
	{
	  
	}

void Database::open_or_reopen()
{
  std::lock_guard<std::mutex> lock(db_mutex);
  // std::cerr << logstamp(0) << "Database::open_or_reopen" << std::endl;
  db = std::make_unique<sqlite::database>(name, config);
}

bool Database::watch_for_changes()
{
// std::cerr << "Database::Database: adding inotify for " << name << std::endl;
  watch_desc = inotify_add_watch(inotfd, name.c_str(), IN_MODIFY | IN_MOVE_SELF | IN_ATTRIB | IN_CREATE | IN_DELETE);
  if(watch_desc==-1) {
    throw std::logic_error("Database::Database: cannot watch "+name+", does it exist?");
  }
  
  size_t bufsiz = 1024;
  char buf[bufsiz];

  int i=0;
  int len=0;
  while(len==0) {
	  // std::cerr << "Database::watch_for_changes: read" << std::endl;
    len = read(inotfd, buf, bufsiz);
    // std::cerr << "Database::watch_for_changes: event size " << len << std::endl;
    if(len>0) {
      while(i<len) {
	struct inotify_event *event = (struct inotify_event *) &buf[i];
	
	if ((event->mask & IN_CREATE) != 0) {
	  std::cerr << "Event: create" << std::endl;
	}
	else if ((event->mask & IN_DELETE) != 0) {
	  std::cerr << "Event: delete" << std::endl;
	}
	else if ((event->mask & IN_MODIFY) != 0) {
	std::cerr << "Event: modify" << std::endl;
	}
	else if ((event->mask & IN_MOVE_SELF) != 0) {
	  std::cerr << "Event: move_self" << std::endl;
	}
	else if ((event->mask & IN_ATTRIB) != 0) {
	  std::cerr << "Event: attrib" << std::endl;
	}
	else {
	  std::cerr << "Unknown event: " << event->mask << std::endl;
	}
	
	i += sizeof (struct inotify_event) + event->len;
      }
      // It looks like we do not have to remove the watch, and we can also
      // safely add it again.
      // https://stackoverflow.com/questions/13409843/inotify-vim-modification
      
      //if(inotify_rm_watch(inotfd, watch_desc)==-1)
      //throw std::logic_error("Database::watch_for_changes: cannot unregister watch.");
      
      return true;
    }
  }
  return false;
}

Database::Event Database::get_event(int event_id) 
	{
	auto events=get_events(event_id);
	if(events.size()>0) return events[0];
	else throw std::range_error("Event id "+std::to_string(event_id)+" does not exist");
	}

std::vector<Database::Event> Database::get_events(int event_id)
	{
	std::lock_guard<std::mutex> lock(db_mutex);
	
	std::vector<Database::Event> results;

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
				// The `primary_source_id` can be a thumbnail name, or in the case of a video
				// it will be a name of the form `video-....`.
				if(primary_source_id.substr(0, 5)=="thumb") event.cover_is_photo=true;
				else                                        event.cover_is_photo=false;
				results.push_back(event);
		};

	// Now give these a first date & last date timestamp.
	for(auto& event: results) {
		query = "select min(exposure_time), max(exposure_time) from PhotoTable where event_id='"+std::to_string(event.id)+"'";
		(*db) << query
			>> [&](uint32_t start, uint32_t end) {
			event.start_timestamp=start;
			event.end_timestamp=end;
			};
		}

	// Sort events by starting date.
	std::sort(results.begin(), results.end(),
				 [&](const auto& e1, const auto& e2) {
					 return e1.end_timestamp > e2.end_timestamp;
					 }
				 );
	
	return results;
	}

Database::Photo Database::get_photo(int photo_id) 
	{
	std::lock_guard<std::mutex> lock(db_mutex);

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
		photo.is_video=false;		
		photo.event_id=event_id;
		};

	return photo;
	}

Database::Photo Database::get_video(int video_id) 
	{
	std::lock_guard<std::mutex> lock(db_mutex);

	std::ostringstream ss;
	ss << "select id,filename,event_id from VideoTable where id='" << video_id << "'";
	std::string query = ss.str();
	Database::Photo photo;
	
	(*db) << query
		>> [&](int id, std::string filename, int event_id) {
		photo.id=id;
		photo.filename=filename;
		replace(photo.filename, oldroot, newroot);
		photo.orientation=1;
		photo.is_video=true;
		photo.event_id=event_id;
		};

	return photo;
	}


std::vector<Database::Photo> Database::get_photos(int event_id)
	{
	std::lock_guard<std::mutex> lock(db_mutex);

	std::vector<Database::Photo> results;

	std::ostringstream ss;
	ss << "select id,filename,orientation,event_id from PhotoTable where event_id='" << event_id << "' "
		<< "order by timestamp, time_created";
	std::string query = ss.str();

	(*db) << query
		>> [&](int id, std::string filename, int orientation, int event_id) {
		Photo photo;
		photo.id=id;
		photo.filename=filename;
		replace(photo.filename, oldroot, newroot);		
		photo.orientation=orientation;
		photo.event_id=event_id;
		photo.is_video=false;		
		results.push_back(photo);
		};

	ss.str("");
	ss << "select id,filename,event_id from VideoTable where event_id='" << event_id << "' "
		<< "order by timestamp, time_created";
	query = ss.str();
	std::cerr << query << std::endl;
	(*db) << query
		>> [&](int id, std::string filename, int event_id) {
		Photo photo;
		photo.id=id;
		photo.filename=filename;
		replace(photo.filename, oldroot, newroot);		
		photo.orientation=1;
		photo.event_id=event_id;
		photo.is_video=true;
		results.push_back(photo);
		};
	

	return results;
	}


