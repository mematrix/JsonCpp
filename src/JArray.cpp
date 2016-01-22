//
// Created by qife on 16/1/12.
//

#include "JArray.h"

using namespace JsonCpp;

const char *JArray::Parse(const char *str)
{
    str = JsonUtil::SkipWhiteSpace(str);
    JsonUtil::AssertEqual<JValueType::Array>(*str, '[');
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

        JsonUtil::AssertEqual<JValueType::Array>(*str, ']');
        return str + 1;
    }
}

JValueType JArray::GetType() const
{
    return JValueType::Array;
}

JArray::operator bool() const
{
    return !children.empty();
}

JArray::operator double() const
{
    return children.size();
}

JArray::operator std::string() const
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

/*
const JToken* JArray::SelectToken(const std::string &path) const {
    auto str = path.c_str();
    unsigned long indexStart = 0;
    if(*str == '$'){
        if(*(str +1)!='['){
            return nullptr;
        }
        indexStart = 2;
    }
}*/

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

const std::string &JArray::ToFormatString() const
{
    if (nullptr == fmtString)
    {
        fmtString = new std::string();
        fmtString->append("[\n");
        if (!children.empty())
        {
            for (const auto &child : children)
            {
                for (const auto &c : child->ToFormatString())
                {
                    if (c == '\n')
                    {
                        fmtString->append("\n\t");
                    }
                    else
                    {
                        fmtString->push_back(c);
                    }
                }
                fmtString->append(",\n");
            }
            fmtString->pop_back();
            fmtString->pop_back();
            fmtString->push_back('\n');
        }
        fmtString->push_back(']');
    }

    return *fmtString;
}

void JArray::Reclaim() const
{
    if (aryString)
    {
        delete aryString;
        aryString = nullptr;
    }
    if (fmtString)
    {
        delete fmtString;
        fmtString = nullptr;
    }
}
