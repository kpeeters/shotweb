
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

std::string trim(const std::string& s)
	{
	if(s.length() == 0)
		return s;
	int b = s.find_first_not_of(" \t\n");
	int e = s.find_last_not_of(" \t\n");
	if(b == -1) // No non-spaces
		return "";
	return std::string(s, b, e - b + 1);
	}


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
	  config(config), hls_thread(&Server::hls_thread_run, this)
	{
	// Read configuration.
	
	try {
		init_authorisations();
		}
	catch(std::exception& ex) {
		terr << "Server::Server: failed to load authorisation database: " << ex.what() << std::endl;
		throw;
		}
		
//	events = db.get_events();
	
	// Setup handler for JSON authenticate requests.
	Post("/.*json",
		  [&](const httplib::Request& request, httplib::Response& response) {
			  terr << logstamp(&request) << "json request " << request.body << std::endl;
			  handle_json(request, response);
			  }
		  );
	Get("/.*", 
		 [&](const httplib::Request& request, httplib::Response& response) {
			 terr << logstamp(&request) << "request " << request.path << std::endl;
			 handle_default(request, response);
			 }
		 );

	}

Server::~Server()
	{
	}

void Server::init_authorisations()
	{
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	terr << logstamp(0) << "auth.db path: " << config.value("auth.db", "") << std::endl;
	
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
	
	terr << logstamp() << "read " << users.size() << " users." << std::endl;
	if(users.size()==0)
		terr << logstamp() << "WARNING: set the admin password NOW by connecting to the server!" << std::endl;

	// Read in all info for passwordless access.

	query = "select token, event_id from shares;";
	db << query
		>> [&](std::string token, int event_id) {
		share_links[token]=event_id;
		};

	terr << logstamp() << "read " << share_links.size() << " sharing tokens." << std::endl;
	}

