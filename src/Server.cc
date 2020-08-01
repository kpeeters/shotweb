
#include "Server.hh"
#include "Utility.hh"
#include <regex>

#include <opencv2/opencv.hpp>

// Boost UUID is already header-only.
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/filesystem.hpp>

#include <libscrypt.h>

#include <thread>

using json = nlohmann::json;

Server::StreamHandler::StreamHandler(std::string fn)
	: filename(fn)
	, vf(filename, std::ios::binary | std::ios::ate)
	, size(0)
	, finished(false)
	{
	data.resize(chunk_size);
	size = vf.tellg();
	vf.seekg(0, std::ios::beg);	
	}

Server::Server(const nlohmann::json& config)
	: db(config.value("photo.db", ""), config.value("old_root", ""), config.value("new_root", "")),
	  config(config)
	{
	// Read configuration.
	
	try {
		init_authorisations();
		}
	catch(std::exception& ex) {
		std::cerr << "Server::Server: failed to load authorisation database: " << ex.what() << std::endl;
		throw;
		}
		
//	events = db.get_events();
	
	// Setup handler for JSON authenticate requests.
	Post("/.*json",
		  [&](const httplib::Request& request, httplib::Response& response) {
			  std::cerr << logstamp(&request) << "json request " << request.body << std::endl;
			  handle_json(request, response);
			  }
		  );
	Get("/.*", 
		 [&](const httplib::Request& request, httplib::Response& response) {
			 std::cerr << logstamp(&request) << "serving " << request.path << std::endl;
			 handle_default(request, response);
			 }
		 );
	}

std::string Server::logstamp(const httplib::Request *request) const
	{
	std::ostringstream str;
	std::time_t time_now = std::time(nullptr);
	str << std::put_time(std::localtime(&time_now), "%y-%m-%d %OH:%OM:%OS") << ", ";
	if(request!=0)
		str << std::setw(16) << request->get_header_value("REMOTE_ADDR") << ", " << std::setw(16) << request->get_header_value("X-Forwarded-For") << ": ";
	else
		str << std::setw(16) << "0.0.0.0" << ", " << std::setw(16) << " " << ": ";
	return str.str();
	}

Server::~Server()
	{
	}

void Server::init_authorisations()
	{
	std::cerr << logstamp(0) << "auth.db path: " << config.value("auth.db", "") << std::endl;
	
	sqlite::database db(config.value("auth.db", ""));
	std::string query;

	// Ensure tables exist.
	query = "create table if not exists users  ( id integer primary key, name text, password text, root int );";
	db << query;
	query = "create table if not exists events ( id int, event_id int );";
	db << query;
	query = "create table if not exists shares ( token text, event_id int );";
	db << query;

   // Read in all users.
	query = "select id, name, password, root from users;";
	db << query
		>> [&](int id, std::string name, std::string password, int root) {
		Authorisation auth;
		auth.id=id;
		auth.name=name;
		auth.password=password;
		auth.root=root;
		users[id]=auth;
		};

	// Read all events for each user.
	query = "select id, event_id from events;";
	db << query
		>> [&](int id, int event_id) {
		users[id].events.push_back(event_id);
		};
	
	std::cerr << logstamp() << "read " << users.size() << " users." << std::endl;
	if(users.size()==0)
		std::cerr << logstamp() << "WARNING: set the admin password NOW by connecting to the server!" << std::endl;

	// Read in all info for passwordless access.

	query = "select token, event_id from shares;";
	db << query
		>> [&](std::string token, int event_id) {
		share_links[token]=event_id;
		};

	std::cerr << logstamp() << "read " << share_links.size() << " sharing tokens." << std::endl;
	}

int Server::register_user(const std::string& user, const std::string& password, bool admin) 
	{
	if(users.size()>0 && admin) {
		std::cerr << logstamp() << "attempt to register admin user when one is already present" << std::endl;
		return -1;
		}
	
	char outbuf[SCRYPT_MCF_LEN];
	libscrypt_hash(outbuf, password.c_str(), SCRYPT_N, SCRYPT_r, SCRYPT_p);
	
	Authorisation auth;
	auth.name=user;
	auth.password=outbuf;
	auth.root=admin;

	sqlite::database db(config.value("auth.db", ""));
	db << "insert into users (name, password, root) values (?, ?, ?);"
		<< auth.name
		<< auth.password
		<< (int)auth.root;

	auth.id=db.last_insert_rowid();
	users[auth.id]=auth;

	std::cerr << logstamp() << "registered user " << user << (admin?" as admin":"as normal user") << std::endl;
	return auth.id;
	}

