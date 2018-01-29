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

class JToken;

enum class JsonFormatOption
{
    NoFormat,
    IndentSpace,
    IndentTab
};

/**
 * parse c-style json string to {@code JToken}.
 * @param json c-style json format string.
 * @param error out param, a code to identify parse error, 0 means no error. {@code nullptr} can be passed.
 * @return if no error occurs, return a {@code JToken} pointer, use {@code JToken::GetType} to determine
 * the actual json type. return a default empty {@code std::unique_ptr} object if any errors occur.
 */
std::unique_ptr<JToken> Parse(const char *json, int *error);

/**
 * parse json string to {@code JToken}.
 * @param json json format string.
 * @param error out param, a code to identify parse error, 0 means no error. {@code nullptr} can be passed.
 * @return if no error occurs, return a {@code JToken} pointer, use {@code JToken::GetType} to determine
 * the actual json type. return a default empty {@code std::unique_ptr} object if any errors occur.
 */
std::unique_ptr<JToken> Parse(const std::string &json, int *error)
{
    return Parse(json.c_str(), error);
}

/**
 * format json instance.
 * @param token a json instance to be formatted.
 * @param option format indention option.
 * @param indention indention count, used only when option is {@code JsonFormatOption::IndentSpace} or
 * {@code JsonFormatOption::IndentTab}.
 * @return formatted string.
 */
std::string ToString(const JToken &token, JsonFormatOption option = JsonFormatOption::NoFormat, unsigned indention = 1);

/**
 * get a human friend description of error code.
 * @param error error code.
 * @return an error description.
 */
const char *GetErrorInfo(int error) noexcept;

/**
 * copy construct a json token object.
 * @param token source object.
 * @return new object copied from source.
 */
std::unique_ptr<JToken> clone(const JToken &token);


enum class JsonType : int
{
    Object = 0,
    Array,
    String,
    Number,
    Bool,
    Null
};


/**
 * a virtual base class, supporting to box the various json data type
 * such as {@code JArray}, {@code JObject}, etc.
 */
class JToken
{
public:
    virtual JsonType GetType() const noexcept = 0;

    virtual ~JToken() = default;
};


/**
 * json object type. An unordered set of properties mapping a string to an instance,
 * from the JSON "object" production.
 */
class JObject : public JToken
{
    using Container = std::map<std::string, std::unique_ptr<JToken>>;

public:
    JObject() = default;

    JsonType GetType() const noexcept override { return TYPE; }

    size_t Size() const
    {
        return children.size();
    }

    // use to access
    JToken *operator[](const std::string &property)
    {
        return GetValue(property);
    }

    const JToken *operator[](const std::string &property) const
    {
        return GetValue(property);
    }

    JToken *GetValue(const std::string &property)
    {
        auto r = children.find(property);
        return r == children.end() ? nullptr : r->second.get();
    }

    const JToken *GetValue(const std::string &property) const
    {
        auto r = children.find(property);
        return r == children.end() ? nullptr : r->second.get();
    }

    // iterator
    auto begin() noexcept -> Container::iterator
    {
        return children.begin();
    }

    auto begin() const noexcept -> Container::const_iterator
    {
        return children.begin();
    }

    auto end() noexcept -> Container::iterator
    {
        return children.end();
    }

    auto end() const noexcept -> Container::const_iterator
    {
        return children.end();
    }

    bool Put(const std::string &property, std::unique_ptr<JToken> &&value)
    {
        return children.emplace(property, std::move(value)).second;
    }

    bool Put(std::string &&property, std::unique_ptr<JToken> &&value)
    {
        return children.emplace(property, std::move(value)).second;
    }

private:
    Container children;

public:
    static constexpr JsonType TYPE = JsonType::Object;
};


/**
 * json array type. An ordered list of instances, from the JSON "array" production.
 */
class JArray : public JToken
{
    using Container = std::vector<std::unique_ptr<JToken>>;

public:
    JArray() = default;

