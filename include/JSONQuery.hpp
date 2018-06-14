//
// Created by Charles on 2018/1/21.
//

#ifndef JSONCPP_JSONQUERY_H
#define JSONCPP_JSONQUERY_H

#include "JSON.hpp"

namespace json {

//const json_token *select_token(const json_token &token, const char *path);
//
//const json_token *select_token(const json_token &token, const std::string &path)
//{
//    return select_token(token, path.c_str());
//}

json_token *select_token(json_token &token, const char *path);

json_token *select_token(json_token &token, const std::string &path)
{
    return select_token(token, path.c_str());
}

//std::vector<const json_token *> select_tokens(const json_token &token, const char *path);
//
//std::vector<const json_token *> select_tokens(const json_token &token, const std::string &path)
//{
//    return select_tokens(token, path.c_str());
//}

std::vector<json_token *> select_tokens(json_token &token, const char *path);

std::vector<json_token *> select_tokens(json_token &token, const std::string &path)
{
    return select_tokens(token, path.c_str());
}

}

#endif //JSONCPP_JSONQUERY_H
