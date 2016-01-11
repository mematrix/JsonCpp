//
// Created by qife on 16/1/11.
//

#ifndef CPPPARSER_JSONUTIL_H
#define CPPPARSER_JSONUTIL_H

#include <locale>

#include "JValueType.h"
#include "JsonException.h"

#define MAKE_VALUE_TYPE_INFO(type, infoStr) \
    template<> struct Info<type> { static const char*GetInfo() { return "Invalid character when parse json "##infoStr; } }

namespace JsonCpp
{
    template<JValueType type>
    struct Info
    {
        static const char *GetInfo() { return "Unknown"; }
    };

    MAKE_VALUE_TYPE_INFO(JValueType::object, "object");

    MAKE_VALUE_TYPE_INFO(JValueType::array, "array");

    class JsonUtil
    {
    public:
        static const char *SkipWhiteSpace(const char *str, unsigned int start = 0)
        {
            str += start;

            while (std::isspace(*str))
            {
                ++str;
            }
            return str;
        }

        template<JValueType type>
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

    };
}

#endif //CPPPARSER_JSONUTIL_H
