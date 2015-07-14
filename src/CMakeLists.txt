cmake_minimum_required(VERSION 2.6)

project(phosha)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++11")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(ImageMagick++ REQUIRED ImageMagick++)
include_directories(${ImageMagick++_INCLUDE_DIRS})
link_directories(${ImageMagick++_LIBRARY_DIRS})
find_library(sqlite3 sqlite3 required)
find_package(Boost 1.45.0 COMPONENTS system)


add_executable(test_database test_database.cc Database.cc)
add_executable(test_server test_server.cc Server.cc Database.cc)
target_link_libraries(test_database sqlite3)
target_link_libraries(test_server pthread ${Boost_LIBRARIES} sqlite3 ${ImageMagick++_LIBRARIES})
