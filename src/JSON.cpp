//
// Created by Charles on 2018/1/18.
//

#include <iostream>
#include <limits>
#include <cassert>
#include <cstring>
#include <cmath>
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

static int Unicode2ToUtf8(unsigned short unicodeChar, char *utf8Str) noexcept
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

static bool TryParseHexShort(const char *str, uint16_t &result) noexcept
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

        r = r | (v << (i * 4));
        ++str;
    }

    result = static_cast<uint16_t>(r);
    return true;
}

/**
 * read json string type value. for string pattern detail see http://www.json.org/index.html
 * @param str in out param, c-style utf8 string.
 * @param error out param, if an error occurs, which value will be set.
 * @return parsed value. or empty string if error occurs.
 */
static std::string ReadString(const char **str, int *error)
{
    auto lst = *str;
    bool escape = false;
    unsigned int count = 0;

    std::string ret;
    for (auto tmp = lst; *tmp != '\"'; ++tmp) {
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
                ret.append(lst, count);
                count = 0;
                lst = tmp + 1;
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
                    if (!TryParseHexShort(tmp + 1, unicode)) {
                        *error = STRING_SYNTAX_ERROR;
                        return std::string();
                    }

                    char utf8[3];
                    auto len = Unicode2ToUtf8(unicode, utf8);
                    for (int i = 0; i < len; ++i) {
                        ret.push_back(utf8[i]);
                    }
                    tmp += 4;
                    lst = tmp + 1;
                    continue;
                }
                default:
                    *error = STRING_SYNTAX_ERROR;
                    return std::string();
            }
        }
        if (!Assert(std::iscntrl(*tmp) == 0)) {
            *error = STRING_SYNTAX_ERROR;
            return std::string();
        }
        ++count;
    }

    if (count != 0) {
        ret.append(lst, count);
    }

    *str = lst + count + 1;
    return ret;
}

namespace {

union NumberUnion
{
    int64_t intValue;
    double floatValue;
};

}

constexpr int64_t MaxCriticalValue = std::numeric_limits<int64_t>::max() / 10 - 1;

/**
 * parse number type string. for number pattern detail see http://www.json.org/index.html
 * @param numStr in out param, c-style utf8 string.
 * @param error out param. if an error occurs, which value will be set.
 * @param number a union type param, value type can be determined by the return value of this function.
 * @return indicate the type of number. true for float while false for integer. if an error
 * occurs, always return false.
 */
static bool ReadNumber(const char **numStr, int *error, NumberUnion &number)
{
    auto str = *numStr;
    bool isNegative = false;
    if (*str == '-') {
        isNegative = true;
        ++str;
    }

    bool isFloat = false;
    double baseFloatNum = 0.0;
    int64_t baseNumber = 0;
    int32_t exponent = 0;
    int32_t integerCount = 0;   // integer part number count
    if (*str == '0') {
        ++str;
    } else if (*str >= '1' && *str <= '9') {
        baseNumber = *str - '0';
        ++str;
        while (*str >= '0' && *str <= '9') {
            if (baseNumber > MaxCriticalValue) {
                // will overflow
                *error = NUMBER_INT_OVERFLOW;
                return false;
            }
            baseNumber = baseNumber * 10 + (*str - '0');
            ++integerCount;
            ++str;
        }
    } else {
        *error = UNEXPECTED_TOKEN;
        return false;
    }

    if (*str == '.') {
        ++str;
        if (*str < '0' || *str > '9') {
            // error syntax: there must be a digit after `digit.`.
            *error = NUMBER_FORMAT_ERROR;
            return false;
        }

        isFloat = true;
        baseFloatNum = baseNumber;
        int precisionBase = 10;
        do {
            double v = *str - '0';
            v /= precisionBase;
            baseFloatNum += v;
            if (precisionBase < 1e17) {
                // precision of double is 16. if value is greater than 1e17, fractional part will be dropped.
                // ignore case that `baseFloatNum` itself is big.
                precisionBase *= 10;
            }

            ++str;
        } while (*str >= '0' && *str <= '9');
    }

    if (*str == 'e' || *str == 'E') {
        // exponent part
        ++str;
        bool isNegativeExp = false;
        if (*str == '-') {
            isNegativeExp = true;
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
            int actualExp = (isNegativeExp ? -exponent : exponent) + integerCount;
            if (actualExp + 1 > std::numeric_limits<double>::max_exponent10) {
                // overflow or underflow
                *error = NUMBER_FLOAT_OVERFLOW;
                return false;
            } else if (actualExp - 1 < std::numeric_limits<double>::min_exponent10) {
                *error = NUMBER_FLOAT_UNDERFLOW;
                return false;
            }

            ++str;
        } while (*str >= '0' && *str <= '9');

        if (isNegativeExp) {
            exponent = -exponent;
        }
    }

    *numStr = str;
    if (isNegative) {
        if (isFloat) {
            baseFloatNum = -baseFloatNum;
        } else {
            baseNumber = -baseNumber;
        }
    }

    if (exponent != 0) {
        number.floatValue = (isFloat ? baseFloatNum : (double)baseNumber) * std::pow(10, exponent);
        return true;
    }
    if (isFloat) {
        number.floatValue = baseFloatNum;
        return true;
    }
    number.intValue = baseNumber;
    return false;
}