int Server::register_user(const std::string& user, const std::string& password, bool admin) 
	{
	std::lock_guard<std::mutex> lock(auth_mutex);
	
	if(users.size()>0 && admin) {
		terr << logstamp() << "attempt to register admin user when one is already present" << std::endl;
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

	terr << logstamp() << "registered user " << user << (admin?" as admin":"as normal user") << std::endl;
	return auth.id;
	}

Server::Token Server::register_share_link(int event_id)
	{
	std::lock_guard<std::mutex> lock(auth_mutex);

	std::string token = boost::uuids::to_string(boost::uuids::random_generator()());
	share_links[token]=event_id;
	sqlite::database db(config.value("auth.db", ""));
	db << "insert into shares (token, event_id) values (?, ?);"
		<< token
		<< event_id;
	return token;
	}

std::string Server::validate_user(const httplib::Request& request, const std::string& user, const std::string& password) 
	{
	std::lock_guard<std::mutex> lock(auth_mutex);

	auto it=users.begin();
	while(it!=users.end()) {
		if(user==it->second.name) {
			char mcf[SCRYPT_MCF_LEN];
			strncpy(mcf, it->second.password.c_str(), std::min(SCRYPT_MCF_LEN, (int)it->second.password.size())+1);
			int retval = libscrypt_check(mcf, password.c_str());
			if(retval>0) {
				std::string token = boost::uuids::to_string(boost::uuids::random_generator()());
				authorisations[token]=(it->second.id);
				terr << logstamp(&request) << "authenticated user " << user << std::endl;
				return token;
				}
			else if(retval<0) {
				terr << logstamp(&request) << "error running libscrypt_check on " << password << std::endl;
				}
			else {
				terr << logstamp(&request) << "failed to authenticate user " << user << std::endl;
				}
			return "";
			}
		++it;
		}
	terr << logstamp(&request) << "no known user " << user << std::endl;
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
	std::lock_guard<std::mutex> lock(auth_mutex);

	auto it=authorisations.find(token);
	if(it==authorisations.end()) return false;
	auto uit=users.find(it->second);
	if(uit==users.end()) return false;
	const Authorisation& auth=uit->second;

	return auth.root;
	}


bool Server::access_allowed(int event_id, const std::string& token) const
	{
	std::lock_guard<std::mutex> lock(auth_mutex);

	auto it=authorisations.find(token);
	if(it==authorisations.end()) { // ticket not found to match to a user
		auto sit=share_links.find(token);
		if(sit==share_links.end()) { // ticket not found for sharing
			// terr << "no user ticket and no share ticket" << std::endl;
			return false;
			}
		else {
			// terr << "share ticket found" << std::endl;
			return sit->second==event_id;
			}
		}

	auto uit=users.find(it->second);
	if(uit==users.end()) {
		// terr << "ticket found but no user" << std::endl;		
		return false;             // ticket found but no corresponding user
		}

	const Authorisation& auth=uit->second;
	
	if(auth.root) {
		// terr << "root access" << std::endl;
		return true;   // root has access to everything
		}

	if(std::find(auth.events.begin(), auth.events.end(), event_id)!=auth.events.end()) {
		// terr << "user ticket found with access to event" << std::endl;
		return true;
		}
	// terr << "user ticket found but no access to event" << std::endl;

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
		
			std::string newtoken = validate_user(request, user, password);

			if(newtoken=="") {
				std::string ret = "{\"status\": \"error\"}\n";
				// int length=ret.size();
				response.set_content(ret, "application/json");
				}
			else {
				std::string ret = "{\"status\": \"ok\", \"token\": \"" + newtoken + "\"}\r\n";
				// int length=ret.size();
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
			// bool first=true;
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
			if(added==0) {
				terr << logstamp(&request) << "user not allowed access to any event" << std::endl;
				response.set_content("[]\n", "application/json");
				}
			else {
				terr << logstamp(&request) << "user allowed access to " << ret.size() << " events" << std::endl;				
				response.set_content(ret.dump(), "application/json");
				}
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
				terr << logstamp(&request) << "access to accounts_db denied because user is not admin" << std::endl;
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
				terr << logstamp(&request) << "access to add_account denied because user is not admin" << std::endl;
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
				terr << logstamp(&request) << "not admin user, cannot change account" << std::endl;
				denied(request, response);
				return;
				}
			int id=content.value("id", -1);
			if(id!=-1) {
				sqlite::database db(config.value("auth.db", ""));

				std::string name=content.value("name", "");
				if(name!="") {
					db << "update users set name=? where id=?;"
						<< name << id;
					users[id].name=content.value("name", "");
					}


				std::string password=content.value("password", "");
				if(password!="") {
					char outbuf[SCRYPT_MCF_LEN];
					libscrypt_hash(outbuf, password.c_str(), SCRYPT_N, SCRYPT_r, SCRYPT_p);
					db << "update users set password=? where id=?;"
						<< outbuf << id;
					users[id].password=outbuf;
					}					

				terr << logstamp(&request) << "user " << content.value("name", "") << " updated" << std::endl;
				
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
			terr << logstamp(&request) << "sharing event " << event_id << " with token " << token << std::endl;
			json ret;
//		for(const auto& m: request.headers) {
//			terr << m.first << " = " << m.second << std::endl;
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
					terr << logstamp(&request) << "shared token access to event " << event_id << std::endl;
					// Set this token as access cookie so we can get through photos etc.
					response.set_header("Set-Cookie", ("token="+event_id_string).c_str());
					}
				else terr << logstamp(&request) << "shared token " << event_id_string << " invalid" << std::endl;
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
					if(it->is_video) {
						photo["converting"]=!have_hls_for_video(*it);
						}
					else {
						photo["converting"]=false;
						}
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
	catch(nlohmann::detail::type_error& jex) {
		terr << logstamp(&request) << "server exception, aborting request: " << jex.what() << std::endl;
		json ret;
		ret["status"]="failure";
		response.set_content(ret.dump(), "application/json");
		}
	catch(nlohmann::json::exception& jex) {
		terr << logstamp(&request) << "server exception, aborting request: " << jex.what() << std::endl;
		json ret;
		ret["status"]="failure";
		response.set_content(ret.dump(), "application/json");
		}
	catch(sqlite::errors::error& ex) {
		terr << logstamp(&request) << "server SQL exception, aborting request: " << ex.what() << std::endl;
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

//		terr << logstamp(&request) << "sanitised path |" << fn << "|" << std::endl;
		std::string token  = extract_token(request);

		if(fn=="" || fn=="/") {
			if(users.size()>0) {
				terr << logstamp(&request) << "redirecting to login because path empty" << std::endl;
				response.set_redirect("login.html");
				}
			else {
				terr << logstamp(&request) << "redirecting to setup because no users in database" << std::endl;
				response.set_redirect("setup.html");
				}
			return;
			}
		if(fn=="setup.html" && users.size()>0) {
			terr << logstamp(&request) << "redirecting to login because setup already done" << std::endl;			
			response.set_redirect("login.html");
			return;
			}
		if(fn=="login.html" && users.size()==0) {
			terr << logstamp(&request) << "redirecting to setup because no users in database" << std::endl;
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
				|| fn.substr(0,5)=="video"
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
					Database::Photo cover_photo;
					if(event.cover_is_photo) 
						cover_photo=db.get_photo(event.cover_photo_id);
					else
						cover_photo=db.get_video(event.cover_photo_id);
					
					send_thumbnail(request, response, cover_photo, 300);
					}
				catch(std::range_error& ex) {
					terr << logstamp(&request) << "error fetching event: " << ex.what() << std::endl;
					return;
					}
				catch(std::logic_error& ex) {
					terr << logstamp(&request) << "file reading error: " << ex.what() << std::endl;
					return;
					}
				}
			else {
				terr << logstamp(&request) << "access to event denied" << std::endl;
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
					terr << logstamp(&request) << "access to photo thumbnail denied" << std::endl;
					denied(request, response);
					}
				}
			catch(std::exception& ex) {
				terr << logstamp(&request) << "failure loading/sending thumb: " << ex.what() << std::endl;
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
					terr << logstamp(&request) << "access to video thumbnail denied" << std::endl;
					denied(request, response);
					}
				}
			catch(std::exception& ex) {
				terr << logstamp(&request) << "failure loading/sending thumb: " << ex.what() << std::endl;
				return;
				}
			return;
			}
		else if(fn.substr(0,5)=="photo") {
			auto id_it=request.params.find("id");
			if(id_it==request.params.end())
				throw std::logic_error("photo endpoint needs id");
		
			int photo_num=atoi(id_it->second.c_str());
			terr << logstamp(&request) << "serving photo " << photo_num << std::endl;
			try {
				Database::Photo photo;
				photo=db.get_photo(photo_num);
				if(access_allowed(photo.event_id, token)) {
//				snoop::log(snoop::info) << "User " << authorisations[token].name << " viewing photo " << photo.filename << snoop::flush;
//				snoop::log.sync_with_server();
					send_photo(request, response, photo);
					}
				else {
					terr << logstamp(&request) << "access to photo denied" << std::endl;
					denied(request, response);
					}
				}
			catch(std::exception& ex) {
				terr << logstamp(&request) << "Failure loading/sending photo: " << ex.what() << std::endl;			
//			snoop::log(snoop::warn) << "Problem opening file " << photo_num << snoop::flush;
//			snoop::log.sync_with_server();
				}
			}
		else if(fn.substr(0,5)=="video") {
//			auto id_it=request.params.find("id");
//			if(id_it==request.params.end())
			if(fn.size()<7)
				throw std::logic_error("video endpoint needs id");
			size_t slash=fn.substr(6).find("/");
			if(slash==std::string::npos)
				throw std::logic_error("video endpoint needs type");
			
			int photo_num=atoi(fn.substr(6, slash).c_str());
			std::string vtype=fn.substr(6+slash+1);
			terr << logstamp(&request) << "video request " << vtype << " for " << photo_num << std::endl;
			try {
				Database::Photo photo;
				photo=db.get_video(photo_num);
				if(true || access_allowed(photo.event_id, token)) {
//				snoop::log(snoop::info) << "User " << authorisations[token].name << " viewing photo " << photo.filename << snoop::flush;
//				snoop::log.sync_with_server();
					send_video(request, response, photo, vtype);
					}
				else {
					terr << logstamp(&request) << "access to video denied" << std::endl;
					denied(request, response);
					}
				}
			catch(std::exception& ex) {
				terr << logstamp(&request) << "Failure loading/sending video: " << ex.what() << std::endl;			
//			snoop::log(snoop::warn) << "Problem opening file " << photo_num << snoop::flush;
//			snoop::log.sync_with_server();
				}
			}
		else {
			fn=config.value("html_css_js_dir", "")+fn;
			send_file(request, response, fn);
			}
		}
	catch(nlohmann::detail::type_error& jex) {
		terr << logstamp(&request) << "server exception, aborting request: " << jex.what() << std::endl;
		}
	catch(nlohmann::json::exception& jex) {
		terr << logstamp(&request) << "server exception, aborting request: " << jex.what() << std::endl;
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
		// terr << logstamp(&request) << "file found: " << fn << std::endl;
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
		terr << logstamp(&request) << "file not found " << fn << std::endl;
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

// void Server::send_m3u8(const httplib::Request& request, httplib::Response& response, const Database::Photo& photo) const
// 	{
// 	terr << logstamp(&request) << "request for video " << photo.filename << std::endl;
// 
// 	boost::filesystem::path path(photo.filename);
// 	auto dir =path.parent_path();
// 	std::string m3u8name = dir.c_str()+std::string("/index.m3u8");
// 	std::ifstream ifs;
// 	ifs.open(m3u8name, std::ifstream::in);
// 	
// 	if(ifs) {
// 		ifs.seekg(0, std::ios::end);
// 		size_t length=ifs.tellg();
// 		ifs.seekg(0, std::ios::beg);
// 		char* buffer=new char[length];
// 		ifs.rdbuf()->sgetn(buffer, length);
// //		response << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n" << ifs.rdbuf();
// 		ifs.close();
// 		response.set_header("Content-Type", "application/x-mpegURL");
// 		terr << logstamp(&request) << "sending m3u8 " << m3u8name << std::endl;
// 		delete [] buffer;
// 		}
// 	else {
// 		terr << logstamp(&request) << "file not found " << m3u8name << std::endl;
// 		std::string ret = "{\"status\": \"notfound\"}\n";
// 		response.set_content(ret, "application/json");
// 		return;
// 		}
// 	}

void Server::send_video(const httplib::Request& request, httplib::Response& response, const Database::Photo& photo,
								const std::string& vtype) const
	{
	terr << logstamp(&request) << "request for video " << photo.filename << " vtype " << vtype << std::endl;
	// Check for presence of converted file.
	boost::filesystem::path path(photo.filename);
	boost::filesystem::path dir, stem;
	dir =path.parent_path();
	stem=path.stem();

	if(!have_hls_for_video(photo)) {
		// If this video is not yet in the queue for conversion, add it.
		std::lock_guard<std::mutex> lock(hls_mutex);
		for(const auto& qe: hls_queue) {
			if(qe.filename == photo.filename) {
				terr << logstamp(&request) << "already converting " << photo.filename << std::endl;
				return;
				}
			}
		terr << logstamp(&request) << "adding " << photo.filename << " to conversion queue" << std::endl;
		hls_queue.push_front(photo);
		return;
		}
	
	// FIXME: sanitise vtype.
	size_t ll=vtype.size();
	std::string file_to_send = dir.c_str()+std::string("/")+stem.c_str()+std::string("/")+vtype;
	if(vtype.substr(ll-5)==".m3u8") {
		response.set_header("Content-Type", "application/x-mpegURL");
		terr << logstamp(&request) << "sending m3u8 " << file_to_send << std::endl;
		send_file(request, response, file_to_send);
		}
	else {
		terr << logstamp(&request) << "sending ts file " << file_to_send << std::endl;
		auto handler = new StreamHandler(file_to_send);
		std::string mime_type="video/mp4";
		if(path.extension()==".ts") mime_type = "video/mp2t";

		auto chunk_size = handler->chunk_size;
		response.set_content_provider(
			handler->size,
			mime_type.c_str(),
			[handler,chunk_size,this,&request](uint64_t offset, uint64_t length, httplib::DataSink& out)  {
				// Serve a chunk of data, of maximal size chunk_size.
				handler->vf.seekg(offset, std::ios::beg);
				handler->vf.read(handler->data.data(), std::min(chunk_size, length));
				uint64_t num=handler->vf.gcount();
				out.write(handler->data.data(), num);
				return true;
				},
			[handler,this,&request]() {
			// Connection has closed, cleanup.
			delete handler;
			terr << logstamp(&request) << "closing connection" << std::endl;
			});
		}
	}

bool Server::file_exists(const std::string& filename) const
	{
	std::ifstream infile(filename);
	return infile.good();
	}

bool Server::have_hls_for_video(const Database::Photo& photo) const
	{
//	terr << logstamp(&request) << "check presence of HLS for " << photo.filename << std::endl;
	boost::filesystem::path path(photo.filename);
	boost::filesystem::path dir, stem;
	dir =path.parent_path();
	stem=path.stem();

	// FIXME: extend this check to be more rigorous, and only return true if all HLS
	// files have been generated.
	std::string file_to_check = dir.c_str()+std::string("/")+stem.c_str()+std::string("/index.m3u8");
	std::ifstream tst(file_to_check);

	return tst.is_open();
	}

void Server::hls_thread_run()
	{
	// This function runs forever, on a separate thread, and watches the
	// `hls_queue` for videos that need converting to HLS format. Will
	// then, synchronously, call the `video2hls.py` script on each of
	// them.
	bool was_empty=false;
	int  empty_loops=0;
	for(;;) {
		sleep(1);
		std::lock_guard<std::mutex> lock(hls_mutex);
		if(hls_queue.size()>0) {
			was_empty=false;
			terr << logstamp(0) << "videos to process on HLS queue: " << hls_queue.size() << std::endl;			
			Database::Photo conv = hls_queue.front();
			hls_queue.pop_front();
			generate_hls_for_video(conv);
			}
		else {
			if(!was_empty) {
				was_empty=true;
				empty_loops=0;
				terr << logstamp(0) << "nothing left on HLS queue." << std::endl;
				}
			else {
				++empty_loops;
				if(empty_loops==10) {
					// Scan videos and put all for conversion.
					// FIXME: TODO
					}
				}
			}
		}
	}

bool Server::generate_hls_for_video(const Database::Photo& photo) const
	{
	terr << logstamp(0) << "generate HLS for " << photo.filename << std::endl;

	boost::filesystem::path path(photo.filename);
	boost::filesystem::path dir, stem;
	dir =path.parent_path();
	stem=path.stem();

	std::string hls_dir = dir.c_str()+std::string("/")+stem.c_str()+std::string("/");
	boost::filesystem::create_directory(hls_dir);
	std::string command = "video2hls.py --no-mp4 --output-overwrite --output "+hls_dir+" "+photo.filename;
	terr << logstamp(0) << "running " << command << std::endl;
	auto res = std::system(command.c_str());
	terr << logstamp(0) << "command " << command << " exited with code " << res << std::endl;
	
	return true;
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
		terr << logstamp(&request) << "creating video thumbnail for " << photo.filename << " at " << loc << std::endl;
		cv::VideoCapture capture(photo.filename);
		if(capture.isOpened()) {
			terr << logstamp(&request) << "opened video " << photo.filename << std::endl;
			capture >> image;
			terr << logstamp(&request) << "read frame" << std::endl;
			}
		else
			terr << logstamp(&request) << "failed video " << photo.filename << std::endl;			
		}
	else {
		terr << logstamp(&request) << "creating thumbnail for " << photo.filename << " orientation " << photo.orientation << " at " << loc << std::endl;
		int flags = cv::IMREAD_ANYCOLOR;
		if(photo.orientation!=1)
			flags = cv::IMREAD_ANYCOLOR | cv::IMREAD_IGNORE_ORIENTATION;
		image=cv::imread(photo.filename, flags);
		}
	
	if(image.data==0)
		throw std::logic_error("Photo "+photo.filename+" cannot be read");

	// Crop to square.
	if(image.cols>image.rows) {
		int margin = (image.cols-image.rows)/2;
		image = image( cv::Range(0, image.rows), cv::Range(margin, margin+image.rows) );
		}
	else if(image.cols<image.rows) {
		int margin = (image.rows-image.cols)/2;
		image = image( cv::Range(margin, margin+image.cols), cv::Range(0, image.cols) );
		}

	terr << logstamp(&request) << image.cols << " x " << image.rows << std::endl;

	// Resize and orient.
	cv::resize(image,image,cv::Size(360, 360), cv::INTER_AREA);
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

//	terr << logstamp(&request) << "sending thumbnail |" << fn << "|" << std::endl;

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
	// Spawn thread to watch changes to database file.
	std::thread watcher( [this]() {
			       for(;;) {
				 if(db.watch_for_changes()) {
				   db.open_or_reopen();
				 } else {
				   break;
				 }
			       }
			     });

	int port=config.value("port", 80);
	terr << logstamp() << "starting server on port " << port << std::endl;
	set_write_timeout(10,0);
	set_read_timeout(10,0);	
	if(listen("localhost", port)==false)
		terr << logstamp() << "failed to bind to port " << port << std::endl;

	std::cerr << "Server::start: joining watcher thread..." << std::endl;
	watcher.join();
	}

