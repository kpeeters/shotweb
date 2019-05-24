cmake_minimum_required(VERSION 2.6)

project(phosha)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++11")
endif()

find_package(OpenCV REQUIRED)
find_library(sqlite3 sqlite3 required)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.45.0 COMPONENTS system REQUIRED)
include_directories(${CMAKE_SOURCE_DIR}/../external/sqlite_modern_cpp/hdr)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        message(FATAL_ERROR "GCC version >=4.9 required for bugfree C++11 regex support.")
    endif()
endif()

add_executable(test_database test_database.cc Database.cc Utility.cc)
add_executable(shotweb Database.cc Server.cc Utility.cc shotweb.cc )
target_link_libraries(test_database sqlite3)
target_link_libraries(shotweb ${Boost_LIBRARIES} sqlite3 ${OpenCV_LIBS} pthread)

set(HTML_CSS_JS
  static/login.html
  static/login.css
  static/login.js
  static/events.html
  static/events.js
  static/event.html
  static/event.js
  static/denied.html
  static/setup.html
  static/jquery.lazyload.js
  static/lazyload.min.js
  static/fullscreen.png
  static/favicon-192.png
  static/strip_upper.png
  static/strip_lower.png  
)
FILE(GLOB FANCYBOX "${PROJECT_SOURCE_DIR}/static/fancybox/*.png" 
                   "${PROJECT_SOURCE_DIR}/static/fancybox/*.css" 
                   "${PROJECT_SOURCE_DIR}/static/fancybox/*.js")
FILE(GLOB FANCYBOX_HELPERS "${PROJECT_SOURCE_DIR}/static/fancybox/helpers/*")

install(PROGRAMS ${PROJECT_BINARY_DIR}/shotweb DESTINATION bin)
install(FILES ${HTML_CSS_JS}      DESTINATION share/shotweb)
install(FILES ${FANCYBOX}         DESTINATION share/shotweb/fancybox)
install(FILES ${FANCYBOX_HELPERS} DESTINATION share/shotweb/fancybox/helpers)
