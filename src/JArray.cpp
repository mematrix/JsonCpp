//
// Created by qife on 16/1/12.
//

#include "JArray.h"

using namespace JsonCpp;

const char *JArray::Parse(const char *str)
{
    str = JsonUtil::SkipWhiteSpace(str);
    JsonUtil::AssertEqual(*str, '[');

    while (true)
    {
        ++str;
        children.push_back(JsonReader::ReadToken(&str));

        str = JsonUtil::SkipWhiteSpace(str);
        if (*str == ',')
        {
            continue;
        }

        JsonUtil::AssertEqual(*str, ']');
        return str + 1;
    }
}
