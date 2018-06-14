//
// Created by Charles on 2018/1/18.
//

#include <iostream>
#include <cstring>
#include "JSON.hpp"
#include "JSONUtils.h"

using namespace json;


static std::unique_ptr<json_token> read_token(const char **str, int *error);

static std::unique_ptr<json_token> read_object(const char **object_str, int *error)
{
    auto str = skip_whitespace(*object_str, 1);
    auto *ptr = new json_object();
    std::unique_ptr<json_token> object_ptr(ptr);
    // empty object
    if (*str == '}') {
        *object_str = str + 1;
        return object_ptr;
    }

    while (true) {
        if (!assert_equal(*str, '\"')) {
            *error = OBJECT_SYNTAX_ERROR;
            return nullptr;
        }
        ++str;
        auto key = read_json_string(&str, error);
        if (*error != NO_ERROR) {
            return nullptr;
        }

        str = skip_whitespace(str);
        if (!assert_equal(*str, ':')) {
            *error = OBJECT_SYNTAX_ERROR;
            return nullptr;
        }
        ++str;
        auto value = read_token(&str, error);
        if (*error != NO_ERROR) {
            return nullptr;
        }

        // insert key and value
        auto ret = ptr->put(std::move(key), std::move(value));
        if (!json_assert(ret)) {
            *error = OBJECT_DUPLICATED_KEY;
            return nullptr;
        }

        str = skip_whitespace(str);
        if (*str == ',') {
            str = skip_whitespace(str, 1);
            continue;
        }
        if (!json_assert(*str == '}')) {
            *error = OBJECT_PARSE_ERROR;
            return nullptr;
        }

        *object_str = str + 1;
        return object_ptr;
    }
}

static std::unique_ptr<json_token> read_array(const char **array_str, int *error)
{
    auto str = skip_whitespace(*array_str, 1);
    auto *ptr = new json_array();
    std::unique_ptr<json_token> array_ptr(ptr);
    // empty array
    if (*str == ']') {
        *array_str = str + 1;
        return array_ptr;
    }

    while (true) {
        auto elem = read_token(&str, error);
        if (*error != NO_ERROR) {
            return nullptr;
        }
        ptr->add(std::move(elem));

        str = skip_whitespace(str);
        if (*str == ',') {
            ++str;
            continue;
        }

        if (!assert_equal(*str, ']')) {
            *error = ARRAY_PARSE_ERROR;
            return nullptr;
        }
        *array_str = str + 1;
        return array_ptr;
    }
}

static std::unique_ptr<json_token> read_value(const char **value_str, int *error)
{
    auto str = *value_str;
    if (*str == '\"') {
        // Parse string value
        ++str;
        auto value = read_json_string(&str, error);
        if (*error != NO_ERROR) {
            return nullptr;
        }
        *value_str = str;
        return std::unique_ptr<json_token>(new json_string_value(std::move(value)));
    }
    if (std::strncmp(str, "true", 4) == 0) {
        *value_str = str + 4;
        return std::unique_ptr<json_token>(new json_bool_value(true));
    }
    if (std::strncmp(str, "false", 5) == 0) {
        *value_str = str + 5;
        return std::unique_ptr<json_token>(new json_bool_value());
    }
    if (std::strncmp(str, "null", 4) == 0) {
        *value_str = str + 4;
        return std::unique_ptr<json_token>(new json_null_value());
    }

    // Parse number value
    number_union number{};
    auto is_float = read_json_number(value_str, error, number);
    if (*error != NO_ERROR) {
        return nullptr;
    }
    if (is_float) {
        return std::unique_ptr<json_token>(new json_number_value(number.float_value));
    }
    return std::unique_ptr<json_token>(new json_number_value(number.int_value));
}

std::unique_ptr<json_token> read_token(const char **str, int *error)
{
    auto tmp = skip_whitespace(*str);
    *str = tmp;

    if (*tmp == '{') {
        return read_object(str, error);
    }

    if (*tmp == '[') {
        return read_array(str, error);
    }

    return read_value(str, error);
}

std::unique_ptr<json_token> json::parse(const char *json, int *error)
{
#ifdef ERROR_LOG
    const char *start = json;
#endif
    int code = NO_ERROR;
    auto ret = read_token(&json, &code);
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
    if (assert_end_str(json)) {
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

static void format_string(const std::string &value, std::string &builder)
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
                    builder.push_back(hex_digit[(unsigned char)c >> 4u]);
                    builder.push_back(hex_digit[(unsigned char)c & 0x0fu]);
                } else {
                    builder.push_back(c);
                }
            }
        }
    }
    builder.push_back('\"');
}

