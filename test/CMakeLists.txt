cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(TEST_SOURCE test.cpp)

include_directories(../include)

add_executable(jsonTest ${TEST_SOURCE})
target_link_libraries(jsonTest JsonCpp)