Server::Token Server::register_share_link(int event_id)
	{
	std::string token = boost::uuids::to_string(boost::uuids::random_generator()());
	share_links[token]=event_id;
	sqlite::database db(config.value("auth.db", ""));
	db << "insert into shares (token, event_id) values (?, ?);"
		<< token
		<< event_id;
	return token;
	}

std::string Server::validate_user(const std::string& user, const std::string& password) 
	{
	auto it=users.begin();
	while(it!=users.end()) {
		if(user==it->second.name) {
			char mcf[SCRYPT_MCF_LEN];
			strncpy(mcf, it->second.password.c_str(), std::min(SCRYPT_MCF_LEN, (int)it->second.password.size())+1);
			int retval = libscrypt_check(mcf, password.c_str());
			if(retval>0) {
				std::string token = boost::uuids::to_string(boost::uuids::random_generator()());
				authorisations[token]=(it->second.id);
				std::cerr << logstamp() << "authenticated user " << user << std::endl;
				return token;
				}
			else if(retval<0) {
				std::cerr << logstamp() << "error running libscrypt_check" << std::endl;
				}
			else {
				std::cerr << logstamp() << "failed to authenticate user " << user << std::endl;
				}
			return "";
			}
		++it;
		}
	std::cerr << logstamp() << "no known user " << user << std::endl;
	return "";
	}

std::string Server::extract_token(const httplib::Request& request)
	{
	auto hit=request.headers.begin();
	while(hit!=request.headers.end()) {
		if((*hit).first=="Cookie") {
			std::regex tok("token=([a-f0-9\\-]*)");
			std::smatch sm;
			if(std::regex_search((*hit).second, sm, tok)) {
				return sm[1];
				}
			}
		++hit;
		}
//	snoop::log(snoop::warn) << "Request without token" << snoop::flush;
	return "";
	}

bool Server::is_root(const std::string& token) const
	{
	auto it=authorisations.find(token);
	if(it==authorisations.end()) return false;
	auto uit=users.find(it->second);
	if(uit==users.end()) return false;
	const Authorisation& auth=uit->second;

	return auth.root;
	}


bool Server::access_allowed(int event_id, const std::string& token) const
	{
	auto it=authorisations.find(token);
	if(it==authorisations.end()) { // ticket not found to match to a user
		auto sit=share_links.find(token);
		if(sit==share_links.end()) { // ticket not found for sharing
			return false;
			}
		else return sit->second==event_id;
		}

	auto uit=users.find(it->second);
	if(uit==users.end())
		return false;             // ticket found but no corresponding user

	const Authorisation& auth=uit->second;
	
	if(auth.root) return true;   // root has access to everything

	if(std::find(auth.events.begin(), auth.events.end(), event_id)!=auth.events.end()) return true;

	return false;
	}
		
