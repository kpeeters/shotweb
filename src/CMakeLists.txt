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
find_package(Boost 1.45.0 COMPONENTS system thread coroutine context REQUIRED)


add_executable(test_database test_database.cc Database.cc)
add_executable(shotweb test_server.cc Server.cc Database.cc)
target_link_libraries(test_database sqlite3)
target_link_libraries(shotweb pthread ${Boost_LIBRARIES} sqlite3 ${OpenCV_LIBS})

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