    explicit JArray(const std::vector<const JToken *> &tokens) : children()
    {
        children.reserve(tokens.size());
        for (auto token : tokens) {
            if (token) {
                children.emplace_back(clone(*token));
            }
        }
    }

    JsonType GetType() const noexcept override { return TYPE; }

    void Reserve(size_t capacity)
    {
        children.reserve(capacity);
    }

    size_t Size() const
    {
        return children.size();
    }

    // use to access
    JToken *operator[](size_t index)
    {
        return children[index].get();
    }

    const JToken *operator[](size_t index) const
    {
        return children[index].get();
    }

    JToken *GetValue(size_t index)
    {
        return index >= Size() ? nullptr : children[index].get();
    }

    const JToken *GetValue(size_t index) const
    {
        return index >= Size() ? nullptr : children[index].get();
    }

    // iterator
    auto begin() noexcept -> Container::iterator
    {
        return children.begin();
    }

    auto begin() const noexcept -> Container::const_iterator
    {
        return children.begin();
    }

    auto end() noexcept -> Container::iterator
    {
        return children.end();
    }

    auto end() const noexcept -> Container::const_iterator
    {
        return children.end();
    }

    void Add(std::unique_ptr<JToken> &&element)
    {
        children.emplace_back(std::move(element));
    }

private:
    Container children;

public:
    static constexpr JsonType TYPE = JsonType::Array;
};


/**
 * json string value. A string of Unicode code points, from the JSON "string" production.
 */
class JStringValue : public JToken
{
public:
    JStringValue() = default;

    explicit JStringValue(const std::string &v) : value(v) { }

    explicit JStringValue(std::string &&v) noexcept : value(std::move(v)) { }

    JsonType GetType() const noexcept override { return TYPE; }

    // use to access value
    std::string &Value()
    {
        return value;
    }

    const std::string &Value() const
    {
        return value;
    }

private:
    std::string value;

public:
    static constexpr JsonType TYPE = JsonType::String;
};


/**
 * json number value. An arbitrary-precision, base-10 decimal number value,
 * from the JSON "number" production
 */
class JNumberValue : public JToken
{
public:
    JNumberValue() : isFloat(false), value{.intValue = 0} { }

    explicit JNumberValue(int64_t v) : isFloat(false), value{.intValue = v} { }

    explicit JNumberValue(double v) : isFloat(true), value{.floatValue = v} { }

    JsonType GetType() const noexcept override { return TYPE; }

    // access
    explicit operator int64_t() const
    {
        return isFloat ? static_cast<int64_t>(value.floatValue) : value.intValue;
    }

    explicit operator double() const
    {
        return isFloat ? value.floatValue : value.intValue;
    }

    void SetValue(int64_t iv)
    {
        isFloat = false;
        value.intValue = iv;
    }

    void SetValue(double dv)
    {
        isFloat = true;
        value.floatValue = dv;
    }

    bool IsFloatValue() const
    {
        return isFloat;
    }

private:
    bool isFloat;
    union
    {
        int64_t intValue;
        double floatValue;
    } value;

public:
    static constexpr JsonType TYPE = JsonType::Number;
};


/**
 * json bool value. A "true" or "false" value, from the JSON "true" or "false" productions.
 */
class JBoolValue : public JToken
{
public:
    explicit JBoolValue(bool v = false) noexcept : value(v) { }

    JsonType GetType() const noexcept override { return TYPE; }

    explicit operator bool() const noexcept
    {
        return value;
    }

private:
    bool value;

public:
    static constexpr JsonType TYPE = JsonType::Bool;
};


/**
 * json null value. A JSON "null" production.
 */
class JNullValue : public JToken
{
public:
    JsonType GetType() const noexcept override { return TYPE; }

public:
    static constexpr JsonType TYPE = JsonType::Null;
};

}

#endif //JSONCPP_JSON_H
