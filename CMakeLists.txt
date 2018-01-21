cmake_minimum_required(VERSION 3.3)
project(JSONCPP)

add_subdirectory(src)
add_subdirectory(test)

set(JSONCPP_LIB_LOCATION "$<TARGET_FILE:${JsonCpp}>")
