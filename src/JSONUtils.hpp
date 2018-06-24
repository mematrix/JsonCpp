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

int unicode_to_utf8(unsigned int unicode_char, char *utf8_str) noexcept;

namespace json {

enum
{
    NO_ERROR = 0,
    STRING_PARSE_ERROR,
    STRING_UNICODE_SYNTAX_ERROR,
    STRING_ESCAPE_SYNTAX_ERROR,
    STRING_CONTROL_CHAR_SYNTAX_ERROR,
    OBJECT_PARSE_ERROR,
    OBJECT_KEY_SYNTAX_ERROR,
    OBJECT_KV_SYNTAX_ERROR,
    OBJECT_DUPLICATED_KEY,
    ARRAY_PARSE_ERROR,
    NUMBER_FRACTION_FORMAT_ERROR,
    NUMBER_EXPONENT_FORMAT_ERROR,
    NUMBER_FLOAT_OVERFLOW,
    UNEXPECTED_TOKEN,
    UNEXPECTED_END_CHAR
};

union number_union
{
    int64_t int_value;
    double float_value;
};


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
 * @param quote in param, quote character used in string syntax.
 * @return parsed value. or empty string if error occurs.
 */
std::string read_json_string(const char **str, int *error, char quote = '\"');

/**
 * parse number type string. for number pattern detail see http://www.json.org/index.html
 * @param number_str in out param, c-style utf8 string.
 * @param error out param. if an error occurs, which value will be set.
 * @param number a union type param, value type can be determined by the return value of this function.
 * @return indicate the type of number. true for float while false for integer. if an error
 * occurs, always return false.
 */
bool read_json_number(const char **number_str, int *error, number_union &number);

/**
 * double to ascii string. buffer size must be greater than 25.
 * @param value double value to format.
 * @param buffer buffer to place the string (not include the end '\0').
 * @param max_decimal_places max decimal count after point.
 * @return pointer to the next character in the buffer after the format-string.
 */
char* dtoa(double value, char* buffer, int max_decimal_places = 324);

/**
 * int64 value to ascii string. buffer size should be enough to place the format-string (not include '\0').
 * @param value 64-bit integer value to format.
 * @param buffer buffer to place the format-string.
 * @return pointer to the next character in the buffer after the format-string.
 */
char* i64toa(int64_t value, char* buffer);

}

#endif //CPPPARSER_JSONUTILS_H
