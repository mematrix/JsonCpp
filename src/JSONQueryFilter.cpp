//
// Created by Charles on 2018/6/15.
//

#include <cassert>
#include "JSONQueryFilter.hpp"

void json::recursive_filter::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{
    filter_next(token, result, single);
    if (single && !result.empty()) {
        return;
    }

    if (token.get_type() == json_type::object) {
        auto &obj = static_cast<json_object &>(token);  // NOLINT
        for (auto &property : obj) {
            do_filter(*property.second, result, single);
            if (single && !result.empty()) {
                return;
            }
        }
    } else if (token.get_type() == json_type::array) {
        auto &array = static_cast<json_array &>(token);   // NOLINT
        for (auto &element : array) {
            do_filter(*element, result, single);
            if (single && !result.empty()) {
                return;
            }
        }
    }
}

void json::object_filter::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{
    if (token.get_type() != json_type::object) {
        return;
    }

    auto &obj = static_cast<json_object &>(token); // NOLINT
    auto value = obj[property_name];
    if (value) {
        filter_next_or_end(*value, result, single);
    }
}

void json::object_multi_filter::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{
    if (token.get_type() != json_type::object) {
        return;
    }

    auto &obj = static_cast<json_object &>(token); // NOLINT
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

void json::array_filter::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{
    if (token.get_type() != json_type::array) {
        return;
    }

    auto &array = static_cast<json_array &>(token);   // NOLINT
    auto value = array.get_value(index);
    if (value) {
        filter_next_or_end(*value, result, single);
    }
}

void json::array_multi_filter::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{
    if (token.get_type() != json_type::array) {
        return;
    }

    auto &array = static_cast<json_array &>(token);   // NOLINT
    for (auto index : index_list) {
        auto value = array.get_value(index);
        if (value) {
            filter_next_or_end(*value, result, single);
            if (single && !result.empty()) {
                return;
            }
        }
    }
}

void json::array_slice_filter::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{
    if (token.get_type() != json_type::array) {
        return;
    }

    auto &array = static_cast<json_array &>(token);   // NOLINT
    auto s = start;
    if (s < 0) {
        s += array.size();
        if (s < 0) {
            s = 0;
        }
    }
    auto e = end;
    if (e < 0) {
        e += array.size();
        if (e <= 0) {
            return;
        }
    } else if (e > array.size()) {
        e = array.size();
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
        if (i + step < i) {
            return;     /* roll back */
        }
    }
}

void json::object_wildcard_filter::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{
    if (token.get_type() != json_type::object) {
        return;
    }

    auto &obj = static_cast<json_object &>(token);      // NOLINT
    for (auto &property : obj) {
        filter_next_or_end(*property.second, result, single);
        if (single && !result.empty()) {
            return;
        }
    }
}

void json::array_wildcard_filter::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{
    if (token.get_type() != json_type::array) {
        return;
    }

    auto &array = static_cast<json_array &>(token);     // NOLINT
    for (auto &element : array) {
        filter_next_or_end(*element, result, single);
        if (single && !result.empty()) {
            return;
        }
    }
}

void json::wildcard_filter::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{
    if (token.get_type() == json_type::object) {
        auto &obj = static_cast<json_object &>(token);  // NOLINT
        for (auto &property : obj) {
            filter_next_or_end(*property.second, result, single);
            if (single && !result.empty()) {
                return;
            }
        }
    } else if (token.get_type() == json_type::array) {
        auto &array = static_cast<json_array &>(token); // NOLINT
        for (auto &element : array) {
            filter_next_or_end(*element, result, single);
            if (single && !result.empty()) {
                return;
            }
        }
    }
}

void json::script_expr_filter::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{
    //
}

void json::array_filter_script::do_filter(json::json_token &token, std::vector<json::json_token *> &result, bool single) noexcept
{

}


std::unique_ptr<json::filter_base> json::parse_expr_script(const char **path)
{
    const char *start = *path;
    assert(*start == '(');

    return nullptr;
}

std::unique_ptr<json::filter_base> json::parse_filter_script(const char **path)
{
    const char *start = *path;
    assert(*start == '?');

    return nullptr;
}

