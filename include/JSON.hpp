//
// Created by qife on 16/1/10.
//

#ifndef CPPPARSER_JTOKEN_H
#define CPPPARSER_JTOKEN_H

#include <string>
#include <vector>
#include <list>
#include <limits>
#include <memory>
#include <cassert>

#include "JsonUtil.h"

namespace json {

namespace util {

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

}


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
    virtual JsonType GetType() const = 0;

    virtual ~JToken() = default;
};


/**
 * json object type. An unordered set of properties mapping a string to an instance,
 * from the JSON "object" production.
 */
class JObject : public JToken
{
public:
    JObject() = default;

    JsonType GetType() const override { return TYPE; }

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

    bool Put(const std::string &property, std::unique_ptr<JToken> &&value)
    {
        return children.emplace(property, std::move(value)).second;
    }

    bool Put(std::string &&property, std::unique_ptr<JToken> &&value)
    {
        return children.emplace(property, std::move(value)).second;
    }

private:
    std::map<std::string, std::unique_ptr<JToken>> children;

public:
    static constexpr JsonType TYPE = JsonType::Object;
};


/**
 * json array type. An ordered list of instances, from the JSON "array" production.
 */
class JArray : public JToken
{
public:
    JArray() = default;

    JsonType GetType() const override { return TYPE; }

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
        return GetValue(index);
    }

    const JToken *operator[](size_t index) const
    {
        return GetValue(index);
    }

    JToken *GetValue(size_t index)
    {
        return index >= Size() ? nullptr : children[index].get();
    }

    const JToken *GetValue(size_t index) const
    {
        return index >= Size() ? nullptr : children[index].get();
    }

    void Add(std::unique_ptr<JToken> &&element)
    {
        children.emplace_back(std::move(element));
    }

private:
    std::vector<std::unique_ptr<JToken>> children;

public:
    static constexpr JsonType TYPE = JsonType::Array;
};


/**
 * json string value. A string of Unicode code points, from the JSON "string" production.
 */
class JStringValue : JToken
{
public:
    JStringValue() = default;

    explicit JStringValue(const std::string &v) : value(v) { }

    explicit JStringValue(std::string &&v) : value(std::move(v)) { }

    JsonType GetType() const override { return TYPE; }

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

    JsonType GetType() const override { return TYPE; }

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
    explicit JBoolValue(bool v = false) : value(v) { }

    JsonType GetType() const override { return TYPE; }

    explicit operator bool() const
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
    JsonType GetType() const override { return TYPE; }

public:
    static constexpr JsonType TYPE = JsonType::Null;
};

}

#endif //CPPPARSER_JTOKEN_H
