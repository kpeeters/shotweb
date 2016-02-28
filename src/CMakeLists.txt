cmake_minimum_required(VERSION 2.6)

project(phosha)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++11")
endif()

find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Snoop REQUIRED)
find_library(sqlite3 sqlite3 required)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.45.0 COMPONENTS system thread coroutine context regex REQUIRED)
pkg_check_modules(JSONCPP jsoncpp REQUIRED)   # libjsoncpp-dev
message("-- Found JSONCPP include dir: ${JSONCPP_INCLUDE_DIRS}")
message("-- Found JSONCPP library dir: ${JSONCPP_LIBRARY_DIRS}")
message("-- Found JSONCPP libraries  : ${JSONCPP_LIBRARIES}")
if(JSONCPP_INCLUDE_DIRS)
include_directories("${JSONCPP_INCLUDE_DIRS}")
endif(JSONCPP_INCLUDE_DIRS)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        message(FATAL_ERROR "GCC version >=4.9 required for bugfree C++11 regex support.")
    endif()
endif()

add_executable(test_database test_database.cc Database.cc)
add_executable(shotweb shotweb.cc Server.cc Database.cc)
target_link_libraries(test_database sqlite3)
target_link_libraries(shotweb ${Boost_LIBRARIES} sqlite3 ${OpenCV_LIBS} pthread snoop jsoncpp uuid)

set(HTML_CSS_JS
  login.html
  login.css
  login.js
  events.html
  events.js
  event.html
  event.js
  denied.html
  jquery.lazyload.js
  fullscreen.png
  favicon-192.png
)
FILE(GLOB FANCYBOX "${PROJECT_SOURCE_DIR}/fancybox/*.png" 
                   "${PROJECT_SOURCE_DIR}/fancybox/*.css" 
                   "${PROJECT_SOURCE_DIR}/fancybox/*.js")
FILE(GLOB FANCYBOX_HELPERS "${PROJECT_SOURCE_DIR}/fancybox/helpers/*")

install(PROGRAMS ${PROJECT_BINARY_DIR}/shotweb DESTINATION bin)
install(FILES ${HTML_CSS_JS}      DESTINATION share/shotweb)
install(FILES ${FANCYBOX}         DESTINATION share/shotweb/fancybox)
install(FILES ${FANCYBOX_HELPERS} DESTINATION share/shotweb/fancybox/helpers)