static std::unique_ptr<JToken> ReadToken(const char **str, int *error);

static std::unique_ptr<JToken> ReadObject(const char **objStr, int *error)
{
    auto str = SkipWhiteSpace(*objStr, 1);
    auto *ptr = new JObject();
    std::unique_ptr<JToken> obj(ptr);
    // empty object
    if (*str == '}') {
        *objStr = str + 1;
        return obj;
    }

    while (true) {
        if (!AssertEqual(*str, '\"')) {
            *error = OBJECT_SYNTAX_ERROR;
            return nullptr;
        }
        ++str;
        auto key = ReadString(&str, error);
        if (*error != NO_ERROR) {
            return nullptr;
        }

        str = SkipWhiteSpace(str);
        if (!AssertEqual(*str, ':')) {
            *error = OBJECT_SYNTAX_ERROR;
            return nullptr;
        }
        ++str;
        auto value = ReadToken(&str, error);
        if (*error != NO_ERROR) {
            return nullptr;
        }

        // insert key and value
        auto ret = ptr->Put(std::move(key), std::move(value));
        if (!Assert(ret)) {
            *error = OBJECT_DUPLICATED_KEY;
            return nullptr;
        }

        str = SkipWhiteSpace(str);
        if (*str == ',') {
            str = SkipWhiteSpace(str, 1);
            continue;
        }
        if (!Assert(*str == '}')) {
            *error = OBJECT_PARSE_ERROR;
            return nullptr;
        }

        *objStr = str + 1;
        return obj;
    }
}

static std::unique_ptr<JToken> ReadArray(const char **aryStr, int *error)
{
    auto str = SkipWhiteSpace(*aryStr, 1);
    auto *ptr = new JArray();
    std::unique_ptr<JToken> ary(ptr);
    // empty array
    if (*str == ']') {
        *aryStr = str + 1;
        return ary;
    }

    while (true) {
        auto elem = ReadToken(&str, error);
        if (*error != NO_ERROR) {
            return nullptr;
        }
        ptr->Add(std::move(elem));

        str = SkipWhiteSpace(str);
        if (*str == ',') {
            ++str;
            continue;
        }

        if (!AssertEqual(*str, ']')) {
            *error = ARRAY_PARSE_ERROR;
            return nullptr;
        }
        *aryStr = str + 1;
        return ary;
    }
}

static std::unique_ptr<JToken> ReadValue(const char **valStr, int *error)
{
    auto str = *valStr;
    if (*str == '\"') {
        // Parse string value
        ++str;
        auto value = ReadString(&str, error);
        if (*error != NO_ERROR) {
            return nullptr;
        }
        *valStr = str;
        return std::unique_ptr<JToken>(new JStringValue(std::move(value)));
    }
    if (std::strncmp(str, "true", 4) == 0) {
        *valStr = str + 4;
        return std::unique_ptr<JToken>(new JBoolValue(true));
    }
    if (std::strncmp(str, "false", 5) == 0) {
        *valStr = str + 5;
        return std::unique_ptr<JToken>(new JBoolValue());
    }
    if (std::strncmp(str, "null", 4) == 0) {
        *valStr = str + 4;
        return std::unique_ptr<JToken>(new JNullValue());
    }

    // Parse number value
    NumberUnion number{};
    auto isFloat = ReadNumber(valStr, error, number);
    if (*error != NO_ERROR) {
        return nullptr;
    }
    if (isFloat) {
        return std::unique_ptr<JToken>(new JNumberValue(number.floatValue));
    }
    return std::unique_ptr<JToken>(new JNumberValue(number.intValue));
}

