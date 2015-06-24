cmake_minimum_required(VERSION 2.6)

project(phosha)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++11")
endif()

find_library(sqlite3 sqlite3 required)

add_executable(app app.cc mongoose.c)
add_executable(test_database test_database.cc Database.cc)
target_link_libraries(app pthread)
target_link_libraries(test_database sqlite3)
