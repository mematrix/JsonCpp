//
// Created by Charles on 2018/6/14.
//

#ifndef CPPPARSER_JSONUTILS_H
#define CPPPARSER_JSONUTILS_H

#include <cctype>
#include <limits>
#include <cassert>
#include <string>

inline static const char *skip_whitespace(const char *str)
{
    while (std::isspace(*str)) {
        ++str;
    }
    return str;
}

inline static const char *skip_whitespace(const char *str, unsigned int start)
{
    str += start;
    return skip_whitespace(str);
}

int unicode_to_utf8(unsigned short unicode_char, char *utf8_str) noexcept;

namespace json {

enum
{
    NO_ERROR = 0,
    STRING_PARSE_ERROR,
    STRING_SYNTAX_ERROR,
    OBJECT_PARSE_ERROR,
    OBJECT_SYNTAX_ERROR,
    OBJECT_DUPLICATED_KEY,
    ARRAY_PARSE_ERROR,
    NUMBER_FORMAT_ERROR,
    NUMBER_INT_OVERFLOW,
    NUMBER_FLOAT_OVERFLOW,
    NUMBER_FLOAT_UNDERFLOW,
    UNEXPECTED_TOKEN,
    UNEXPECTED_END_CHAR
};

union number_union
{
    int64_t int_value;
    double float_value;
};

constexpr static int64_t MaxCriticalValue = std::numeric_limits<int64_t>::max() / 10 - 1;


inline static bool assert_equal(char a, char b)
{
    assert(a == b);
    return a == b;
}

inline static bool json_assert(bool b)
{
    assert(b);
    return b;
}

inline static bool assert_end_str(const char *str)
{
    str = skip_whitespace(str);
    assert(*str == '\0');
    return *str == '\0';
}

/**
 * read json string type value. for string pattern detail see http://www.json.org/index.html
 * @param str in out param, c-style utf8 string.
 * @param error out param, if an error occurs, which value will be set.
 * @return parsed value. or empty string if error occurs.
 */
std::string read_json_string(const char **str, int *error);

/**
 * parse number type string. for number pattern detail see http://www.json.org/index.html
 * @param number_str in out param, c-style utf8 string.
 * @param error out param. if an error occurs, which value will be set.
 * @param number a union type param, value type can be determined by the return value of this function.
 * @return indicate the type of number. true for float while false for integer. if an error
 * occurs, always return false.
 */
bool read_json_number(const char **number_str, int *error, number_union &number);

}

#endif //CPPPARSER_JSONUTILS_H
