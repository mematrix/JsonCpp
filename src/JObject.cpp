//
// Created by qife on 16/1/11.
//

#include <string>

#include "JObject.h"
#include "JsonReader.h"

using namespace JsonCpp;
using pToken = std::unique_ptr<JToken>;
using tokenItem = std::pair<std::string, pToken>;

const char *JObject::Parse(const char *str)
{
    while (true)
    {
        str = JsonUtil::SkipWhiteSpace(str);
        JsonUtil::AssertEqual<JValueType::object>(*str, '{');

        str = JsonUtil::SkipWhiteSpace(str, 1);
        JsonUtil::AssertEqual<JValueType::object>(*str, '\"');

        ++str;
        auto end = strchr(str, '\"');
        std::string key(str, end - str);

        str = JsonUtil::SkipWhiteSpace(end, 1);
        JsonUtil::AssertEqual<JValueType::object>(*str, ':');
        JToken *token = nullptr;
        str = JsonReader::ReadToken(str + 1, &token);
        auto ret = children.insert(tokenItem(key, pToken(token)));
        JsonUtil::Assert(ret.second);

        str = JsonUtil::SkipWhiteSpace(str);
        if (*str == ',')
        {
            ++str;
            continue;
        }
        JsonUtil::Assert(*str == '}');

        return str + 1;
    }
}
