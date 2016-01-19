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
        autoMem.type = JValueType::String;
        return str;
    }
    if (std::strncmp(str, "true", 4) == 0)
    {
        autoMem.type = JValueType::Boolean;
        autoMem.value.bVal = true;
        return str + 4;
    }
    if (std::strncmp(str, "false", 5) == 0)
    {
        autoMem.type = JValueType::Boolean;
        autoMem.value.bVal = false;
        return str + 5;
    }
    if (std::strncmp(str, "null", 4) == 0)
    {
        autoMem.type = JValueType::Null;
        return str + 4;
    }

    // TODO: Parse number value
    char *end;
    autoMem.value.num = std::strtod(str, &end);
    JsonUtil::Assert(end != str);
    autoMem.type = JValueType::Number;
    return end;
}

JValueType JValue::GetType() const
{
    return autoMem.type;
}

JValue::operator bool() const
{
    if (autoMem.type == JValueType::Boolean)
    {
        return autoMem.value.bVal;
    }

    throw JsonException("This json value is not a bool value");
}

JValue::operator double() const
{
    if (autoMem.type == JValueType::Number)
    {
        return autoMem.value.num;
    }

    throw JsonException("This json value is not a number value");
}

JValue::operator std::string() const
{
    if (autoMem.type == JValueType::String)
    {
        return *autoMem.value.pStr;
    }

    throw JsonException("This json value is not a string value");
}

const JToken &JValue::operator[](const std::string &) const
{
    throw JsonException("Access not support");
}

const JToken &JValue::operator[](unsigned long) const
{
    throw JsonException("Access not support");
}

const JToken &JValue::GetValue(const std::string &) const
{
    throw JsonException("Access not support");
}

const JToken &JValue::GetValue(unsigned long) const
{
    throw JsonException("Access not support");
}

const std::string &JValue::ToString() const
{
    if (nullptr == valString)
    {
        valString = new std::string();
        if (JValueType::String == autoMem.type)
        {
            valString->push_back('\"');
            valString->append(*autoMem.value.pStr);
            valString->push_back('\"');
        }
        else if (JValueType::Null == autoMem.type)
        {
            valString->append("null");
        }
        else if (JValueType::Boolean == autoMem.type)
        {
            valString->append(autoMem.value.bVal ? "true" : "false");
        }
        else if (JValueType::Number == autoMem.type)
        {
            auto str = std::to_string(autoMem.value.num);
            auto len = str.size() - 1;
            while (str[len] == '0')
            {
                str.pop_back();
                --len;
            }
            if (str[len] == '.')
            {
                str.pop_back();
            }
            valString->append(str);
        }
    }

    return *valString;
}
