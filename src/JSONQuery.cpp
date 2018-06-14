//
// Created by Charles on 2018/1/25.
//

#include "JSONQuery.hpp"
#include "JSONQueryFilter.hpp"
#include "JSONUtils.hpp"

using namespace json;

namespace {

/**
 * sentry filter class, only used to construct the filter chain.
 */
class sentry_filter final : public filter_base
{
protected:
    void do_filter(json_token &token, std::vector<json_token *> &result, bool single) noexcept override
    {
        filter_next(token, result, single);
    }
};

/**
 * parser state when parsing json-path string. a state starting with dot_* means parsing dot-notation
 * json-path expression, while a state starting with bracket_* means parsing bracket-notation json-path
 * expression, and other states mean notation is not yet determined.
 */
enum class parser_state
{
    start,
    start_dollar,
    one_dot,
    two_dot,
    start_bracket,
    check_end_bracket,
    end_bracket,
    dot_parse_object,
    dot_continue,
    dot_one_dot,
    dot_two_dot,
    dot_start_bracket,
    dot_check_end_bracket,
    dot_wildcard,
    bracket_parse_object,
    bracket_start_bracket,
    bracket_check_end_bracket,
    bracket_end_bracket,
    bracket_wildcard
};

}

inline static bool is_alnum_or_line(char c)
{
    return std::isalnum(c) || c == '_' || c == '-';
}

/**
 * parse object key or key list in bracket-notation. when parse success, **path == ']'.
 */
static std::unique_ptr<filter_base> parse_bracket_object_key(const char **path)
{
    const char *start = *path;
    int code = NO_ERROR;
    std::list<std::string> key_list;

    while (true) {
        if (*start != '\'') {
            return nullptr;
        }

        ++start;
        auto key_str = read_json_string(&start, &code);
        if (code != NO_ERROR) {
            return nullptr;
        }

        if (!key_str.empty()) {
            key_list.push_back(std::move(key_str));
        }

        start = skip_whitespace(start);
        if (*start == ']') {
            break;
        }
        if (*start != ',') {
            return nullptr;
        }
        start = skip_whitespace(start, 1);
    }

    if (key_list.empty()) {
        return nullptr;
    }

    return std::unique_ptr<filter_base>(new object_multi_filter(std::move(key_list)));
}

/**
 * parse object key access syntax in dot-notation.
 */
static std::unique_ptr<filter_base> parse_dot_object_key(const char **path)
{
    const char *start = *path;
    const char *end = start;
    while (is_alnum_or_line(*end)) {
        ++end;
    }

    if (end == start) {
        return nullptr;
    }
    *path = end;

    return std::unique_ptr<filter_base>(new object_filter(std::string(start, end - start)));
}

/**
 * parse array index syntax or script syntax in any notation. when parse success, **path == ']'.
 */
