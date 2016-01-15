//
// Created by qife on 16/1/15.
//

#include "JsonReader.h"
#include "JObject.h"
#include "JArray.h"
#include "JValue.h"

using namespace JsonCpp;

std::unique_ptr<JToken> JsonReader::ReadToken(const char **str)
{
    auto tmp = JsonUtil::SkipWhiteSpace(*str);
    if (*tmp == '{')
    {
        auto obj = new JObject();
        std::unique_ptr<JToken> pt(obj);
        *str = obj->Parse(tmp);
        return pt;
    }

    if (*tmp == '[')
    {
        auto ary = new JArray();
        std::unique_ptr<JToken> pt(ary);
        *str = ary->Parse(tmp);
        return pt;
    }

    auto val = new JValue();
    std::unique_ptr<JToken> pt(val);
    *str = val->Parse(tmp);
    return pt;
}

std::string JsonReader::ReadString(const char **str)
{
    auto lst = *str;
    bool escape = false;
    unsigned int count = 0;

    std::string ret;
    for (auto tmp = lst; *tmp != '\"'; ++tmp)
    {
        if (*tmp == '\\')
        {
            escape = true;
            ++tmp;
        }
        if (escape)
        {
            escape = false;
            if (count != 0)
            {
                ret.append(lst, count);
                count = 0;
                lst = tmp + 1;
            }
            switch (*tmp)
            {
                case '\"':
                case '\\':
                case '/':
                    ret.push_back(*tmp);
                    continue;
                case 'b':
                    ret.push_back('\b');
                    continue;
                case 'f':
                    ret.push_back('\f');
                    continue;
                case 'n':
                    ret.push_back('\n');
                    continue;
                case 'r':
                    ret.push_back('\r');
                    continue;
                case 't':
                    ret.push_back('\t');
                    continue;
                case 'u':
                    char *end;
                    auto unicode = (unsigned short) std::strtoul(tmp + 1, &end, 16);
                    JsonUtil::Assert(end == tmp + 5);

                    char utf8[3];
                    auto len = JsonUtil::Unicode2ToUtf8(unicode, utf8);
                    for (int i = 0; i < len; ++i)
                    {
                        ret.push_back(utf8[i]);
                    }
                    tmp += 4;
                    lst = tmp + 1;
                    continue;
                default:
                    throw JsonException("Illegal character");
            }
        }
        JsonUtil::Assert(std::iscntrl(*tmp) != 0);
        ++count;
    }

    if (count != 0)
    {
        ret.append(lst, count);
    }

    *str = lst + count + 1;
    return ret;
}
