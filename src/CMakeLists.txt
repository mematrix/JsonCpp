cmake_minimum_required(VERSION 3.3)

set(JSONCPP_SOURCE JSON.cpp)
set(FORCE_CLION_CODE_INSIGHT ../include/JSON.hpp ../include/JSONConvert.hpp ../include/JSONQuery.hpp)

option(BUILD_QUERY_SUPPORT "build json query support module" ON)
if (BUILD_QUERY_SUPPORT)
    set(JSONCPP_SOURCE ${JSONCPP_SOURCE} JSONQuery.cpp)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(../include)

add_library(JsonCpp ${JSONCPP_SOURCE} ${FORCE_CLION_CODE_INSIGHT})
