
Shotweb: web server for shotwell photo databases
================================================

Shotweb is a simple, modern and compact C++ server to share Shotwell
photo albums via the web.

Features:

* Serve directly from the shotwell database and photo folders.
* Photos as well as videos.
* Multiple user accounts.
* Each user can be given access to individual events.
* Events can be shared by sending a link with a token, does not
  require guest to register an account.
* Generates thumbnails for photos and videos on the fly.
* Thumbnail caching.

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
    
    
    

Used libraries 
--------------

* opencv for image manipulation and thumbnail generation.
* sqlite_modern_cpp to access e.g. shotwell's sqlite database.
* nlohmann/json for json manipulation.
* httplib.h [] for web server functionality.
* miniz-cpp [https://github.com/tfussell/miniz-cpp.git] to create zip files.