std::unique_ptr<JToken> ReadToken(const char **str, int *error)
{
    auto tmp = SkipWhiteSpace(*str);
    *str = tmp;

    if (*tmp == '{') {
        return ReadObject(str, error);
    }

    if (*tmp == '[') {
        return ReadArray(str, error);
    }

    return ReadValue(str, error);
}

std::unique_ptr<JToken> json::Parse(const char *json, int *error)
{
#ifdef ERROR_LOG
    const char *start = json;
#endif
    int code = NO_ERROR;
    auto ret = ReadToken(&json, &code);
    if (error) {
        *error = code;
    }

    if (code != NO_ERROR) {
#ifdef ERROR_LOG
        std::cerr << "unexcepted end character near byte position " << static_cast<int64_t>(json - start)
                  << ", error code: " << code << ", info: " << GetErrorInfo(code) << std::endl;
#endif
        return nullptr;     // read token error
    }
    if (AssertEndStr(json)) {
        return ret;     // no error: read token return no error && string end.
    }
    // read token no error, but string not end.
    if (error) {
        *error = UNEXPECTED_END_CHAR;
    }
#ifdef ERROR_LOG
    std::cerr << "unexcepted end character near byte position " << static_cast<int64_t>(json - start) << std::endl;
#endif
    return nullptr;
}

