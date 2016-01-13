//
// Created by qife on 16/1/11.
//

#include <string>

#include "JObject.h"

using namespace JsonCpp;
using pToken = std::unique_ptr<JToken>;
using tokenItem = std::pair<std::string, pToken>;

const char *JObject::Parse(const char *str)
{
    str = JsonUtil::SkipWhiteSpace(str);
    JsonUtil::AssertEqual<JValueType::object>(*str, '{');
    // empty object
    str = JsonUtil::SkipWhiteSpace(str, 1);
    if (*str == '}')
    {
        return str + 1;
    }

    while (true)
    {
        JsonUtil::AssertEqual<JValueType::object>(*str, '\"');

        ++str;
        auto end = strchr(str, '\"');
        std::string key(str, end - str);        // TODO: handle escape character

        str = JsonUtil::SkipWhiteSpace(end, 1);
        JsonUtil::AssertEqual<JValueType::object>(*str, ':');
        ++str;
        auto ret = children.insert(tokenItem(key, JsonReader::ReadToken(&str)));
        JsonUtil::Assert(ret.second);

        str = JsonUtil::SkipWhiteSpace(str);
        if (*str == ',')
        {
            str = JsonUtil::SkipWhiteSpace(str, 1);
            continue;
        }
        JsonUtil::Assert(*str == '}');

        return str + 1;
    }
}
