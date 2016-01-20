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
    JsonUtil::AssertEqual<JValueType::Object>(*str, '{');
    // empty object
    str = JsonUtil::SkipWhiteSpace(str, 1);
    if (*str == '}')
    {
        return str + 1;
    }

    while (true)
    {
        JsonUtil::AssertEqual<JValueType::Object>(*str, '\"');
        ++str;
        std::string key(JsonReader::ReadString(&str));

        str = JsonUtil::SkipWhiteSpace(str);
        JsonUtil::AssertEqual<JValueType::Object>(*str, ':');
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

JValueType JObject::GetType() const
{
    return JValueType::Object;
}

JObject::operator bool() const
{
    // or throw exception
    return !children.empty();
}

JObject::operator double() const
{
    return children.size();
}

JObject::operator std::string() const
{
    return ToString();
}

const JToken &JObject::operator[](const std::string &str) const
{
    return GetValue(str);
}

const JToken &JObject::operator[](unsigned long index) const
{
    return GetValue(index);
}

const JToken &JObject::GetValue(const std::string &str) const
{
    return *(children.at(str).get());
}

const JToken &JObject::GetValue(unsigned long) const
{
    throw JsonException("Access not support");
}

const std::string &JObject::ToString() const
{
    if (nullptr == objString)
    {
        objString = new std::string();
        objString->push_back('{');
        if (!children.empty())
        {
            for (const auto &child : children)
            {
                objString->push_back('\"');
                objString->append(child.first);
                objString->append("\":");
                objString->append(child.second->ToString());
                objString->push_back(',');
            }
            objString->pop_back();
        }
        objString->push_back('}');
    }

    return *objString;
}

const std::string &JObject::ToFormatString() const
{
    if (nullptr == fmtString)
    {
        fmtString = new std::string();
        fmtString->append("{\n");
        if (!children.empty())
        {
            for (const auto &child : children)
            {
                fmtString->append("\t\"");
                fmtString->append(child.first);
                fmtString->append("\" : ");
                for (const auto &c : child.second->ToFormatString())
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
        fmtString->push_back('}');
    }

    return *fmtString;
}

void JObject::Reclaim() const
{
    if (objString)
    {
        delete objString;
        objString = nullptr;
    }
    if (fmtString)
    {
        delete fmtString;
        fmtString = nullptr;
    }
}
