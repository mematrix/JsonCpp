//
// Created by Charles on 2018/6/14.
//

#include <cstdint>
#include <cmath>
#include "JSONUtils.hpp"

int unicode_to_utf8(unsigned short unicode_char, char *utf8_str) noexcept
{
    if (unicode_char <= 0x007f) {
        // * U-00000000 - U-0000007F:  0xxxxxxx
        *utf8_str = static_cast<char>(unicode_char);
        return 1;
    }

    if (unicode_char <= 0x07ff) {
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
        utf8_str[1] = static_cast<char>((unicode_char & 0x3f) | 0x80);
        utf8_str[0] = static_cast<char>(((unicode_char >> 6) & 0x1f) | 0xc0);
        return 2;
    }

    // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
    utf8_str[2] = static_cast<char>((unicode_char & 0x3f) | 0x80);
    utf8_str[1] = static_cast<char>(((unicode_char >> 6) & 0x3f) | 0x80);
    utf8_str[0] = static_cast<char>(((unicode_char >> 12) & 0x0f) | 0xe0);
    return 3;
}


static bool try_parse_hex_short(const char *str, uint16_t &result) noexcept
{
    uint32_t r = 0;
    for (int i = 3; i >= 0; --i) {
        uint32_t v;
        if (*str >= 'a' && *str <= 'f') {
            v = static_cast<uint32_t>(*str - 'a' + 10);
        } else if (*str >= 'A' && *str <= 'F') {
            v = static_cast<uint32_t>(*str - 'A' + 10);
        } else if (*str >= '0' && *str <= '9') {
            v = static_cast<uint32_t >(*str - '0');
        } else {
            return false;
        }

        r = r | (v << static_cast<uint32_t >(i * 4));
        ++str;
    }

    result = static_cast<uint16_t>(r);
    return true;
}

std::string json::read_json_string(const char **str, int *error)
{
    auto last_handle_pos = *str;
    bool escape = false;
    unsigned int count = 0;

    std::string ret;
    for (auto tmp = last_handle_pos; *tmp != '\"'; ++tmp) {
        if (*tmp == '\\') {
            escape = true;
            ++tmp;
        }
        if (*tmp == 0) {
            *error = STRING_PARSE_ERROR;
            return std::string();
        }

        if (escape) {
            escape = false;
            if (count != 0) {
                ret.append(last_handle_pos, count);
                count = 0;
                last_handle_pos = tmp + 1;
            }

            switch (*tmp) {
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
                case 'u': {
                    uint16_t unicode;
                    if (!try_parse_hex_short(tmp + 1, unicode)) {
                        *error = STRING_SYNTAX_ERROR;
                        return std::string();
                    }

                    char utf8[3];
                    auto len = unicode_to_utf8(unicode, utf8);
                    for (int i = 0; i < len; ++i) {
                        ret.push_back(utf8[i]);
                    }
                    tmp += 4;
                    last_handle_pos = tmp + 1;
                    continue;
                }
                default:
                    *error = STRING_SYNTAX_ERROR;
                    return std::string();
            }
        }
        if (!json_assert(std::iscntrl(*tmp) == 0)) {
            *error = STRING_SYNTAX_ERROR;
            return std::string();
        }

        ++count;
    }

    if (count != 0) {
        ret.append(last_handle_pos, count);
    }

    *str = last_handle_pos + count + 1;
    return ret;
}

bool json::read_json_number(const char **number_str, int *error, number_union &number)
{
    auto str = *number_str;
    bool is_negative = false;
    if (*str == '-') {
        is_negative = true;
        ++str;
    }

    bool is_float = false;
    double float_base_number = 0.0;
    int64_t base_number = 0;
    int32_t exponent = 0;
    int32_t integer_part_count = 0;   // integer part number count

    if (*str == '0') {
        ++str;
    } else if (*str >= '1' && *str <= '9') {
        base_number = *str - '0';
        ++str;
        while (*str >= '0' && *str <= '9') {
            if (base_number > MaxCriticalValue) {
                // will overflow
                *error = NUMBER_INT_OVERFLOW;
                return false;
            }
            base_number = base_number * 10 + (*str - '0');
            ++integer_part_count;
            ++str;
        }
    } else {
        *error = UNEXPECTED_TOKEN;
        return false;
    }

    if (*str == '.') {
        ++str;
        if (*str < '0' || *str > '9') {
            // error syntax: not like c/c++, there must be a digit after `digit.`.
            *error = NUMBER_FORMAT_ERROR;
            return false;
        }

        is_float = true;
        float_base_number = base_number;
        int precision_base = 10;
        do {
            double v = *str - '0';
            v /= precision_base;
            float_base_number += v;
            if (precision_base < 1e17) {
                // precision of double is 16. if value is greater than 1e17, fractional part will be dropped.
                // ignore case that `float_base_number` itself is big.
                precision_base *= 10;
            }

            ++str;
        } while (*str >= '0' && *str <= '9');
    }

    if (*str == 'e' || *str == 'E') {
        // exponent part
        ++str;
        bool is_negative_exp = false;
        if (*str == '-') {
            is_negative_exp = true;
            ++str;
        } else if (*str == '+') {
            ++str;
        }

        if (*str < '0' || *str > '9') {
            *error = NUMBER_FORMAT_ERROR;
            return false;
        }

        do {
            int v = *str - '0';
            exponent = exponent * 10 + v;
            int actual_exp_value = (is_negative_exp ? -exponent : exponent) + integer_part_count;
            if (actual_exp_value + 1 > std::numeric_limits<double>::max_exponent10) {
                // overflow or underflow
                *error = NUMBER_FLOAT_OVERFLOW;
                return false;
            } else if (actual_exp_value - 1 < std::numeric_limits<double>::min_exponent10) {
                *error = NUMBER_FLOAT_UNDERFLOW;
                return false;
            }

            ++str;
        } while (*str >= '0' && *str <= '9');

        if (is_negative_exp) {
            exponent = -exponent;
        }
    }

    *number_str = str;
    if (is_negative) {
        if (is_float) {
            float_base_number = -float_base_number;
        } else {
            base_number = -base_number;
        }
    }

    if (exponent != 0) {
        number.float_value = (is_float ? float_base_number : (double)base_number) * std::pow(10, exponent);
        return true;
    }
    if (is_float) {
        number.float_value = float_base_number;
        return true;
    }
    number.int_value = base_number;
    return false;
}