void Server::handle_json(const httplib::Request& request, httplib::Response& response)
	{
	try {
		auto content = json::parse(request.body);
		std::string action = content["action"];
		std::string token  = extract_token(request);
	
		// Login authentication, hands out a token if authentication valid.

		if(action=="login") {
			std::string user = content["username"];
			std::string password = content["password"];

			if(users.size()==0) {
				register_user(user, password, true);
				}
		
			std::string newtoken = validate_user(user, password);

			if(newtoken=="") {
				std::string ret = "{\"status\": \"error\"}\n";
				int length=ret.size();
				response.set_content(ret, "application/json");
				}
			else {
				std::string ret = "{\"status\": \"ok\", \"token\": \"" + newtoken + "\"}\r\n";
				int length=ret.size();
				response.set_header("Set-Cookie", ("token="+newtoken).c_str());
				response.set_content(ret, "application/json");
				}
			return;
			}

		// Produce a JSON structure with all events for this user.

		if(action=="events") {
			auto events = db.get_events();		
			auto it=events.begin();
			json ret;
			bool first=true;
			int added=0;
			while(it!=events.end()) {
				if(access_allowed(it->id, token)) {
					++added;
					json evt;
					evt["id"]=it->id;
					evt["event"]=it->name;
					evt["start"]= it->start_timestamp;
					evt["end"]  = it->end_timestamp;
					ret.push_back(evt);
					}
				++it;
				}
			if(added==0)
				response.set_content("[]\n", "application/json");
			else
				response.set_content(ret.dump(), "application/json");
			return;
			}

		// Send account information.
	
		if(action=="account") {
			json ret;
			ret["title"]=config.value("title", "");
			ret["allowed_add_user"]=false;
			ret["allowed_share"]=false;
			ret["allowed_download"]=false;
			response.set_content(ret.dump(), "application/json");
			}

		// Send accounts database.

		if(action=="accounts_db") {
			if(!is_root(token)) {
				denied(request, response);
				return;
				}
			auto ret=json::array();
			for(const auto& user: users) {
				json acc;
				acc["id"]  =user.second.id;
				acc["name"]=user.second.name;
				acc["root"]=user.second.root;
				ret.push_back(acc);
				}
			response.set_content(ret.dump(), "application/json");
			}
	
		// Add a new empty, non-root account to the accounts
		// database. Return the account id.

		if(action=="add_account") {
			if(!is_root(token)) {
				denied(request, response);
				return;
				}
			int id = register_user("", "", false);
			json ret;
			ret["status"]="success";
			ret["id"]=id;
			response.set_content(ret.dump(), "application/json");
			}
	
		// Update an account in the accounts database.

		if(action=="update_account") {
			if(!is_root(token)) {
				std::cerr << logstamp(&request) << "not admin user, cannot change account" << std::endl;
				denied(request, response);
				return;
				}
			int id=content.value("id", -1);
			if(id!=-1) {
				char outbuf[SCRYPT_MCF_LEN];
				libscrypt_hash(outbuf, content.value("password", "").c_str(), SCRYPT_N, SCRYPT_r, SCRYPT_p);
			
				sqlite::database db(config.value("auth.db", ""));
				db << "update users set name=?, password=? where id=?;"
					<< content.value("name", "")
					<< outbuf
					<< id;
			
				users[id].name=content.value("name", "");
				users[id].password=outbuf;

				std::cerr << logstamp(&request) << "user " << content.value("name", "") << " updated" << std::endl;
				
				json ret;
				ret["status"]="success";
				response.set_content(ret.dump(), "application/json");
				}
			else {
				json ret;
				ret["status"]="failure";
				response.set_content(ret.dump(), "application/json");
				}
			return;
			}
		
		// Produce a sharing token URL.

		if(action=="share") {
			int event_id=content["event_id"];
			Token token=register_share_link(event_id);
			std::cerr << logstamp(&request) << "sharing event " << event_id << " with token " << token << std::endl;
			json ret;
//		for(const auto& m: request.headers) {
//			std::cerr << m.first << " = " << m.second << std::endl;
//			}
			auto refit=request.headers.find("Referer");
			std::string ref;
			if(refit!=request.headers.end())
				ref=refit->second;
			replace(ref, "events.html", "event.html");
			ret["url"]=ref+"?id="+token;
			response.set_content(ret.dump(), "application/json");
			return;
			}
	
		// Produce a JSON structure with all photos for this user.

		if(action=="photos") {
			std::string event_id_string = content["id"];
			bool allowed=false;
			int event_id=-1;
			if(event_id_string.size()==36) {// this is a UUID, not a numerical id
				auto it=share_links.find(event_id_string);
				if(it!=share_links.end()) {
					allowed=true;
					event_id=it->second;
					std::cerr << logstamp(&request) << "shared token access to event " << event_id << std::endl;
					// Set this token as access cookie so we can get through photos etc.
					response.set_header("Set-Cookie", ("token="+event_id_string).c_str());
					}
				else std::cerr << logstamp(&request) << "shared token " << event_id_string << " invalid" << std::endl;
				}
			else {
				event_id=atoi(event_id_string.c_str());
				allowed=access_allowed(event_id, token);
				}

			if(allowed) {
				Database::Event ev=db.get_event(event_id);
//			snoop::log(snoop::info) << "User " << authorisations[token].name 
//											<< " viewing event " << ev.name << snoop::flush;
//			snoop::log.sync_with_server();

				// FIXME: this is a waste, we should cache the photos information and then
				// use it in the next call to serve from this vector, rather than re-run the query.
				// How does this work with multi-threads, by the way?
				auto photos = db.get_photos(event_id);
				auto events = db.get_events();
			
				std::ostringstream ss;
				auto eit=events.begin();
				while(eit!=events.end())
					if(eit->id==event_id)
						break;
					else 
						++eit;

				json ret;
				auto it=photos.begin();
				ret["name"]=eit->name;
				json jphotos;
				while(it!=photos.end()) {
					json photo;
					photo["id"]=it->id;
					photo["filename"]=it->filename;
					photo["is_video"]=it->is_video;
					jphotos.push_back(photo);
					++it;
					}
				ret["photos"]=jphotos;
				response.set_content(ret.dump(), "application/json");
				}
			else {
				std::ostringstream ss;
				ss << "{ \"name\": \"\", \"photos\": [] }";
				std::string ret=ss.str();
				response.set_content(ret, "application/json");
				}
			return;
			}
		}
	catch(nlohmann::json::exception& jex) {
		std::cerr << logstamp(&request) << "server exception, aborting request: " << jex.what() << std::endl;
		json ret;
		ret["status"]="failure";
		response.set_content(ret.dump(), "application/json");
		}
	catch(nlohmann::detail::type_error& jex) {
		std::cerr << logstamp(&request) << "server exception, aborting request: " << jex.what() << std::endl;
		json ret;
		ret["status"]="failure";
		response.set_content(ret.dump(), "application/json");
		}
	catch(sqlite::errors::error& ex) {
		std::cerr << logstamp(&request) << "server SQL exception, aborting request: " << ex.what() << std::endl;
		json ret;
		ret["status"]="failure";
		response.set_content(ret.dump(), "application/json");
		}
	}

