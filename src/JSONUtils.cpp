//
// Created by Charles on 2018/6/14.
//

#include <cstdint>
#include <cmath>
#include "JSONUtils.hpp"

int unicode_to_utf8(unsigned int unicode_char, char *utf8_str) noexcept
{
    if (unicode_char <= 0x007fu) {
        // * U-00000000 - U-0000007F:  0xxxxxxx
        *utf8_str = static_cast<char>(unicode_char);
        return 1;
    }

    if (unicode_char <= 0x07ffu) {
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
        utf8_str[1] = static_cast<char>((unicode_char & 0x3fu) | 0x80u);
        utf8_str[0] = static_cast<char>(((unicode_char >> 6u) & 0x1fu) | 0xc0u);
        return 2;
    }

    if (unicode_char <= 0x0000ffffu) {
        // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
        utf8_str[2] = static_cast<char>((unicode_char & 0x3fu) | 0x80u);
        utf8_str[1] = static_cast<char>(((unicode_char >> 6u) & 0x3fu) | 0x80u);
        utf8_str[0] = static_cast<char>(((unicode_char >> 12u) & 0x0fu) | 0xe0u);
        return 3;
    }

    // * U-00010000 - U-0010FFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    utf8_str[3] = static_cast<char>((unicode_char & 0x3fu) | 0x80u);
    utf8_str[2] = static_cast<char>(((unicode_char >> 6u) & 0x3fu) | 0x80u);
    utf8_str[1] = static_cast<char>(((unicode_char >> 12u) & 0x3fu) | 0x80u);
    utf8_str[0] = static_cast<char>(((unicode_char >> 18u) & 0x7u) | 0xf0u);
    return 4;
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

std::string json::read_json_string(const char **str, int *error, char quote)
{
    auto last_handle_pos = *str;
    bool escape = false;
    unsigned int count = 0;

    std::string ret;
    for (auto tmp = last_handle_pos; *tmp != quote; ++tmp) {
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
            }
            last_handle_pos = tmp + 1;

            switch (*tmp) {
                case '\'':  /* handle case that quote == '\'' */
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
                    uint16_t unicode_first;
                    if (!try_parse_hex_short(tmp + 1, unicode_first)) {
                        *error = STRING_UNICODE_SYNTAX_ERROR;
                        return std::string();
                    }
                    tmp += 4;
                    uint32_t unicode = unicode_first;
                    if (0xd800u <= unicode_first && unicode_first <= 0xdbffu) {
                        // unicode extended characters
                        uint16_t unicode_second;
                        if (tmp[1] != '\\' || tmp[2] != 'u' || !try_parse_hex_short(tmp + 3, unicode_second)) {
                            *error = STRING_UNICODE_SYNTAX_ERROR;
                            return std::string();
                        }

                        if (0xdc00u <= unicode_second && unicode_second <= 0xdfffu) {
                            unicode = (((unicode_first - 0xd800u) << 10u) | (unicode_second - 0xdc00u)) + 0x010000u;
                            tmp += 6;
                        } else {
                            *error = STRING_UNICODE_SYNTAX_ERROR;
                            return std::string();
                        }
                    }

                    char utf8[8];
                    auto len = unicode_to_utf8(unicode, utf8);
                    for (int i = 0; i < len; ++i) {
                        ret.push_back(utf8[i]);
                    }
                    last_handle_pos = tmp + 1;
                    continue;
                }
                default:
                    *error = STRING_ESCAPE_SYNTAX_ERROR;
                    return std::string();
            }
        }
        if (!json_assert(std::iscntrl(*tmp) == 0)) {
            *error = STRING_CONTROL_CHAR_SYNTAX_ERROR;
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


constexpr uint64_t const_exp(uint64_t b, int e)
{
    return e == 0 ? 1 : (e % 2 == 0 ? const_exp(b * b, e / 2) : const_exp(b * b, (e - 1) / 2) * b);
}

// 0x1999999999999999, 2^64 - 1 = 18446744073709551615
constexpr uint64_t MaxCriticalValue = static_cast<uint64_t>((const_exp(2, 64) - 1) / 10);       // NOLINT: compile time value.
// 0x0CCCCCCCCCCCCCCC, 2^63 = 9223372036854775808;
constexpr uint64_t NegMaxCriticalValue = static_cast<uint64_t>(const_exp(2, 63) / 10);          // NOLINT: compile time value.
constexpr uint64_t FastPathValue = const_exp(2, 53) - 1;                                        // NOLINT: compile time value.
constexpr double MaxDoubleCriticalValue = std::numeric_limits<double>::max() / 10.0;

constexpr double e[] = { // 1e-0...1e308: 309 * 8 bytes = 2472 bytes
        1e+0,
        1e+1, 1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7, 1e+8, 1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18, 1e+19, 1e+20,
        1e+21, 1e+22, 1e+23, 1e+24, 1e+25, 1e+26, 1e+27, 1e+28, 1e+29, 1e+30, 1e+31, 1e+32, 1e+33, 1e+34, 1e+35, 1e+36, 1e+37, 1e+38, 1e+39, 1e+40,
        1e+41, 1e+42, 1e+43, 1e+44, 1e+45, 1e+46, 1e+47, 1e+48, 1e+49, 1e+50, 1e+51, 1e+52, 1e+53, 1e+54, 1e+55, 1e+56, 1e+57, 1e+58, 1e+59, 1e+60,
        1e+61, 1e+62, 1e+63, 1e+64, 1e+65, 1e+66, 1e+67, 1e+68, 1e+69, 1e+70, 1e+71, 1e+72, 1e+73, 1e+74, 1e+75, 1e+76, 1e+77, 1e+78, 1e+79, 1e+80,
        1e+81, 1e+82, 1e+83, 1e+84, 1e+85, 1e+86, 1e+87, 1e+88, 1e+89, 1e+90, 1e+91, 1e+92, 1e+93, 1e+94, 1e+95, 1e+96, 1e+97, 1e+98, 1e+99, 1e+100,
        1e+101, 1e+102, 1e+103, 1e+104, 1e+105, 1e+106, 1e+107, 1e+108, 1e+109, 1e+110, 1e+111, 1e+112, 1e+113, 1e+114, 1e+115, 1e+116, 1e+117, 1e+118, 1e+119, 1e+120,
        1e+121, 1e+122, 1e+123, 1e+124, 1e+125, 1e+126, 1e+127, 1e+128, 1e+129, 1e+130, 1e+131, 1e+132, 1e+133, 1e+134, 1e+135, 1e+136, 1e+137, 1e+138, 1e+139, 1e+140,
        1e+141, 1e+142, 1e+143, 1e+144, 1e+145, 1e+146, 1e+147, 1e+148, 1e+149, 1e+150, 1e+151, 1e+152, 1e+153, 1e+154, 1e+155, 1e+156, 1e+157, 1e+158, 1e+159, 1e+160,
        1e+161, 1e+162, 1e+163, 1e+164, 1e+165, 1e+166, 1e+167, 1e+168, 1e+169, 1e+170, 1e+171, 1e+172, 1e+173, 1e+174, 1e+175, 1e+176, 1e+177, 1e+178, 1e+179, 1e+180,
        1e+181, 1e+182, 1e+183, 1e+184, 1e+185, 1e+186, 1e+187, 1e+188, 1e+189, 1e+190, 1e+191, 1e+192, 1e+193, 1e+194, 1e+195, 1e+196, 1e+197, 1e+198, 1e+199, 1e+200,
        1e+201, 1e+202, 1e+203, 1e+204, 1e+205, 1e+206, 1e+207, 1e+208, 1e+209, 1e+210, 1e+211, 1e+212, 1e+213, 1e+214, 1e+215, 1e+216, 1e+217, 1e+218, 1e+219, 1e+220,
        1e+221, 1e+222, 1e+223, 1e+224, 1e+225, 1e+226, 1e+227, 1e+228, 1e+229, 1e+230, 1e+231, 1e+232, 1e+233, 1e+234, 1e+235, 1e+236, 1e+237, 1e+238, 1e+239, 1e+240,
        1e+241, 1e+242, 1e+243, 1e+244, 1e+245, 1e+246, 1e+247, 1e+248, 1e+249, 1e+250, 1e+251, 1e+252, 1e+253, 1e+254, 1e+255, 1e+256, 1e+257, 1e+258, 1e+259, 1e+260,
        1e+261, 1e+262, 1e+263, 1e+264, 1e+265, 1e+266, 1e+267, 1e+268, 1e+269, 1e+270, 1e+271, 1e+272, 1e+273, 1e+274, 1e+275, 1e+276, 1e+277, 1e+278, 1e+279, 1e+280,
        1e+281, 1e+282, 1e+283, 1e+284, 1e+285, 1e+286, 1e+287, 1e+288, 1e+289, 1e+290, 1e+291, 1e+292, 1e+293, 1e+294, 1e+295, 1e+296, 1e+297, 1e+298, 1e+299, 1e+300,
        1e+301, 1e+302, 1e+303, 1e+304, 1e+305, 1e+306, 1e+307, 1e+308
};

constexpr double pow10(int32_t n)
{
    // assert(n >= 0 && n <= 308);
    return e[n];
}

static double strtod_normal_precision(double d, int32_t p)
{
    if (p < -308 - 308) {
        return 0.0;
    }
    if (p < -308) {
        d = d / pow10(308);
        return d / pow10(-(p + 308));
    }
    if (p < 0) {
        return d / pow10(-p);
    }
    return d * pow10(p);
}

// inspired by Google/double-conversion & Tencent/RapidJSON
bool json::read_json_number(const char **number_str, int *error, number_union &number)
{
    auto str = *number_str;
    bool is_negative = false;
    if (*str == '-') {
        is_negative = true;
        ++str;
    }

    bool use_double = false;
    double d = 0.0;
    uint64_t base_number = 0;
    int32_t significant_digit = 0;
    if (*str == '0') {
        ++str;
    } else if (*str >= '1' && *str <= '9') {
        base_number = static_cast<uint64_t>(*str - '0');
        ++str;

        if (is_negative) {
            while (*str >= '0' && *str <= '9') {
                if (base_number >= NegMaxCriticalValue) {
                    if (base_number != NegMaxCriticalValue || str[1] > '8') {
                        d = static_cast<double>(base_number);
                        use_double = true;
                        break;
                    }
                }

                base_number = base_number * 10 + static_cast<uint64_t>(*str - '0');
                ++significant_digit;
                ++str;
            }
        } else {
            while (*str >= '0' && *str <= '9') {
                if (base_number >= MaxCriticalValue) {
                    if (base_number != MaxCriticalValue || str[1] > '5') {
                        d = static_cast<double>(base_number);
                        use_double = true;
                        break;
                    }
                }

                base_number = base_number * 10 + static_cast<uint64_t>(*str - '0');
                ++significant_digit;
                ++str;
            }
        }
    } else {
        *error = UNEXPECTED_TOKEN;
        return false;
    }

    if (use_double) {
        // force use double for big integer number
        while (*str >= '0' && *str <= '9') {
            if (d >= MaxDoubleCriticalValue) {
                *error = NUMBER_FLOAT_OVERFLOW;
                return false;
            }
            d = d * 10 + (*str - '0');
            ++significant_digit;
            ++str;
        }
    }

    int32_t exp_fraction = 0;
    if (*str == '.') {
        ++str;
        if (*str < '0' || *str > '9') {
            // error syntax: not like c/c++, there must be a digit after `digit.`.
            *error = NUMBER_FRACTION_FORMAT_ERROR;
            return false;
        }

        if (!use_double) {
            while (*str >= '0' && *str <= '9') {
                if (base_number > FastPathValue) {
                    break;
                } else {
                    base_number = base_number * 10 + static_cast<uint64_t>(*str - '0');
                    ++str;
                    --exp_fraction;
                    if (0 != base_number) {
                        ++significant_digit;
                    }
                }
            }

            d = static_cast<double>(base_number);
            use_double = true;
        }

        while (*str >= '0' && *str <= '9') {
            // precision of double is 16.
            if (significant_digit < 17) {
                d = d * 10 + (*str - '0');
                --exp_fraction;
                if (d > 0.0) {
                    ++significant_digit;
                }
            }

            ++str;
        }
    }

    int32_t exponent = 0;
    if (*str == 'e' || *str == 'E') {
        // exponent part
        ++str;

        if (!use_double) {
            d = static_cast<double>(base_number);
            use_double = true;
        }

        bool is_negative_exp = false;
        if (*str == '-') {
            is_negative_exp = true;
            ++str;
        } else if (*str == '+') {
            ++str;
        }

        if (*str < '0' || *str > '9') {
            *error = NUMBER_EXPONENT_FORMAT_ERROR;
            return false;
        }

        exponent = *str - '0';
        ++str;
        if (is_negative_exp) {
            constexpr int32_t MinIntMinus9 = -(std::numeric_limits<int32_t>::min() + 9);
            int32_t max_exp = (exp_fraction + MinIntMinus9) / 10;

            while (*str >= '0' && *str <= '9') {
                exponent = exponent * 10 + (*str - '0');
                ++str;

                if (exponent > max_exp) {
                    while (*str >= '0' && *str <= '9') {
                        ++str;
                    }
                }
            }
        } else {
            // positive exponent;
            int32_t max_exp = std::numeric_limits<double>::max_exponent10 - exp_fraction - significant_digit;
            if (exponent > max_exp) {
                *error = NUMBER_FLOAT_OVERFLOW;
                return false;
            }

            while (*str >= '0' && *str <= '9') {
                exponent = exponent * 10 + (*str - '0');
                ++str;
                if (exponent > max_exp) {
                    *error = NUMBER_FLOAT_OVERFLOW;
                    return false;
                }
            }
        }

        if (is_negative_exp) {
            exponent = -exponent;
        }
    }

    *number_str = str;
    if (use_double) {
        int p = exponent + exp_fraction;
        d = strtod_normal_precision(d, p);
        number.float_value = is_negative ? -d : d;
        return true;
    } else {
        number.int_value = static_cast<int64_t>(is_negative ? (~base_number + 1) : base_number);
        return false;
    }
}