static std::unique_ptr<filter_base> parse_array_index_or_script(const char **path)
{
    const char *start = *path;

    if (*start == '(') {
        auto expr_script = parse_expr_script(&start);
        *path = skip_whitespace(start);
        return expr_script;
    }
    if (*start == '?') {
        auto filter_script = parse_filter_script(&start);
        *path = skip_whitespace(start);
        return filter_script;
    }

    // parse number index syntax
    uint64_t first_num = 0;
    int64_t slice_start = 0;
    int64_t slice_end = std::numeric_limits<int64_t>::max();
    uint64_t slice_step = 1;
    bool is_slice_syntax = false;
    char *end;

    // if first char equals ':', then treat the following string as the slice operator description.
    // because subscript and subscript list are positive number, so if first char equals '-', treat
    // as slice operator too.
    if (*start == '-') {
        is_slice_syntax = true;
        slice_start = std::strtoll(start, &end, 10);
        if (end == start || errno == ERANGE) {
            return nullptr;
        }

        start = skip_whitespace(end);
        if (*start != ':') {
            return nullptr;
        }
        ++start;
    } else if (*start == ':') {
        is_slice_syntax = true;
        ++start;
    } else {
        first_num = std::strtoull(start, &end, 10);
        if (end == start || errno == ERANGE) {
            return nullptr;
        }

        start = skip_whitespace(end);
        if (*start == ']') {
            *path = start;
            return std::unique_ptr<filter_base>(new array_filter(first_num));
        }
        if (*start == ':') {
            is_slice_syntax = true;
            slice_start = static_cast<int64_t>(first_num);
            if (slice_start < 0) {
                return nullptr;     // overflow.
            }
            ++start;
        } else if (*start == ',') {
            ++start;
        } else {
            return nullptr;
        }
    }

    start = skip_whitespace(start);
    if (is_slice_syntax) {          /* [num: or [: */
        do {
            if (*start == ']') {        /* [num:] or [:] */
                break;
            }

            if (*start != ':') {        /* [num:num or [:num */
                slice_end = std::strtoll(start, &end, 10);
                if (end == start || errno == ERANGE) {
                    return nullptr;
                }

                start = skip_whitespace(end);
                if (*start == ']') {    /* [num:num] or [:num] */
                    break;
                }
            }

            if (*start != ':') {
                return nullptr;
            }
            /* [num:num: or [:num: or [num:: or [:: */
            start = skip_whitespace(start, 1);
            if (*start == ']') {    /* [num:num:] or [:num:] or [num::] or [::] */
                break;
            }

            slice_step = std::strtoull(start, &end, 10);
            if (end == start || errno == ERANGE) {
                return nullptr;
            }
            start = skip_whitespace(end);
            if (*start != ']') {
                return nullptr;
            }
            /* [num:num:num] or [:num:num] or [num::num] or [::num] */
        } while (false);

        *path = start;
        return std::unique_ptr<filter_base>(new array_slice_filter(slice_start, slice_end, slice_step));
    }

    /* index list */
    std::vector<uint64_t> indices;
    indices.push_back(first_num);
    do {
        first_num = std::strtoull(start, &end, 10);
        if (end == start || errno == ERANGE) {
            return nullptr;
        }

        indices.push_back(first_num);
        start = skip_whitespace(end);
        if (*start == ']') {
            break;
        }
        if (*start != ',') {
            return nullptr;
        }
        start = skip_whitespace(start, 1);
    } while (true);

    *path = start;
    return std::unique_ptr<filter_base>(new array_multi_filter(std::move(indices)));
}