void Server::denied(const httplib::Request& request, httplib::Response& response)
	{
//	snoop::log(snoop::warn) << "Access denied for " << request->path << snoop::flush;
//	snoop::log.sync_with_server();

	send_file(request, response, config.value("html_css_js_dir", "")+"denied.html");
	}

void Server::handle_default(const httplib::Request& request, httplib::Response& response)
	{
	try {
		std::string fn;
		fn = request.path;
		while(!fn.empty() && fn[0] == '/') 
			fn = fn.substr(1);

		std::cerr << logstamp(&request) << "sanitised path |" << fn << "|" << std::endl;
		std::string token  = extract_token(request);

		if(fn=="" || fn=="/") {
			if(users.size()>0) response.set_redirect("login.html");
			else               response.set_redirect("setup.html");
			return;
			}
		if(fn=="setup.html" && users.size()>0) {
			response.set_redirect("login.html");
			return;
			}
		if(fn=="login.html" && users.size()==0) {
			response.set_redirect("setup.html");
			return;
			}

//	snoop::log(snoop::info) << "Request for " << fn << snoop::flush;

		if(! (fn=="login.html" || fn=="login.js" || fn=="login.css"
				|| fn=="setup.html"
				|| fn=="favicon-192.png"
				|| fn=="events.html" || fn=="events.js" 
				|| fn=="event.html"  || fn=="event.js"
				|| fn=="photo"
				|| fn=="video"
				|| fn=="jquery.lazyload.js"
				|| fn=="lazyload.min.js"			
				|| fn=="fullscreen.png"
				|| fn=="strip_upper.png"
				|| fn=="strip_lower.png"			
				|| fn=="fancybox/jquery.fancybox.css"
				|| fn=="fancybox/jquery.fancybox.pack.js"
				|| fn=="fancybox/fancybox_overlay.png"
				|| fn=="fancybox/fancybox_sprite.png"
				|| fn=="fancybox/fancybox_loading.gif"
				|| fn=="fancybox/helpers/jquery.fancybox-buttons.css"
				|| fn=="fancybox/helpers/jquery.fancybox-buttons.js"
				|| fn=="fancybox/helpers/jquery.fancybox-media.js"
				|| fn.substr(0,15)=="event_thumbnail"
				|| fn.substr(0,15)=="video_thumbnail"			
				|| fn.substr(0,15)=="photo_thumbnail" )) {
			std::string ret = "Error";
			response.set_content(ret, "text/plain");

//		snoop::log(snoop::warn) << "Refused to serve " << fn << snoop::flush;
//		snoop::log.sync_with_server();

			return;
			}
		if(fn.substr(0,15)=="event_thumbnail") {
			auto events = db.get_events();
			int event_num=atoi(fn.substr(16).c_str());
			// std::cout << "need event thumbnail " << fn.substr(16) << std::endl;
			if(access_allowed(event_num, token)) {
				try {
					Database::Event event;
					for(unsigned i=0; i<events.size(); ++i)
						if(events[i].id==event_num)
							event=events[i];
					Database::Photo cover_photo=db.get_photo(event.cover_photo_id);
					send_thumbnail(request, response, cover_photo, 300);
					}
				catch(std::range_error& ex) {
					std::cerr << logstamp(&request) << "error fetching event: " << ex.what() << std::endl;
					return;
					}
				catch(std::logic_error& ex) {
					std::cerr << logstamp(&request) << "file reading error: " << ex.what() << std::endl;
					return;
					}
				}
			else {
				denied(request, response);
				}
			return;
			}
		else if(fn.substr(0,15)=="photo_thumbnail") {
			int photo_num=atoi(fn.substr(16).c_str());
			try {
				Database::Photo photo;
				photo=db.get_photo(photo_num);
				if(access_allowed(photo.event_id, token)) {
					send_thumbnail(request, response, photo, 300);
					}
				else {
					denied(request, response);
					}
				}
			catch(std::exception& ex) {
				std::cerr << logstamp(&request) << "failure loading/sending thumb: " << ex.what() << std::endl;
				return;
				}
			return;
			}
		else if(fn.substr(0,15)=="video_thumbnail") {
			int video_num=atoi(fn.substr(16).c_str());
			try {
				Database::Photo photo;
				photo=db.get_video(video_num);
				if(access_allowed(photo.event_id, token)) {
					send_thumbnail(request, response, photo, 300);
					}
				else {
					denied(request, response);
					}
				}
			catch(std::exception& ex) {
				std::cerr << logstamp(&request) << "failure loading/sending thumb: " << ex.what() << std::endl;
				return;
				}
			return;
			}
		else if(fn.substr(0,5)=="photo") {
			auto id_it=request.params.find("id");
			if(id_it==request.params.end())
				throw std::logic_error("photo endpoint needs id");
		
			int photo_num=atoi(id_it->second.c_str());
			std::cerr << logstamp(&request) << "serving photo " << photo_num << std::endl;
			try {
				Database::Photo photo;
				photo=db.get_photo(photo_num);
				if(access_allowed(photo.event_id, token)) {
//				snoop::log(snoop::info) << "User " << authorisations[token].name << " viewing photo " << photo.filename << snoop::flush;
//				snoop::log.sync_with_server();
					send_photo(request, response, photo);
					}
				else
					denied(request, response);
				}
			catch(std::exception& ex) {
				std::cerr << logstamp(&request) << "Failure loading/sending photo: " << ex.what() << std::endl;			
//			snoop::log(snoop::warn) << "Problem opening file " << photo_num << snoop::flush;
//			snoop::log.sync_with_server();
				}
			}
		else if(fn.substr(0,5)=="video") {
			auto id_it=request.params.find("id");
			if(id_it==request.params.end())
				throw std::logic_error("video endpoint needs id");
		
			int photo_num=atoi(id_it->second.c_str());
			std::cerr << logstamp(&request) << "serving video " << photo_num << std::endl;
			try {
				Database::Photo photo;
				photo=db.get_video(photo_num);
				if(true || access_allowed(photo.event_id, token)) {
//				snoop::log(snoop::info) << "User " << authorisations[token].name << " viewing photo " << photo.filename << snoop::flush;
//				snoop::log.sync_with_server();
					send_video(request, response, photo);
					}
				else
					denied(request, response);
				}
			catch(std::exception& ex) {
				std::cerr << logstamp(&request) << "Failure loading/sending video: " << ex.what() << std::endl;			
//			snoop::log(snoop::warn) << "Problem opening file " << photo_num << snoop::flush;
//			snoop::log.sync_with_server();
				}
			}
		else {
			fn=config.value("html_css_js_dir", "")+fn;
			send_file(request, response, fn);
			}
		}
	catch(nlohmann::json::exception& jex) {
		std::cerr << logstamp(&request) << "server exception, aborting request: " << jex.what() << std::endl;
		}
	catch(nlohmann::detail::type_error& jex) {
		std::cerr << logstamp(&request) << "server exception, aborting request: " << jex.what() << std::endl;
		}
	}

