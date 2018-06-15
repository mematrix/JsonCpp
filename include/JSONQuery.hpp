//
// Created by Charles on 2018/1/21.
//

#ifndef JSONCPP_JSONQUERY_H
#define JSONCPP_JSONQUERY_H

#include "JSON.hpp"

namespace json {

json_token *select_token(json_token &token, const char *path);

inline json_token *select_token(json_token &token, const std::string &path)
{
    return select_token(token, path.c_str());
}

std::vector<json_token *> select_tokens(json_token &token, const char *path);

inline std::vector<json_token *> select_tokens(json_token &token, const std::string &path)
{
    return select_tokens(token, path.c_str());
}

}

#endif //JSONCPP_JSONQUERY_H
