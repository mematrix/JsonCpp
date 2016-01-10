cmake_minimum_required(VERSION 3.3)
project(JSONCPP)

include_directories(${JSONCPP_SOURCE_DIR}/include)
add_subdirectory(src)

set(JSONCPP_LIB_LOCATION "$<TARGET_FILE:${JsonCpp}>")
