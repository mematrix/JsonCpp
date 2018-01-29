//
// Created by Charles on 2018/1/25.
//

#include <limits>
#include "JSONQuery.hpp"

using namespace json;

namespace {

class filter_base
{
    std::unique_ptr<filter_base> next;

protected:
    /**
     * only check if next is exist. if exist, call it.
     */
    void filter_next(JToken &token, std::vector<JToken *> &result, bool single) noexcept
    {
        if (next) {
            next->filter(token, result, single);
        }
    }

    /**
     * call next filter if it is not null, otherwise end filter and add result.
     */
    void filter_next_or_end(JToken &token, std::vector<JToken *> &result, bool single) noexcept
    {
        if (next) {
            next->filter(token, result, single);
        } else {
            result.push_back(&token);
        }
    }

    /**
     * do filter work.
     */
    virtual void do_filter(JToken &token, std::vector<JToken *> &result, bool single) noexcept = 0;

public:
    filter_base() = default;

    void set_next_filter(std::unique_ptr<filter_base> &&filter) noexcept
    {
        next = std::move(filter);
    }

    void filter(JToken &token, std::vector<JToken *> &result, bool single) noexcept
    {
        do_filter(token, result, single);
    }

    virtual ~filter_base() = default;
};

/**
 * recursive descent filter. run filter on self and descendant node. syntax: .. (e.g. ..prop_name or ..[...])
 */
class recursive_filter : public filter_base
{
protected:
    void do_filter(JToken &token, std::vector<JToken *> &result, bool single) noexcept override
    {
        filter_next(token, result, single);
        if (single && !result.empty()) {
            return;
        }

        if (token.GetType() == JsonType::Object) {
            auto &obj = static_cast<JObject &>(token);  // NOLINT
            for (auto &property : obj) {
                do_filter(*property.second, result, single);
                if (single && !result.empty()) {
                    return;
                }
            }
        } else if (token.GetType() == JsonType::Array) {
            auto &array = static_cast<JArray &>(token);   // NOLINT
            for (auto &element : array) {
                do_filter(*element, result, single);
                if (single && !result.empty()) {
                    return;
                }
            }
        }
    }
};

/**
 * filter object. syntax: .property_name in dot-notation or ['property_name'] in bracket-notation.
 */
class object_filter : public filter_base
{
    const std::string property_name;

public:
    explicit object_filter(std::string &&name) noexcept : property_name(std::move(name)) { }

protected:
    void do_filter(JToken &token, std::vector<JToken *> &result, bool single) noexcept override
    {
        if (token.GetType() != JsonType::Object) {
            return;
        }

        auto &obj = static_cast<JObject &>(token); // NOLINT
        auto value = obj[property_name];
        if (value) {
            filter_next_or_end(*value, result, single);
        }
    }
};

/**
 * filter object with property set. syntax: ['prop_name1','prop_name2',...] in bracket-notation or .* in dot-notation.
 */
class object_multi_filter : public filter_base
{
    const std::vector<std::string> property_set;  // if empty, means wildcard(*)

public:
    object_multi_filter() = default;

    explicit object_multi_filter(std::vector<std::string> &&prop_set) noexcept : property_set(std::move(prop_set)) { }

protected:
    void do_filter(JToken &token, std::vector<JToken *> &result, bool single) noexcept override
    {
        if (token.GetType() != JsonType::Object) {
            return;
        }

        auto &obj = static_cast<JObject &>(token); // NOLINT
        if (property_set.empty()) {
            for (auto &property : obj) {
                filter_next_or_end(*property.second, result, single);
                if (single && !result.empty()) {
                    return;
                }
            }
        } else {
            for (const auto &prop_name : property_set) {
                auto value = obj[prop_name];
                if (value) {
                    filter_next_or_end(*value, result, single);
                    if (single && !result.empty()) {
                        return;
                    }
                }
            }
        }
    }
};

/**
 * filter array. syntax: [number] in any notation. num should be positive.
 */
class array_filter : public filter_base
{
    const uint64_t index;

public:
    explicit array_filter(uint64_t i) noexcept : index(i) { }

protected:
    void do_filter(JToken &token, std::vector<JToken *> &result, bool single) noexcept override
    {
        if (token.GetType() != JsonType::Array) {
            return;
        }

        auto &array = static_cast<JArray &>(token);   // NOLINT;
        auto value = array.GetValue(index);
        if (value) {
            filter_next_or_end(*value, result, single);
        }
    }
};

/**
 * filter array with index set. syntax: [num1,num2,...] in any notation. nums should be positive.
 */
class array_multi_filter : public filter_base
{
    const std::vector<uint64_t> index_list;

public:
    explicit array_multi_filter(std::vector<uint64_t> &&idx) noexcept : index_list(std::move(idx)) { }

protected:
    void do_filter(JToken &token, std::vector<JToken *> &result, bool single) noexcept override
    {
        if (token.GetType() != JsonType::Array) {
            return;
        }

        auto &array = static_cast<JArray &>(token);   // NOLINT;
        for (auto index : index_list) {
            auto value = array.GetValue(index);
            if (value) {
                filter_next_or_end(*value, result, single);
                if (single && !result.empty()) {
                    return;
                }
            }
        }
    }
};

/**
 * filter array with slice index. syntax: [start:end:step] in any notation.
 */
class array_slice_filter : public filter_base
{
    const int64_t start;
    const int64_t end;
    const uint64_t step;

public:
    explicit array_slice_filter(int64_t s = 0, uint64_t st = 1, int64_t e = std::numeric_limits<int64_t>::max()) noexcept
            : start(s), end(e), step(st > 0 ? st : 1) { }

protected:
    void do_filter(JToken &token, std::vector<JToken *> &result, bool single) noexcept override
    {
        if (token.GetType() != JsonType::Array) {
            return;
        }

        auto &array = static_cast<JArray &>(token);   // NOLINT;
        auto s = start;
        if (s < 0) {
            s += array.Size();
            if (s < 0) {
                s = 0;
            }
        }
        auto e = end;
        if (e < 0) {
            e += array.Size();
            if (e <= 0) {
                return;
            }
        } else if (e > array.Size()) {
            e = array.Size();
        }
        if (s >= e) {
            return;
        }

        for (auto i = static_cast<uint64_t>(s), j = static_cast<uint64_t>(e); i < j; i += step) {
            auto value = array[i];
            filter_next_or_end(*value, result, single);
            if (single && !result.empty()) {
                return;
            }
        }
    }
};

/**
 * wildcard filter, can be used to filter array or object. syntax: [*] in any-notation.
 */
class wildcard_filter : public filter_base
{
protected:
    void do_filter(JToken &token, std::vector<JToken *> &result, bool single) noexcept override
    {
        if (token.GetType() == JsonType::Object) {
            auto &obj = static_cast<JObject &>(token);  // NOLINT
            for (auto &property : obj) {
                filter_next_or_end(*property.second, result, single);
                if (single && !result.empty()) {
                    return;
                }
            }
        } else if (token.GetType() == JsonType::Array) {
            auto &array = static_cast<JArray &>(token); // NOLINT
            for (auto &element : array) {
                filter_next_or_end(*element, result, single);
                if (single && !result.empty()) {
                    return;
                }
            }
        }
    }
};

/**
 * filter by script expression, not support currently. syntax: [(expr)]
 */
class script_expr_filter : public filter_base
{
    //
};

/**
 * filter array by test each child in array. syntax: [?(expr)]
 */
class array_filter_script : public filter_base
{
    //
};

}
