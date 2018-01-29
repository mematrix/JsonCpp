//
// Created by Charles on 2018/1/21.
//

#ifndef JSONCPP_JSONQUERY_H
#define JSONCPP_JSONQUERY_H

#include "JSON.hpp"

namespace json {

//const JToken *select_token(const JToken &token, const char *path);
//
//const JToken *select_token(const JToken &token, const std::string &path)
//{
//    return select_token(token, path.c_str());
//}

JToken *select_token(JToken &token, const char *path);

JToken *select_token(JToken &token, const std::string &path)
{
    return select_token(token, path.c_str());
}

//std::vector<const JToken *> select_tokens(const JToken &token, const char *path);
//
//std::vector<const JToken *> select_tokens(const JToken &token, const std::string &path)
//{
//    return select_tokens(token, path.c_str());
//}

std::vector<JToken *> select_tokens(JToken &token, const char *path);

std::vector<JToken *> select_tokens(JToken &token, const std::string &path)
{
    return select_tokens(token, path.c_str());
}

}

#endif //JSONCPP_JSONQUERY_H
