//
// Created by qife on 16/1/13.
//

#include "JValue.h"

using namespace JsonCpp;

const char *JValue::Parse(const char *str)
{
    str = JsonUtil::SkipWhiteSpace(str);
    if (*str == '\"')
    {
        // TODO: Parse string value
        ++str;
        autoMem.value.pStr = new std::string(JsonReader::ReadString(&str));
        return str;
    }
    if (std::strncmp(str, "true", 4) == 0)
    {
        autoMem.type = JValueType::boolean;
        autoMem.value.bVal = true;
        return str + 4;
    }
    if (std::strncmp(str, "false", 5) == 0)
    {
        autoMem.type = JValueType::boolean;
        autoMem.value.bVal = false;
        return str + 5;
    }
    if (std::strncmp(str, "null", 4) == 0)
    {
        autoMem.type = JValueType::null;
        return str + 4;
    }

    // TODO: Parse number value
    char *end;
    autoMem.value.num = std::strtod(str, &end);
    JsonUtil::Assert(end != str);
    autoMem.type = JValueType::number;
    return end;
}