static void format_number(const json_number_value &num, std::string &builder)
{
    if (num.is_float_value()) {
        builder.append(std::to_string((double)num));
    } else {
        builder.append(std::to_string((int64_t)num));
    }
}

static void format_token(const json_token &token, std::string &builder, char indent, unsigned base, unsigned level);

static void format_object(const json_object &obj, std::string &builder, char indent, unsigned base, unsigned level)
{
    size_t count = base * (level + 1);
    builder.append("{\n");
    if (obj.size() > 0) {
        for (const auto &property : obj) {
            builder.append(count, indent);
            format_string(property.first, builder);
            builder.append(": ");
            format_token(*property.second, builder, indent, base, level + 1);
            builder.append(",\n");
        }
        builder.pop_back();
        builder.pop_back();
        builder.push_back('\n');
    }
    builder.append(base * level, indent);
    builder.push_back('}');
}

static void format_array(const json_array &ary, std::string &builder, char indent, unsigned base, unsigned level)
{
    size_t count = base * (level + 1);
    builder.append("[\n");
    if (ary.size() > 0) {
        for (const auto &element : ary) {
            builder.append(count, indent);
            format_token(*element, builder, indent, base, level + 1);
            builder.append(",\n");
        }
        builder.pop_back();
        builder.pop_back();
        builder.push_back('\n');
    }
    builder.append(base * level, indent);
    builder.push_back(']');
}

void format_token(const json_token &token, std::string &builder, char indent, unsigned base, unsigned level)
{
    switch (token.get_type()) {
        case json_type::object:
            format_object(static_cast<const json_object &>(token), builder, indent, base, level); // NOLINT
            break;
        case json_type::array:
            format_array(static_cast<const json_array &>(token), builder, indent, base, level); // NOLINT
            break;
        case json_type::string:
            format_string(static_cast<const json_string_value &>(token).value(), builder); // NOLINT
            break;
        case json_type::number:
            format_number(static_cast<const json_number_value &>(token), builder); // NOLINT
            break;
        case json_type::boolean: {
            if ((bool)static_cast<const json_bool_value &>(token)) { // NOLINT
                builder.append("true");
            } else {
                builder.append("false");
            }
            break;
        }
        case json_type::null: {
            builder.append("null");
            break;
        }
    }
}

static void format_token(const json_token &token, std::string &builder);

static void format_object(const json_object &obj, std::string &builder)
{
    builder.push_back('{');
    if (obj.size() > 0) {
        for (const auto &property : obj) {
            format_string(property.first, builder);
            builder.push_back(':');
            format_token(*property.second, builder);
            builder.push_back(',');
        }
        builder.pop_back();
    }
    builder.push_back('}');
}

static void format_array(const json_array &ary, std::string &builder)
{
    builder.push_back('[');
    if (ary.size() > 0) {
        for (const auto &element : ary) {
            format_token(*element, builder);
            builder.push_back(',');
        }
        builder.pop_back();
    }
    builder.push_back(']');
}

void format_token(const json_token &token, std::string &builder)
{
    switch (token.get_type()) {
        case json_type::object:
            format_object(static_cast<const json_object &>(token), builder); // NOLINT
            break;
        case json_type::array:
            format_array(static_cast<const json_array &>(token), builder); // NOLINT
            break;
        case json_type::string:
            format_string(static_cast<const json_string_value &>(token).value(), builder); // NOLINT
            break;
        case json_type::number:
            format_number(static_cast<const json_number_value &>(token), builder); // NOLINT
            break;
        case json_type::boolean: {
            if ((bool)static_cast<const json_bool_value &>(token)) { // NOLINT
                builder.append("true");
            } else {
                builder.append("false");
            }
            break;
        }
        case json_type::null: {
            builder.append("null");
            break;
        }
    }
}

std::string json::to_string(const json_token &token, json_format_option option, unsigned int indention)
{
    std::string builder;
    switch (option) {
        case json_format_option::indent_space:
            format_token(token, builder, ' ', indention, 0);
            break;
        case json_format_option::indent_tab:
            format_token(token, builder, '\t', indention, 0);
            break;
        default:
            format_token(token, builder);
    }
    return builder;
}

const char *json::get_error_info(int error) noexcept
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