static bool ends_with(const std::string& str, const std::string& suffix)
	{
	return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
	}

void Server::send_file(const httplib::Request& request, httplib::Response& response, const std::string& fn) const
	{
	std::ifstream ifs;
//	std::cout << "serving " << fn << std::endl;
	ifs.open(fn, std::ifstream::in);
	
	if(ifs) {
		std::cerr << logstamp(&request) << "file found: " << fn << std::endl;
		ifs.seekg(0, std::ios::end);
		size_t length=ifs.tellg();
		ifs.seekg(0, std::ios::beg);
		char* buffer=new char[length];
		ifs.rdbuf()->sgetn(buffer, length);
//		response << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n" << ifs.rdbuf();
		ifs.close();
		std::string ctype;
		if(ends_with(fn, ".jpg")) ctype="image/jpeg";
		else if(ends_with(fn, ".gif")) ctype="image/gif";
		else if(ends_with(fn, ".css")) ctype="text/css";
		else if(ends_with(fn, ".js"))  ctype="application/javascript";
		response.set_content(buffer, length, ctype.c_str());
		delete [] buffer;
		}
	else {
		std::cerr << logstamp(&request) << "file not found " << fn << std::endl;
//		snoop::log(snoop::warn) << "File " << fn << " not found" << snoop::flush;
//		snoop::log.sync_with_server();

		std::string ret = "{\"status\": \"notfound\"}\n";
		response.set_content(ret, "application/json");
		return;
		}
	}

