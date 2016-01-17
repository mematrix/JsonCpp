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

JValueType JArray::GetType() const
{
    return JValueType::array;
}

bool JArray::operator bool() const
{
    return !children.empty();
}

double JArray::operator double() const
{
    return children.size();
}

std::string JArray::operator std::string() const
{
    return ToString();
}

const JToken &JArray::operator[](const std::string &str) const
{
    return GetValue(str);
}

const JToken &JArray::operator[](unsigned long index) const
{
    return GetValue(index);
}

const JToken &JArray::GetValue(const std::string &str) const
{
    throw JsonException("Access not support");
}

const JToken &JArray::GetValue(unsigned long index) const
{
    return children.at(index).operator*();
}

const std::string &JArray::ToString() const
{
    if (nullptr == aryString)
    {
        aryString = new std::string();
        aryString->push_back('[');
        if (!children.empty())
        {
            for (const auto &child : children)
            {
                aryString->append(child->ToString());
                aryString->push_back(',');
            }
            aryString->pop_back();  // remove last ','
        }
        aryString->push_back(']');
    }

    return *aryString;
}