static std::unique_ptr<filter_base> parse_filter(const char *path)
{
    parser_state state = parser_state::start;
    const char *last_handle_pos = path;
    std::unique_ptr<filter_base> sentry(new sentry_filter());
    filter_base *cur = sentry.get();

    while (*path) {
        bool handled = last_handle_pos == path;
        path = skip_whitespace(path);
        if (handled) {
            last_handle_pos = path;
            if (*path == '\0') {
                break;
            }
        }

        char c = *path;
rerun:
        switch (state) {
            case parser_state::start: {
                if (c == '$') {
                    state = parser_state::start_dollar;
                    break;
                }
                state = parser_state::dot_parse_object;
                goto rerun;
            }

            case parser_state::start_dollar: {
                if (c == '.') {
                    state = parser_state::one_dot;
                } else if (c == '[') {
                    state = parser_state::start_bracket;
                } else {
                    return nullptr;
                }
                break;
            }

            case parser_state::one_dot: {
                if (c == '.') {
                    state = parser_state::two_dot;
                    break;
                } else if (c == '*') {
                    state = parser_state::dot_wildcard;
                    goto rerun;
                } else {
                    state = parser_state::dot_parse_object;
                    goto rerun;
                }
            }

            case parser_state::two_dot: {
                cur = cur->set_next_filter(std::unique_ptr<filter_base>(new recursive_filter()));

                if (c == '*') {
                    state = parser_state::dot_wildcard;
                    goto rerun;
                } else if (c == '[') {
                    state = parser_state::start_bracket;
                    break;
                } else {
                    state = parser_state::dot_parse_object;
                    goto rerun;
                }
            }

            case parser_state::start_bracket: {
                if (c == '*') {
                    state = parser_state::bracket_wildcard;
                    goto rerun;
                } else if (c == '\'') {
                    state = parser_state::bracket_parse_object;
                    goto rerun;
                } else {
                    auto filter = parse_array_index_or_script(&path);
                    if (filter) {
                        cur = cur->set_next_filter(std::move(filter));
                        state = parser_state::check_end_bracket;
                        goto rerun;
                    } else {
                        return nullptr;
                    }
                }
            }

            case parser_state::check_end_bracket: {
                if (*path != ']') {     // goto rerun, so value of `c` did not update.
                    return nullptr;
                }

                last_handle_pos = ++path;
                state = parser_state::end_bracket;
                continue;
            }

            case parser_state::end_bracket: {
                if (c == '.') {
                    state = parser_state::one_dot;
                } else if (c == '[') {
                    state = parser_state::start_bracket;
                } else {
                    return nullptr;
                }
                break;
            }

            case parser_state::dot_parse_object: {
                auto filter = parse_dot_object_key(&path);
                if (filter) {
                    cur = cur->set_next_filter(std::move(filter));
                    last_handle_pos = path;
                    state = parser_state::dot_continue;
                    continue;
                } else {
                    return nullptr;
                }
            }

            case parser_state::dot_continue: {
                if (c == '.') {
                    state = parser_state::dot_one_dot;
                } else if (c == '[') {
                    state = parser_state::dot_start_bracket;
                } else {
                    return nullptr;
                }
                break;
            }

            case parser_state::dot_one_dot: {
                if (c == '.') {
                    state = parser_state::dot_two_dot;
                    break;
                } else if (c == '*') {
                    state = parser_state::dot_wildcard;
                    goto rerun;
                } else {
                    state = parser_state::dot_parse_object;
                    goto rerun;
                }
            }

            case parser_state::dot_two_dot: {
                cur = cur->set_next_filter(std::unique_ptr<filter_base>(new recursive_filter()));

                if (c == '*') {
                    state = parser_state::dot_wildcard;
                    goto rerun;
                } else if (c == '[') {
                    state = parser_state::dot_start_bracket;
                    break;
                } else {
                    state = parser_state::dot_parse_object;
                    goto rerun;
                }
            }

            case parser_state::dot_start_bracket: {
                if (c == '*') {
                    cur = cur->set_next_filter(std::unique_ptr<filter_base>(new array_wildcard_filter()));
                    state = parser_state::dot_check_end_bracket;
                    break;
                }

                auto filter = parse_array_index_or_script(&path);
                if (filter) {
                    cur = cur->set_next_filter(std::move(filter));
                    state = parser_state::dot_check_end_bracket;
                    goto rerun;
                } else {
                    return nullptr;
                }
            }

            case parser_state::dot_check_end_bracket: {
                if (*path != ']') {     // goto rerun. we should use current `*path` value.
                    return nullptr;
                }
                last_handle_pos = ++path;
                state = parser_state::dot_continue;
                continue;
            }

            case parser_state::dot_wildcard: {
                cur = cur->set_next_filter(std::unique_ptr<filter_base>(new object_wildcard_filter()));
                last_handle_pos = ++path;
                state = parser_state::dot_continue;
                continue;
            }

            case parser_state::bracket_parse_object: {
                auto filter = parse_bracket_object_key(&path);
                if (filter) {
                    cur = cur->set_next_filter(std::move(filter));
                    state = parser_state::bracket_check_end_bracket;
                    goto rerun;
                } else {
                    return nullptr;
                }
            }

            case parser_state::bracket_start_bracket: {
                if (c == '*') {
                    state = parser_state::bracket_wildcard;
                    goto rerun;
                } else if (c == '\'') {
                    state = parser_state::bracket_parse_object;
                    goto rerun;
                } else {
                    auto filter = parse_array_index_or_script(&path);
                    if (filter) {
                        cur = cur->set_next_filter(std::move(filter));
                        state = parser_state::bracket_check_end_bracket;
                        goto rerun;
                    } else {
                        return nullptr;
                    }
                }
            }

            case parser_state::bracket_check_end_bracket: {
                if (*path != ']') {     // goto rerun. value of `c` is out of date.
                    return nullptr;
                }

                last_handle_pos = ++path;
                state = parser_state::bracket_end_bracket;
                continue;
            }

            case parser_state::bracket_end_bracket: {
                if (c == '[') {
                    state = parser_state::bracket_start_bracket;
                    break;
                } else {
                    return nullptr;
                }
            }

            case parser_state::bracket_wildcard: {
                cur = cur->set_next_filter(std::unique_ptr<filter_base>(new wildcard_filter()));
                state = parser_state::bracket_check_end_bracket;
                break;
            }
        }

        ++path;
    }

    if (last_handle_pos != path) {
        return nullptr;
    }

    return sentry->fetch_next_filter();
}

json_token *json::select_token(json_token &token, const char *path)
{
    auto filter = parse_filter(path);
    if (!filter) {
        return nullptr;
    }

    std::vector<json_token *> result;
    filter->filter(token, result, true);
    if (result.empty()) {
        return nullptr;
    }
    return result.front();
}

std::vector<json_token *> json::select_tokens(json_token &token, const char *path)
{
    auto filter = parse_filter(path);
    if (!filter) {
        return nullptr;
    }

    std::vector<json_token *> result;
    filter->filter(token, result, false);
    return result;
}