constexpr char hex_digit[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static void FormatString(const std::string &value, std::string &builder)
{
    builder.push_back('\"');
    for (auto c : value) {
        switch (c) {
            case '\"':
            case '\\':
            case '/':
                builder.push_back('\\');
                builder.push_back(c);
                continue;
            case '\b':
                builder.append("\\b");
                continue;
            case '\f':
                builder.append("\\f");
                continue;
            case '\n':
                builder.append("\\n");
                continue;
            case '\r':
                builder.append("\\r");
                continue;
            case '\t':
                builder.append("\\t");
                continue;
            default: {
                if (std::iscntrl(c)) {
                    builder.append("\\u00");
                    builder.push_back(hex_digit[(unsigned char)c >> 4]);
                    builder.push_back(hex_digit[(unsigned char)c & 0x0f]);
                } else {
                    builder.push_back(c);
                }
            }
        }
    }
    builder.push_back('\"');
}

static void FormatNumber(const JNumberValue &num, std::string &builder)
{
    if (num.IsFloatValue()) {
        builder.append(std::to_string((double)num));
    } else {
        builder.append(std::to_string((int64_t)num));
    }
}

static void FormatToken(const JToken &token, std::string &builder, char indent, unsigned base, unsigned level);

static void FormatObject(const JObject &obj, std::string &builder, char indent, unsigned base, unsigned level)
{
    size_t count = base * (level + 1);
    builder.append("{\n");
    if (obj.Size() > 0) {
        for (const auto &property : obj) {
            builder.append(count, indent);
            FormatString(property.first, builder);
            builder.append(": ");
            FormatToken(*property.second, builder, indent, base, level + 1);
            builder.append(",\n");
        }
        builder.pop_back();
        builder.pop_back();
        builder.push_back('\n');
    }
    builder.append(base * level, indent);
    builder.push_back('}');
}

static void FormatArray(const JArray &ary, std::string &builder, char indent, unsigned base, unsigned level)
{
    size_t count = base * (level + 1);
    builder.append("[\n");
    if (ary.Size() > 0) {
        for (const auto &element : ary) {
            builder.append(count, indent);
            FormatToken(*element, builder, indent, base, level + 1);
            builder.append(",\n");
        }
        builder.pop_back();
        builder.pop_back();
        builder.push_back('\n');
    }
    builder.append(base * level, indent);
    builder.push_back(']');
}

void FormatToken(const JToken &token, std::string &builder, char indent, unsigned base, unsigned level)
{
    switch (token.GetType()) {
        case JsonType::Object:
            FormatObject(static_cast<const JObject &>(token), builder, indent, base, level); // NOLINT
            break;
        case JsonType::Array:
            FormatArray(static_cast<const JArray &>(token), builder, indent, base, level); // NOLINT
            break;
        case JsonType::String:
            FormatString(static_cast<const JStringValue &>(token).Value(), builder); // NOLINT
            break;
        case JsonType::Number:
            FormatNumber(static_cast<const JNumberValue &>(token), builder); // NOLINT
            break;
        case JsonType::Bool: {
            if ((bool)static_cast<const JBoolValue &>(token)) { // NOLINT
                builder.append("true");
            } else {
                builder.append("false");
            }
            break;
        }
        case JsonType::Null: {
            builder.append("null");
            break;
        }
    }
}

static void FormatToken(const JToken &token, std::string &builder);

static void FormatObject(const JObject &obj, std::string &builder)
{
    builder.push_back('{');
    if (obj.Size() > 0) {
        for (const auto &property : obj) {
            FormatString(property.first, builder);
            builder.push_back(':');
            FormatToken(*property.second, builder);
            builder.push_back(',');
        }
        builder.pop_back();
    }
    builder.push_back('}');
}

static void FormatArray(const JArray &ary, std::string &builder)
{
    builder.push_back('[');
    if (ary.Size() > 0) {
        for (const auto &element : ary) {
            FormatToken(*element, builder);
            builder.push_back(',');
        }
        builder.pop_back();
    }
    builder.push_back(']');
}

void FormatToken(const JToken &token, std::string &builder)
{
    switch (token.GetType()) {
        case JsonType::Object:
            FormatObject(static_cast<const JObject &>(token), builder); // NOLINT
            break;
        case JsonType::Array:
            FormatArray(static_cast<const JArray &>(token), builder); // NOLINT
            break;
        case JsonType::String:
            FormatString(static_cast<const JStringValue &>(token).Value(), builder); // NOLINT
            break;
        case JsonType::Number:
            FormatNumber(static_cast<const JNumberValue &>(token), builder); // NOLINT
            break;
        case JsonType::Bool: {
            if ((bool)static_cast<const JBoolValue &>(token)) { // NOLINT
                builder.append("true");
            } else {
                builder.append("false");
            }
            break;
        }
        case JsonType::Null: {
            builder.append("null");
            break;
        }
    }
}

std::string json::ToString(const JToken &token, JsonFormatOption option, unsigned int indention)
{
    std::string builder;
    switch (option) {
        case JsonFormatOption::IndentSpace:
            FormatToken(token, builder, ' ', indention, 0);
            break;
        case JsonFormatOption::IndentTab:
            FormatToken(token, builder, '\t', indention, 0);
            break;
        default:
            FormatToken(token, builder);
    }
    return builder;
}

const char *json::GetErrorInfo(int error) noexcept
{
    switch (error) {
        case NO_ERROR:
            return "No error.";
        case STRING_PARSE_ERROR:
            return "String parse error, check the string format.";
        case STRING_SYNTAX_ERROR:
            return "String value syntax error, check if exists a wrong escape character or a control character.";
        case OBJECT_PARSE_ERROR:
            return "Object parse error, check the object format.";
        case OBJECT_SYNTAX_ERROR:
            return "Object syntax error.";
        case OBJECT_DUPLICATED_KEY:
            return "Duplicated key in object.";
        case ARRAY_PARSE_ERROR:
            return "Array parse error, check the array format.";
        case NUMBER_FORMAT_ERROR:
            return "Number format error.";
        case NUMBER_INT_OVERFLOW:
            return "Integer number overflow.";
        case NUMBER_FLOAT_OVERFLOW:
            return "Float point number overflow.";
        case NUMBER_FLOAT_UNDERFLOW:
            return "Float point number underflow.";
        case UNEXPECTED_TOKEN:
            return "Unexpected token. Only support json standard primitive types.";
        case UNEXPECTED_END_CHAR:
            return "Unexpected character at the end, NULL character needed.";
        default:
            return "Unknown error code.";
    }
}
