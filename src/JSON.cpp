//
// Created by qife on 16/1/27.
//

#include "JSON.hpp"

using namespace json;


inline static const char *SkipWhiteSpace(const char *str)
{
    while (std::isspace(*str)) {
        ++str;
    }
    return str;
}

inline static const char *SkipWhiteSpace(const char *str, unsigned int start)
{
    str += start;
    return SkipWhiteSpace(str);
}

inline static bool AssertEqual(char a, char b)
{
    assert(a == b);
    return a == b;
}

inline static bool Assert(bool b)
{
    assert(b);
    return b;
}

inline static bool AssertEndStr(const char *str)
{
    str = SkipWhiteSpace(str);
    assert(*str == '\0');
    return *str == '\0';
}

static int Unicode2ToUtf8(unsigned short unicodeChar, char *utf8Str)
{
    if (unicodeChar <= 0x007f) {
        // * U-00000000 - U-0000007F:  0xxxxxxx
        *utf8Str = static_cast<char>(unicodeChar);
        return 1;
    }

    if (unicodeChar <= 0x07ff) {
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
        utf8Str[1] = static_cast<char>((unicodeChar & 0x3f) | 0x80);
        utf8Str[0] = static_cast<char>(((unicodeChar >> 6) & 0x1f) | 0xc0);
        return 2;
    }

    // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
    utf8Str[2] = static_cast<char>((unicodeChar & 0x3f) | 0x80);
    utf8Str[1] = static_cast<char>(((unicodeChar >> 6) & 0x3f) | 0x80);
    utf8Str[0] = static_cast<char>(((unicodeChar >> 12) & 0x0f) | 0xe0);
    return 3;
}
