//
// Created by Charles on 2018/1/21.
//

#ifndef JSONCPP_JSONCONVERT_H
#define JSONCPP_JSONCONVERT_H

#include <list>
#include <cstring>
#include "JSON.hpp"

namespace json {

template<typename T>
struct deserialize_impl
{
    static void deserialize(T &t, const JToken &token)
    {
        std::cerr << "deserialize impl type: " << typeid(T).name() << std::endl;
        //static_assert(false);
    }
};

template<typename T, bool = std::is_arithmetic<typename std::remove_cv<T>::type>::value>
struct deserialize_dispatcher
{
    typedef deserialize_impl<T> handler;
};

template<typename T>
void deserialize(T &t, const JToken &token)
{
    using handler = typename deserialize_dispatcher<T>::handler;
    handler::deserialize(t, token);
}

template<typename T>
struct deserialize_arithmetic_impl
{
    static void deserialize(T &t, const JToken &token)
    {
        if (token.GetType() != JsonType::Number) {
            return;
        }

        const JNumberValue &num = static_cast<const JNumberValue &>(token); // NOLINT
        if (num.IsFloatValue()) {
            t = static_cast<T>((double)num);
        } else {
            t = static_cast<T>((int64_t)num);
        }
    }
};

template<>
struct deserialize_arithmetic_impl<bool>
{
    static void deserialize(bool &t, const JToken &token)
    {
        if (token.GetType() != JsonType::Bool) {
            return;
        }

        t = (bool)static_cast<const JBoolValue &>(token); // NOLINT
    }
};

template<typename T>
struct deserialize_dispatcher<T, true>
{
    typedef deserialize_arithmetic_impl<T> handler;
};

template<typename T>
struct deserialize_impl<std::map<std::string, T>>
{
    static void deserialize(std::map<std::string, T> &t, const JToken &token)
    {
        using handler = typename deserialize_dispatcher<T>::handler;

        if (token.GetType() != JsonType::Object) {
            return;
        }

        const JObject &obj = static_cast<const JObject &>(token);
        for (const auto &property : obj) {
            T value = T();
            handler::deserialize(value, *property.second);
            t.emplace(property.first, std::move(value));
        }
    }
};

template<typename T>
struct deserialize_impl<std::vector<T>>
{
    static void deserialize(std::vector<T> &t, const JToken &token)
    {
        using handler = typename deserialize_dispatcher<T>::handler;

        if (token.GetType() != JsonType::Array) {
            return;
        }

        const JArray &ary = static_cast<const JArray &>(token);
        for (const auto &element : ary) {
            T value = T();
            handler::deserialize(value, *element);
            t.emplace_back(std::move(value));
        }
    }
};

template<typename T>
struct deserialize_impl<std::list<T>>
{
    static void deserialize(std::list<T> &t, const JToken &token)
    {
        using handler = typename deserialize_dispatcher<T>::handler;

        if (token.GetType() != JsonType::Array) {
            return;
        }

        const JArray &ary = static_cast<const JArray &>(token);
        for (const auto &element : ary) {
            T value = T();
            handler::deserialize(value, *element);
            t.emplace_back(std::move(value));
        }
    }
};

template<typename T, size_t N>
struct deserialize_impl<T[N]>
{
    static void deserialize(T *t, const JToken &token)
    {
        using handler = typename deserialize_dispatcher<T>::handler;

        if (token.GetType() != JsonType::Array) {
            return;
        }

        const JArray &ary = static_cast<const JArray &>(token);
        size_t count = ary.Size();
        count = count > N ? N : count;
        for (size_t i = 0; i < count; ++i) {
            handler::deserialize(t[i], *ary[i]);
        }
    }
};

template<>
struct deserialize_impl<std::string>
{
    static void deserialize(std::string &t, const JToken &token)
    {
        if (token.GetType() != JsonType::String) {
            return;
        }

        t = static_cast<const JStringValue &>(token).Value(); // NOLINT
    }
};

template<size_t N>
struct deserialize_impl<char[N]>
{
    static void deserialize(char *t, const JToken &token)
    {
        static_assert(N > 0);

        if (token.GetType() == JsonType::Null) {
            t[0] = '\0';
            return;
        }
        if (token.GetType() != JsonType::String) {
            return;
        }

        auto &str = static_cast<const JStringValue &>(token).Value();
        size_t count = str.size();
        count = count > (N - 1) ? (N - 1) : count;
        std::memcpy(t, str.c_str(), count);
        t[count] = '\0';
    }
};

template<typename T>
struct deserialize_impl<std::unique_ptr<T>>
{
    static void deserialize(std::unique_ptr<T> &t, const JToken &token)
    {
        using handler = typename deserialize_dispatcher<T>::handler;

        if (token.GetType() == JsonType::Null) {
            t = nullptr;
            return;
        }

        t.reset(new T());
        handler::deserialize(*t, token);
    }
};

template<typename T>
struct deserialize_impl<std::shared_ptr<T>>
{
    static void deserialize(std::shared_ptr<T> &t, const JToken &token)
    {
        using handler = typename deserialize_dispatcher<T>::handler;

        if (token.GetType() == JsonType::Null) {
            t = nullptr;
            return;
        }

        t = std::make_shared<T>();
        handler::deserialize(*t, token);
    }
};

}

#define DESERIALIZE(member) \
    0); v = obj[#member]; \
    if (v) { json::deserialize(t.member, *v); } (void)(0

#define DESERIALIZE_CLASS(type, ...) \
template<> \
struct deserialize_impl<type> { \
    static void deserialize(type &t, const JToken &token) { \
        static_assert(!std::is_const<type>::value); \
        if (token.GetType() != JsonType::Object) { return; } \
        const JObject &obj = static_cast<const JObject &>(token); /* NOLINT */ \
        const JToken *v = nullptr; \
        (void)(0, ##__VA_ARGS__ ,0); \
    } \
}


#endif //JSONCPP_JSONCONVERT_H
