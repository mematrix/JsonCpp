//
// Created by Charles on 2018/6/14.
//

#include <cstdint>
#include <cmath>
#include <cstring>
#include "JSONUtils.hpp"
#include "FloatNumUtils.hpp"

// String parse begin

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

// String parse end


// Number parse begin

// 0x1999999999999999, 2^64 - 1 = 18446744073709551615
constexpr uint64_t MaxCriticalValue = static_cast<uint64_t>((const_exp(2, 64) - 1) / 10);       // NOLINT: compile time value.
// 0x0CCCCCCCCCCCCCCC, 2^63 = 9223372036854775808;
constexpr uint64_t NegMaxCriticalValue = static_cast<uint64_t>(const_exp(2, 63) / 10);          // NOLINT: compile time value.
constexpr uint64_t FastPathValue = const_exp(2, 53) - 1;                                        // NOLINT: compile time value.
constexpr double MaxDoubleCriticalValue = std::numeric_limits<double>::max() / 10.0;

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
    int32_t significand_digit = 0;
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
                ++significand_digit;
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
                ++significand_digit;
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
            ++significand_digit;
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
                        ++significand_digit;
                    }
                }
            }

            d = static_cast<double>(base_number);
            use_double = true;
        }

        while (*str >= '0' && *str <= '9') {
            // precision of double is 16.
            if (significand_digit < 17) {
                d = d * 10 + (*str - '0');
                --exp_fraction;
                if (d > 0.0) {
                    ++significand_digit;
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
            int32_t max_exp = std::numeric_limits<double>::max_exponent10 - exp_fraction - significand_digit;
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

// Number parse end

// Number format begin

static const char digits_lut[200] = {
        '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
        '1', '0', '1', '1', '1', '2', '1', '3', '1', '4', '1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
        '2', '0', '2', '1', '2', '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
        '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
        '4', '0', '4', '1', '4', '2', '4', '3', '4', '4', '4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
        '5', '0', '5', '1', '5', '2', '5', '3', '5', '4', '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
        '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
        '7', '0', '7', '1', '7', '2', '7', '3', '7', '4', '7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
        '8', '0', '8', '1', '8', '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
        '9', '0', '9', '1', '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9', '7', '9', '8', '9', '9'
};

inline const char *get_digits_lut()
{
    return digits_lut;
}

static char *write_exponent(int K, char *buffer)
{
    if (K < 0) {
        *buffer++ = '-';
        K = -K;
    }

    if (K >= 100) {
        *buffer++ = '0' + static_cast<char>(K / 100);
        K %= 100;
        const char *d = get_digits_lut() + K * 2;
        *buffer++ = d[0];
        *buffer++ = d[1];
    } else if (K >= 10) {
        const char *d = get_digits_lut() + K * 2;
        *buffer++ = d[0];
        *buffer++ = d[1];
    } else {
        *buffer++ = '0' + static_cast<char>(K);
    }

    return buffer;
}

static char *prettify(char *buffer, int length, int k, int max_decimal_places)
{
    const int kk = length + k;  // 10^(kk-1) <= v < 10^kk

    if (0 <= k && kk <= 21) {
        // 1234e7 -> 12340000000
        for (int i = length; i < kk; i++) {
            buffer[i] = '0';
        }
        buffer[kk] = '.';
        buffer[kk + 1] = '0';
        return &buffer[kk + 2];
    } else if (0 < kk && kk <= 21) {
        // 1234e-2 -> 12.34
        std::memmove(&buffer[kk + 1], &buffer[kk], static_cast<size_t>(length - kk));
        buffer[kk] = '.';
        if (0 > k + max_decimal_places) {
            // When max_decimal_places = 2, 1.2345 -> 1.23, 1.102 -> 1.1
            // Remove extra trailing zeros (at least one) after truncation.
            for (int i = kk + max_decimal_places; i > kk + 1; --i) {
                if (buffer[i] != '0') {
                    return &buffer[i + 1];
                }
            }
            return &buffer[kk + 2]; // Reserve one zero
        } else {
            return &buffer[length + 1];
        }
    } else if (-6 < kk && kk <= 0) {
        // 1234e-6 -> 0.001234
        const int offset = 2 - kk;
        std::memmove(&buffer[offset], &buffer[0], static_cast<size_t>(length));
        buffer[0] = '0';
        buffer[1] = '.';
        for (int i = 2; i < offset; i++) {
            buffer[i] = '0';
        }
        if (length - kk > max_decimal_places) {
            // When max_decimal_places = 2, 0.123 -> 0.12, 0.102 -> 0.1
            // Remove extra trailing zeros (at least one) after truncation.
            for (int i = max_decimal_places + 1; i > 2; --i) {
                if (buffer[i] != '0') {
                    return &buffer[i + 1];
                }
            }
            return &buffer[3]; // Reserve one zero
        } else {
            return &buffer[length + offset];
        }
    } else if (kk < -max_decimal_places) {
        // Truncate to zero
        buffer[0] = '0';
        buffer[1] = '.';
        buffer[2] = '0';
        return &buffer[3];
    } else if (length == 1) {
        // 1e30
        buffer[1] = 'e';
        return write_exponent(kk - 1, &buffer[2]);
    } else {
        // 1234e30 -> 1.234e33
        std::memmove(&buffer[2], &buffer[1], static_cast<size_t>(length - 1));
        buffer[1] = '.';
        buffer[length + 1] = 'e';
        return write_exponent(kk - 1, &buffer[0 + length + 2]);
    }
}

char *json::dtoa(double value, char *buffer, int max_decimal_places)
{
    union
    {
        double d;
        uint64_t i;
    } u = {value};

    if (is_zero_double(u.i)) {
        if (is_sign_double(u.i)) {
            *buffer++ = '-';
        }     // -0.0, Issue #289
        buffer[0] = '0';
        buffer[1] = '.';
        buffer[2] = '0';
        return &buffer[3];
    } else {
        if (value < 0) {
            *buffer++ = '-';
            value = -value;
        }
        int length, K;
        grisu2(value, buffer, &length, &K);
        return prettify(buffer, length, K, max_decimal_places);
    }
}

static char *u64toa(uint64_t value, char *buffer)
{
    const char *const_digits_lut = get_digits_lut();
    constexpr uint64_t ten_8 = 100000000;
    constexpr uint64_t ten_9 = ten_8 * 10;
    constexpr uint64_t ten_10 = ten_8 * 100;
    constexpr uint64_t ten_11 = ten_8 * 1000;
    constexpr uint64_t ten_12 = ten_8 * 10000;
    constexpr uint64_t ten_13 = ten_8 * 100000;
    constexpr uint64_t ten_14 = ten_8 * 1000000;
    constexpr uint64_t ten_15 = ten_8 * 10000000;
    constexpr uint64_t ten_16 = ten_8 * ten_8;

    if (value < ten_8) {
        auto v = static_cast<uint32_t>(value);
        if (v < 10000) {
            const uint32_t d1 = (v / 100) << 1u;
            const uint32_t d2 = (v % 100) << 1u;

            if (v >= 1000) {
                *buffer++ = const_digits_lut[d1];
            }
            if (v >= 100) {
                *buffer++ = const_digits_lut[d1 + 1];
            }
            if (v >= 10) {
                *buffer++ = const_digits_lut[d2];
            }
            *buffer++ = const_digits_lut[d2 + 1];
        } else {
            // value = bbbbcccc
            const uint32_t b = v / 10000;
            const uint32_t c = v % 10000;

            const uint32_t d1 = (b / 100) << 1u;
            const uint32_t d2 = (b % 100) << 1u;

            const uint32_t d3 = (c / 100) << 1u;
            const uint32_t d4 = (c % 100) << 1u;

            if (value >= 10000000) {
                *buffer++ = const_digits_lut[d1];
            }
            if (value >= 1000000) {
                *buffer++ = const_digits_lut[d1 + 1];
            }
            if (value >= 100000) {
                *buffer++ = const_digits_lut[d2];
            }
            *buffer++ = const_digits_lut[d2 + 1];

            *buffer++ = const_digits_lut[d3];
            *buffer++ = const_digits_lut[d3 + 1];
            *buffer++ = const_digits_lut[d4];
            *buffer++ = const_digits_lut[d4 + 1];
        }
    } else if (value < ten_16) {
        const auto v0 = static_cast<uint32_t>(value / ten_8);
        const auto v1 = static_cast<uint32_t>(value % ten_8);

        const uint32_t b0 = v0 / 10000;
        const uint32_t c0 = v0 % 10000;

        const uint32_t d1 = (b0 / 100) << 1u;
        const uint32_t d2 = (b0 % 100) << 1u;

        const uint32_t d3 = (c0 / 100) << 1u;
        const uint32_t d4 = (c0 % 100) << 1u;

        const uint32_t b1 = v1 / 10000;
        const uint32_t c1 = v1 % 10000;

        const uint32_t d5 = (b1 / 100) << 1u;
        const uint32_t d6 = (b1 % 100) << 1u;

        const uint32_t d7 = (c1 / 100) << 1u;
        const uint32_t d8 = (c1 % 100) << 1u;

        if (value >= ten_15) {
            *buffer++ = const_digits_lut[d1];
        }
        if (value >= ten_14) {
            *buffer++ = const_digits_lut[d1 + 1];
        }
        if (value >= ten_13) {
            *buffer++ = const_digits_lut[d2];
        }
        if (value >= ten_12) {
            *buffer++ = const_digits_lut[d2 + 1];
        }
        if (value >= ten_11) {
            *buffer++ = const_digits_lut[d3];
        }
        if (value >= ten_10) {
            *buffer++ = const_digits_lut[d3 + 1];
        }
        if (value >= ten_9) {
            *buffer++ = const_digits_lut[d4];
        }
        if (value >= ten_8) {
            *buffer++ = const_digits_lut[d4 + 1];
        }

        *buffer++ = const_digits_lut[d5];
        *buffer++ = const_digits_lut[d5 + 1];
        *buffer++ = const_digits_lut[d6];
        *buffer++ = const_digits_lut[d6 + 1];
        *buffer++ = const_digits_lut[d7];
        *buffer++ = const_digits_lut[d7 + 1];
        *buffer++ = const_digits_lut[d8];
        *buffer++ = const_digits_lut[d8 + 1];
    } else {
        const auto a = static_cast<uint32_t>(value / ten_16); // 1 to 1844
        value %= ten_16;

        if (a < 10) {
            *buffer++ = '0' + static_cast<char>(a);
        } else if (a < 100) {
            const uint32_t i = a << 1u;
            *buffer++ = const_digits_lut[i];
            *buffer++ = const_digits_lut[i + 1];
        } else if (a < 1000) {
            *buffer++ = '0' + static_cast<char>(a / 100);

            const uint32_t i = (a % 100) << 1u;
            *buffer++ = const_digits_lut[i];
            *buffer++ = const_digits_lut[i + 1];
        } else {
            const uint32_t i = (a / 100) << 1u;
            const uint32_t j = (a % 100) << 1u;
            *buffer++ = const_digits_lut[i];
            *buffer++ = const_digits_lut[i + 1];
            *buffer++ = const_digits_lut[j];
            *buffer++ = const_digits_lut[j + 1];
        }

        const auto v0 = static_cast<uint32_t>(value / ten_8);
        const auto v1 = static_cast<uint32_t>(value % ten_8);

        const uint32_t b0 = v0 / 10000;
        const uint32_t c0 = v0 % 10000;

        const uint32_t d1 = (b0 / 100) << 1u;
        const uint32_t d2 = (b0 % 100) << 1u;

        const uint32_t d3 = (c0 / 100) << 1u;
        const uint32_t d4 = (c0 % 100) << 1u;

        const uint32_t b1 = v1 / 10000;
        const uint32_t c1 = v1 % 10000;

        const uint32_t d5 = (b1 / 100) << 1u;
        const uint32_t d6 = (b1 % 100) << 1u;

        const uint32_t d7 = (c1 / 100) << 1u;
        const uint32_t d8 = (c1 % 100) << 1u;

        *buffer++ = const_digits_lut[d1];
        *buffer++ = const_digits_lut[d1 + 1];
        *buffer++ = const_digits_lut[d2];
        *buffer++ = const_digits_lut[d2 + 1];
        *buffer++ = const_digits_lut[d3];
        *buffer++ = const_digits_lut[d3 + 1];
        *buffer++ = const_digits_lut[d4];
        *buffer++ = const_digits_lut[d4 + 1];
        *buffer++ = const_digits_lut[d5];
        *buffer++ = const_digits_lut[d5 + 1];
        *buffer++ = const_digits_lut[d6];
        *buffer++ = const_digits_lut[d6 + 1];
        *buffer++ = const_digits_lut[d7];
        *buffer++ = const_digits_lut[d7 + 1];
        *buffer++ = const_digits_lut[d8];
        *buffer++ = const_digits_lut[d8 + 1];
    }

    return buffer;
}

char *json::i64toa(int64_t value, char *buffer)
{
    auto u = static_cast<uint64_t>(value);
    if (value < 0) {
        *buffer++ = '-';
        u = ~u + 1;
    }

    return u64toa(u, buffer);
}
