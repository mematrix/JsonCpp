//
// Created by qife on 16/1/12.
//

#include "JArray.h"

using namespace JsonCpp;

const char *JArray::Parse(const char *str)
{
    str = JsonUtil::SkipWhiteSpace(str);
    JsonUtil::AssertEqual<JValueType::array>(*str, '[');
    // empty array
    str = JsonUtil::SkipWhiteSpace(str, 1);
    if (*str == ']')
    {
        return str + 1;
    }

    while (true)
    {
        children.push_back(JsonReader::ReadToken(&str));

        str = JsonUtil::SkipWhiteSpace(str);
        if (*str == ',')
        {
            ++str;
            continue;
        }

        JsonUtil::AssertEqual<JValueType::array>(*str, ']');
        return str + 1;
    }
}