void Server::send_photo(const httplib::Request& request, httplib::Response& response, const Database::Photo& photo) const
	{
	try {
		// Shotwell has the convention (I think...) that the orientation stored
		// in the database overrides exif orientation.
		int flags = cv::IMREAD_ANYCOLOR;
		if(photo.orientation!=1)
			flags = cv::IMREAD_ANYCOLOR | cv::IMREAD_IGNORE_ORIENTATION;
		
		cv::Mat image = cv::imread(photo.filename.c_str(), flags);
		int maxsize=2048;
		// FIXME: adapt to user's browser resolution.
		if(image.cols>image.rows) {
			cv::resize(image,image,cv::Size(maxsize, (maxsize*image.rows)/image.cols));
			}
		else {
			cv::resize(image,image,cv::Size((maxsize*image.cols)/image.rows, maxsize));
			}

		image = apply_orientation(image, photo.orientation);
		
		std::vector<uchar> buf;
		cv::imencode(".jpg", image, buf, std::vector<int>() );
		response.set_content((const char *)buf.data(), buf.size(), "image/jpeg");
//		response << "HTTP/1.1 200 OK\r\nContent-Length: " << buf.size() << "\r\n\r\n";
//		response.write((const char *)buf.data(), buf.size());
		}
	catch(cv::Exception& ex) {
		}
	}

void Server::send_video(const httplib::Request& request, httplib::Response& response, const Database::Photo& photo) const
	{
	std::string file_to_send=photo.filename;
	std::cerr << logstamp(&request) << "request for video " << file_to_send << std::endl;
	// Check for presence of converted file.
	// FIXME: adapt to user preference for bitrate.
	boost::filesystem::path path(file_to_send);
	boost::filesystem::path path16m;
	path16m =path.parent_path();
	path16m/="16M";
	path16m/=path.filename();
	if(boost::filesystem::exists(path16m)) {
		file_to_send=path16m.string();
		std::cerr << logstamp(&request) << "diverting to re-encoded version " << file_to_send << std::endl;
		}
	
	response.set_header("Content-Type", "video/mp4");
	auto handler = new StreamHandler(file_to_send);

	auto chunk_size = handler->chunk_size;
	response.set_content_provider(
		handler->size,
		[handler,chunk_size,this,&request](uint64_t offset, uint64_t length, httplib::Out out)  {
			// Serve a chunk of data, of maximal size chunk_size.
			handler->vf.seekg(offset, std::ios::beg);
			handler->vf.read(handler->data.data(), std::min(chunk_size, length));
			uint64_t num=handler->vf.gcount();
			out(handler->data.data(), num);
			},
		[handler,this,&request]() {
			// Connection has closed, cleanup.
			delete handler;
			std::cerr << logstamp(&request) << "closing connection" << std::endl;
			});
	}

