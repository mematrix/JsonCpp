cmake_minimum_required(VERSION 3.3)

set(JSONCPP_SOURCE JSON.cpp)
set(FORCE_CLION_CODE_INSIGHT ../include/JSON.hpp ../include/JSONConvert.hpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(../include)

add_library(JsonCpp ${JSONCPP_SOURCE} ${FORCE_CLION_CODE_INSIGHT})
