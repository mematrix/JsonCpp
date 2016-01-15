set(JSONCPP_SOURCE
        JObject.cpp JArray.cpp JValue.cpp JsonReader.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_library(JsonCpp ${JSONCPP_SOURCE})
