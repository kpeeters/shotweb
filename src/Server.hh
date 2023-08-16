
// Http server to server Shotwell photo galleries.

#pragma once

#include "ThreadStream.hh"

// https://github.com/yhirose/cpp-httplib
#include "httplib.h"

// https://github.com/nlohmann/json
#include "json.hpp"

#include "Database.hh"

#include <mutex>

namespace cv {
	class Mat;
};

class Server : public httplib::Server {
	public:
		Server(const nlohmann::json& config);
		virtual ~Server();

		void start();

	private:
		void handle_json(const httplib::Request& request, httplib::Response& response);
		void handle_default(const httplib::Request& request, httplib::Response& response);
		void send_file(const httplib::Request& request, httplib::Response& response, const std::string& fn) const;
		void send_thumbnail(const httplib::Request& request, httplib::Response& response, const Database::Photo& photo, int max_size) const;
		void send_photo(const httplib::Request& request, httplib::Response& response, const Database::Photo&) const;
		void send_video(const httplib::Request& request, httplib::Response& response, const Database::Photo&,
							 const std::string& vtype) const;		
		bool file_exists(const std::string&) const;

		// Data structures for user -> event(s) authorisation mapping. These will 
		// eventually come from a database but we have them hardcoded right now
		// to get something up and running quickly.

		class Authorisation {
			public:
				int              id;
				std::string      name;
				std::string      password;
				bool             root;
				std::vector<int> events;
		};
		typedef std::string Token;

		// Information mapping a user ID to Authorisation data.
		// FIXME: just hit the database directly.
		std::map<int,   Authorisation> users;

		// Information mapping a UUID token to a user ID.
		std::map<Token, int>           authorisations;

		// Information mapping a UUID token to an event (for passwordless sharing).
		std::map<Token, int>           share_links;

		// Initialise the authorisations map from a disk database.
		void        init_authorisations();

		// Register a new user.
		int         register_user(const std::string& user, const std::string& passwd, bool admin);

		// Register a new token for sharing an event without password.
		Token       register_share_link(int event_id);
			
		// Validate a user, return an access token if valid.
		std::string validate_user(const httplib::Request& request, const std::string& user, const std::string& passwd);

		// Determine if the user identified by the token has root permissions.
		bool        is_root(const std::string& token) const;

		// Determine if access to the given event is allowed with the given authorisation token.
		bool        access_allowed(int event_id, const std::string& token) const;
		
		// Extract the authorisation token from the HTTP request header, if any.
		std::string extract_token(const httplib::Request& request);

		// Send an access denied page and log the request.
		void        denied(const httplib::Request& request, httplib::Response& response);

		// Create a thumbnail file of the given photo and store at the given location on disk.
		void        create_thumbnail(const httplib::Request&, const Database::Photo& photo, const std::string& loc) const;

		// Correct orientation of the image given the orientation flag.
		cv::Mat     apply_orientation(cv::Mat, int orientation) const;

		// Start the thread which takes care of converting videos to HLS.
		void        hls_thread_run();
		
		// Given a video entry, check if the HLS version is present.
		bool        have_hls_for_video(const Database::Photo& photo) const;

		// Generate HLS version of given video entry. This will put the entry
		// on the stack of entries to be generated, and start the generation
		// thread if it isn't running.
		bool        generate_hls_for_video(const Database::Photo& photo) const;
		
		Database db;
//		std::vector<Database::Event> events;

		// Configuration details.
		nlohmann::json config;

		mutable std::mutex video_mutex;
		mutable std::mutex auth_mutex;

		class StreamHandler {
			public:
				StreamHandler(std::string);

				static const uint64_t chunk_size=32*1024;

				std::string       filename;
				std::ifstream     vf;
				std::streamsize   size;				
				bool              finished;
				std::vector<char> data;
		};
		std::map<std::function<std::string(uint64_t)>, StreamHandler> stream_handlers;


		// Conversion thread and queue for videos.
		std::thread                         hls_thread;
		mutable std::mutex                  hls_mutex;
		mutable std::deque<Database::Photo> hls_queue;
};
