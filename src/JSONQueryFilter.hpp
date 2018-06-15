//
// Created by Charles on 2018/6/15.
//

#ifndef JSONCPP_JSONQUERYFILTER_HPP
#define JSONCPP_JSONQUERYFILTER_HPP

#include <list>
#include <limits>
#include "JSON.hpp"

namespace json {

class filter_base
{
    std::unique_ptr<filter_base> next;

protected:
    /**
     * only check if next is exist. if exist, call it.
     */
    void filter_next(json_token &token, std::vector<json_token *> &result, bool single) noexcept
    {
        if (next) {
            next->filter(token, result, single);
        }
    }

    /**
     * call next filter if it is not null, otherwise end filter and add result.
     */
    void filter_next_or_end(json_token &token, std::vector<json_token *> &result, bool single) noexcept
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
    virtual void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept = 0;

public:
    filter_base() = default;

    filter_base *set_next_filter(std::unique_ptr<filter_base> &&filter) noexcept
    {
        next = std::move(filter);
        return next.get();
    }

    std::unique_ptr<filter_base> fetch_next_filter() noexcept
    {
        return std::move(next);
    }

    void filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept
    {
        do_filter(token, result, single);
    }

    virtual ~filter_base() = default;
};

/**
 * recursive descent filter. run filter on self and descendant node. syntax: .. (e.g. ..prop_name or ..[...])
 */
class recursive_filter final : public filter_base
{
protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

/**
 * filter object. syntax: .property_name in dot-notation or ['property_name'] in bracket-notation.
 */
class object_filter final : public filter_base
{
    const std::string property_name;

public:
    explicit object_filter(std::string &&name) noexcept : property_name(std::move(name)) { }

protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

/**
 * filter object with property set. syntax: ['prop_name1','prop_name2',...] in bracket-notation.
 */
class object_multi_filter final : public filter_base
{
    const std::list<std::string> property_set;

public:
    explicit object_multi_filter(std::list<std::string> &&prop_set) noexcept : property_set(std::move(prop_set)) { }

protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

/**
 * filter array. syntax: [number] in any notation. num should be positive.
 */
class array_filter final : public filter_base
{
    const uint64_t index;

public:
    explicit array_filter(uint64_t i) noexcept : index(i) { }

protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

/**
 * filter array with index set. syntax: [num1,num2,...] in any notation. nums should be positive.
 */
class array_multi_filter final : public filter_base
{
    const std::vector<uint64_t> index_list;

public:
    explicit array_multi_filter(std::vector<uint64_t> &&idx) noexcept : index_list(std::move(idx)) { }

protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

/**
 * filter array with slice index. syntax: [start:end:step] in any notation.
 */
class array_slice_filter final : public filter_base
{
    const int64_t start;
    const int64_t end;
    const uint64_t step;

public:
    explicit array_slice_filter(int64_t s = 0, int64_t e = std::numeric_limits<int64_t>::max(), uint64_t st = 1) noexcept
            : start(s), end(e), step(st > 0 ? st : 1) { }

protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

/**
 * object wildcard filter. syntax: .* in dot-notation.
 */
class object_wildcard_filter final : public filter_base
{
protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

/**
 * array wildcard filter. syntax: [*] in dot-notation.
 */
class array_wildcard_filter final : public filter_base
{
protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

/**
 * wildcard filter, can be used to filter array or object. syntax: [*] in bracket-notation.
 */
class wildcard_filter final : public filter_base
{
protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

/**
 * filter by script expression, not support currently. syntax: [(expr)] in any notation
 */
class script_expr_filter final : public filter_base
{
protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

/**
 * filter array by test each child in array. syntax: [?(expr)]
 */
class array_filter_script final : public filter_base
{
protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override;
};

std::unique_ptr<filter_base> parse_expr_script(const char **path);

std::unique_ptr<filter_base> parse_filter_script(const char **path);

}

#endif //JSONCPP_JSONQUERYFILTER_HPP