bool Server::file_exists(const std::string& filename) const
	{
	std::ifstream infile(filename);
	return infile.good();
	}

cv::Mat Server::apply_orientation(cv::Mat image, int orientation) const
	{
	if(orientation==8) {
		image = image.t();
		cv::flip(image,image,0);
		}
	if(orientation==6) {
		image = image.t();
		cv::flip(image,image,1);
		}
	if(orientation==3) {
		cv::flip(image,image,-1);
		}
	return image;
	}

void Server::create_thumbnail(const httplib::Request& request, const Database::Photo& photo, const std::string& loc) const
	{
	cv::Mat image;
	if(photo.is_video) {
		std::lock_guard<std::mutex> lock(video_mutex);
		std::cerr << logstamp(&request) << "creating video thumbnail for " << photo.filename << " at " << loc << std::endl;
		cv::VideoCapture capture(photo.filename);
		if(capture.isOpened()) {
			std::cerr << logstamp(&request) << "opened video " << photo.filename << std::endl;
			capture >> image;
			std::cerr << logstamp(&request) << "read frame" << std::endl;
			}
		else
			std::cerr << logstamp(&request) << "failed video " << photo.filename << std::endl;			
		}
	else {
		std::cerr << logstamp(&request) << "creating thumbnail for " << photo.filename << " orientation " << photo.orientation << " at " << loc << std::endl;
		int flags = cv::IMREAD_ANYCOLOR;
		if(photo.orientation!=1)
			flags = cv::IMREAD_ANYCOLOR | cv::IMREAD_IGNORE_ORIENTATION;
		image=cv::imread(photo.filename, flags);
		}
	
	if(image.data==0)
		throw std::logic_error("Photo "+photo.filename+" cannot be read");

	if(image.cols>image.rows) {
	  cv::resize(image,image,cv::Size(360, (360*image.rows)/image.cols), cv::INTER_AREA);
		}
	else {
	  cv::resize(image,image,cv::Size((360*image.cols)/image.rows, 360), cv::INTER_AREA);
		}
	image = apply_orientation(image, photo.orientation);

	cv::imwrite(loc, image);
	}

void Server::send_thumbnail(const httplib::Request& request, httplib::Response& response, const Database::Photo& photo, int max_size) const
	{
	// FIXME: use https://github.com/mattes/epeg

	// Shotwell stores its thumbnail files with the filename containing a hex representation
	// of the 'id' column of the photo. 

	std::ostringstream ss;
	ss << config.value("thumb_cache_dir", "") << (photo.is_video?"vthumb":"thumb")
		<< std::hex << std::noshowbase << std::setw(16) << std::setfill('0')
		<< photo.id << ".jpg";
	std::string fn=ss.str();

	if(!file_exists(fn))
		create_thumbnail(request, photo, fn);

//	std::cerr << logstamp(&request) << "sending thumbnail |" << fn << "|" << std::endl;

	try {
		cv::Mat image=cv::imread(fn.c_str());
		// If the thumbnail was generated by shotwell itself, it may not
		// be square. In that case, we need to cut a square image from
		// the middle.
		
		int w = image.cols;
		int h = image.rows;
		cv::Mat cropped;
		if(w>=h) {
			cropped = image(cv::Rect((w-h)/2,0,h,h));
			w=h;
			}
		else {
			cropped = image(cv::Rect(0,(h-w)/2,w,w));
			h=w;
			}
		
		// Resize to maximal size, if necessary.
//		if(w>max_size) {
//			cropped.resize(max_size,max_size);
//			}
		
		std::vector<uchar> buf;
		cv::imencode(".jpg", cropped, buf, std::vector<int>() );

		response.set_content((const char *)buf.data(), buf.size(), "image/jpeg");
		}
	catch(cv::Exception& ex) {
		}
	}

void Server::start() 
	{
	int port=config.value("port", 80);
	std::cerr << logstamp() << "starting server on port " << port << std::endl;
	listen("localhost", port);
	}

