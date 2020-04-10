
Shotweb: web server for shotwell photo databases
================================================

Written by Kasper Peeters. Licensed under the GPL.

*WARNING* not yet completely useable for others!


Shotweb is a simple, modern and compact C++ server to share Shotwell
[https://wiki.gnome.org/Apps/Shotwell] photo albums via the web.

Features:

* Serve directly from the shotwell database and photo folders.
* Photos as well as videos.
* Multiple user accounts.
* Each user can be given access to individual events.
* Events can be shared by sending a link with a token, does not
  require guest to register an account.
* Generates thumbnails for photos and videos on the fly.
* Thumbnail caching.
* Clean compact C++ with handcoded HTML/CSS/JS.


In the pipeline:

* Photo pre-loading for quick browsing.
* Download single photos or entire events as zip files.

Building
--------

First install the prerequisites with::

    sudo apt install cmake g++ libopencv-dev libboost-all-dev \
                     libsqlite3-dev ffmpegthumbnailer 
    
Then build as::

    cd shotweb
    mkdir build
    cd build
    cmake ..
    make
    sudo make install


Running
-------

Run the shotweb server with::

    shotweb [path-to-photo.db] [path-to-auth.db] [path-to-html] [port]

The meaning of the arguments::

  * path-to-photo.db:   
        
       The full path to the 'photo.db' database, as generated by
       Shotwell. Use a copy if you can.
       
  * path-to-auth.db:
  
       The full path of the authentication database file
       'auth.db'. Can be empty on start, in which case the first
       access to shotweb will ask for a new admin name and password.
       
  * path-to-html:
  
       The directory in which the HTML/CSS/JS files are stored. This
       is typically '/usr/local/share/shotweb/' or similar.
       
  * port:
  
       The IP port on which to run the server. If you want to run it
       over https, the best idea is to run it behind an apache or
       nginx web server and let that one proxy the connection to
       shotweb. 
    
You can now access the site and setup your administrator password (do
this immediately).

    
Inspiration
-----------

There are a few related projects, mostly unmaintained, which inspired Shotweb::

    http://www.jonh.net/~jonh/shotgo/README.html
    https://github.com/vmassuchetto/shotwell-web-client/tree/master/shotwell_web_client

    
Movie re-encoding
-----------------

To re-encode a movie for a maximal bitrate of 16M, do something like::

    ffmpeg -i IMG_0841.MOV -c:v libx264 -movflags faststart -crf 23 -maxrate 16M -bufsize 32M -vf "scale=960:-1" 16M/IMG_0841.MOV

The server will automatically pick up when there is a re-encoded movie
in the `16M` subdirectory available.

See [https://slhck.info/video/2017/03/01/rate-control.html] for useful
info on the various ways to reduce bandwidth.


TODO
----

* Re-load database when it has been updated externally.
* Script to re-encode videos for streaming.
* Log all access into database table and allow viewing by admin.
* Document database files used.
* Edit users and rights.


Used libraries 
--------------

* OpenCV [https://opencv.org]
  For image manipulation and photo/video thumbnail generation.

* sqlite_modern_cpp [https://github.com/SqliteModernCpp/sqlite_modern_cpp] 
  Used to access e.g. shotwell's sqlite database.
  
* nlohmann/json [https://github.com/nlohmann/json]
  For json manipulation.

* httplib.h [https://github.com/yhirose/cpp-httplib] 
  For web server functionality.

* miniz-cpp [https://github.com/tfussell/miniz-cpp.git] 
  To create zip files.
