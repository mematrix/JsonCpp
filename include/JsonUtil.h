//
// Created by qife on 16/1/11.
//

#ifndef CPPPARSER_JSONUTIL_H
#define CPPPARSER_JSONUTIL_H

#include <locale>

#include "JValueType.h"
#include "JsonException.h"

#define MAKE_VALUE_TYPE_STR(infoStr) Invalid character when parse json##infoStr

#define MAKE_STR(str) #str

#define MAKE_VALUE_TYPE_INFO(type, infoStr) \
    template<> struct Info<type> { static const char*GetInfo() { return MAKE_STR(MAKE_VALUE_TYPE_STR(infoStr)); } }

namespace JsonCpp
{
    template<JValueType type>
    struct Info
    {
        static const char *GetInfo() { return "Error when parse json value"; }
    };

    MAKE_VALUE_TYPE_INFO(JValueType::object, _object);

    MAKE_VALUE_TYPE_INFO(JValueType::array, _array);

    struct JsonUtil
    {
        static const char *SkipWhiteSpace(const char *str)
        {
            while (std::isspace(*str))
            {
                ++str;
            }
            return str;
        }

        static const char *SkipWhiteSpace(const char *str, unsigned int start)
        {
            str += start;

            return SkipWhiteSpace(str);
        }

        template<JValueType type = JValueType::null>
        static void AssertEqual(char a, char b)
        {
            if (a != b)
            {
                throw JsonException(Info<type>::GetInfo());
            }
        }

        static void Assert(bool b)
        {
            if (!b)
            {
                throw JsonException("Incorrect json string");
            }
        }

        static void AssertEndStr(const char *str)
        {
            str = SkipWhiteSpace(str);
            if (*str != '\0')
            {
                throw JsonException("Incorrect json string");
            }
        }


        static int Unicode2ToUtf8(unsigned short uChar, char *utf8Str)
        {
            if (uChar <= 0x007f)
            {
                // * U-00000000 - U-0000007F:  0xxxxxxx
                *utf8Str = static_cast<char>(uChar);
                return 1;
            }

            if (uChar <= 0x07ff)
            {
                // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
                utf8Str[1] = static_cast<char>((uChar & 0x3f) | 0x80);
                utf8Str[0] = static_cast<char>(((uChar >> 6) & 0x1f) | 0xc0);
                return 2;
            }

            // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
            utf8Str[2] = static_cast<char>((uChar & 0x3f) | 0x80);
            utf8Str[1] = static_cast<char>(((uChar >> 6) & 0x3f) | 0x80);
            utf8Str[0] = static_cast<char>(((uChar >> 12) & 0x0f) | 0xe0);
            return 3;
        }
    };
}

#endif //CPPPARSER_JSONUTIL_H
