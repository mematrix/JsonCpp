//
// Created by Charles on 2018/1/18.
//

#ifndef JSONCPP_JSON_H
#define JSONCPP_JSON_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace json {

class json_token;

enum class json_format_option
{
    no_format,
    indent_space,
    indent_tab
};

/**
 * parse c-style json string to {@code json_token}.
 * @param json c-style json format string.
 * @param error out param, a code to identify parse error, 0 means no error. {@code nullptr} can be passed.
 * @return if no error occurs, return a {@code json_token} pointer, use {@code json_token::get_type} to determine
 * the actual json type. return a default empty {@code std::unique_ptr} object if any errors occur.
 */
std::unique_ptr<json_token> parse(const char *json, int *error);

/**
 * parse json string to {@code json_token}.
 * @param json json format string.
 * @param error out param, a code to identify parse error, 0 means no error. {@code nullptr} can be passed.
 * @return if no error occurs, return a {@code json_token} pointer, use {@code json_token::get_type} to determine
 * the actual json type. return a default empty {@code std::unique_ptr} object if any errors occur.
 */
inline std::unique_ptr<json_token> parse(const std::string &json, int *error)
{
    return parse(json.c_str(), error);
}

/**
 * format json instance.
 * @param token a json instance to be formatted.
 * @param option format indention option.
 * @param indention indention count, used only when option is {@code json_format_option::indent_space} or
 * {@code json_format_option::indent_tab}.
 * @return formatted string.
 */
std::string to_string(const json_token &token, json_format_option option = json_format_option::no_format, unsigned indention = 1);

/**
 * get a human friend description of error code.
 * @param error error code.
 * @return an error description.
 */
const char *get_error_info(int error) noexcept;

/**
 * copy construct a json token object.
 * @param token source object.
 * @return new object copied from source.
 */
// std::unique_ptr<json_token> clone(const json_token &token);


enum class json_type : int
{
    object = 0,
    array,
    string,
    number,
    boolean,
    null
};


/**
 * a virtual base class, supporting to box the various json data type
 * such as {@code json_array}, {@code json_object}, etc.
 */
class json_token
{
public:
    virtual json_type get_type() const noexcept = 0;

    virtual ~json_token() = default;
};


/**
 * json object type. An unordered set of properties mapping a string to an instance,
 * from the JSON "object" production.
 */
class json_object : public json_token
{
    using container = std::map<std::string, std::unique_ptr<json_token>>;

public:
    json_object() = default;

    json_type get_type() const noexcept override { return TYPE; }

    size_t size() const
    {
        return children.size();
    }

    // use to access
    json_token *operator[](const std::string &property)
    {
        return get_value(property);
    }

    const json_token *operator[](const std::string &property) const
    {
        return get_value(property);
    }

    json_token *get_value(const std::string &property)
    {
        auto r = children.find(property);
        return r == children.end() ? nullptr : r->second.get();
    }

    const json_token *get_value(const std::string &property) const
    {
        auto r = children.find(property);
        return r == children.end() ? nullptr : r->second.get();
    }

    // iterator
    auto begin() noexcept -> container::iterator
    {
        return children.begin();
    }

    auto begin() const noexcept -> container::const_iterator
    {
        return children.begin();
    }

    auto end() noexcept -> container::iterator
    {
        return children.end();
    }

    auto end() const noexcept -> container::const_iterator
    {
        return children.end();
    }

    bool put(const std::string &property, std::unique_ptr<json_token> &&value)
    {
        return children.emplace(property, std::move(value)).second;
    }

    bool put(std::string &&property, std::unique_ptr<json_token> &&value)
    {
        return children.emplace(std::move(property), std::move(value)).second;
    }

private:
    container children;

public:
    static constexpr json_type TYPE = json_type::object;
};


/**
 * json array type. An ordered list of instances, from the JSON "array" production.
 */
class json_array : public json_token
{
    using container = std::vector<std::unique_ptr<json_token>>;

public:
    json_array() = default;

    json_type get_type() const noexcept override { return TYPE; }

    void reserve(size_t capacity)
    {
        children.reserve(capacity);
    }

    size_t size() const
    {
        return children.size();
    }

    // use to access
    json_token *operator[](size_t index)
    {
        return children[index].get();
    }

    const json_token *operator[](size_t index) const
    {
        return children[index].get();
    }

    json_token *get_value(size_t index)
    {
        return index >= size() ? nullptr : children[index].get();
    }

    const json_token *get_value(size_t index) const
    {
        return index >= size() ? nullptr : children[index].get();
    }

    // iterator
    auto begin() noexcept -> container::iterator
    {
        return children.begin();
    }

    auto begin() const noexcept -> container::const_iterator
    {
        return children.begin();
    }

    auto end() noexcept -> container::iterator
    {
        return children.end();
    }

    auto end() const noexcept -> container::const_iterator
    {
        return children.end();
    }

    void add(std::unique_ptr<json_token> &&element)
    {
        children.emplace_back(std::move(element));
    }

private:
    container children;

public:
    static constexpr json_type TYPE = json_type::array;
};


/**
 * json string value. A string of Unicode code points, from the JSON "string" production.
 */
class json_string_value : public json_token
{
public:
    json_string_value() = default;

    explicit json_string_value(const std::string &v) : str_value(v) { }

    explicit json_string_value(std::string &&v) noexcept : str_value(std::move(v)) { }

    json_type get_type() const noexcept override { return TYPE; }

    // use to access value
    std::string &value()
    {
        return str_value;
    }

    const std::string &value() const
    {
        return str_value;
    }

private:
    std::string str_value;

public:
    static constexpr json_type TYPE = json_type::string;
};


/**
 * json number value. An arbitrary-precision, base-10 decimal number value,
 * from the JSON "number" production
 */
class json_number_value : public json_token
{
public:
    json_number_value() : is_float(false) { value.int_value = 0; } // NOLINT

    explicit json_number_value(int64_t v) : is_float(false) { value.int_value = v; } // NOLINT

    explicit json_number_value(double v) : is_float(true) { value.float_value = v; } // NOLINT

    json_type get_type() const noexcept override { return TYPE; }

    // access
    explicit operator int64_t() const
    {
        return is_float ? static_cast<int64_t>(value.float_value) : value.int_value;
    }

    explicit operator double() const
    {
        return is_float ? value.float_value : value.int_value;
    }

    void set_value(int64_t iv)
    {
        is_float = false;
        value.int_value = iv;
    }

    void set_value(double dv)
    {
        is_float = true;
        value.float_value = dv;
    }

    bool is_float_value() const
    {
        return is_float;
    }

private:
    bool is_float;
    union
    {
        int64_t int_value;
        double float_value;
    } value;

public:
    static constexpr json_type TYPE = json_type::number;
};


/**
 * json bool value. A "true" or "false" value, from the JSON "true" or "false" productions.
 */
class json_bool_value : public json_token
{
public:
    explicit json_bool_value(bool v = false) noexcept : value(v) { }

    json_type get_type() const noexcept override { return TYPE; }

    explicit operator bool() const noexcept
    {
        return value;
    }

private:
    bool value;

public:
    static constexpr json_type TYPE = json_type::boolean;
};


/**
 * json null value. A JSON "null" production.
 */
class json_null_value : public json_token
{
public:
    json_type get_type() const noexcept override { return TYPE; }

public:
    static constexpr json_type TYPE = json_type::null;
};

}

#endif //JSONCPP_JSON_H
