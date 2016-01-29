//
// Created by qife on 16/1/27.
//

#include "JToken.h"

using namespace JsonCpp;

NodePtrList JToken::ParseJPath(const char *str) const
{
    str = JsonUtil::SkipWhiteSpace(str);
    NodePtrList list;
    if (*str == '$')
    {
        list.push_back(std::shared_ptr<ActionNode>(new ActionNode(ActionType::RootItem)));
        ++str;
    }
    else
    {
        list.push_back(std::shared_ptr<ActionNode>(new ActionNode(ActionType::RootItem)));
        if (!ReadNodeByDotRefer(&str, list))
        {
            goto error;
        }
    }

    while (*str)
    {
        switch (*str)
        {
            case '.':
                if (*(str + 1) == '.')
                {
                    str += 2;
                    if (*str == '*')
                    {
                        list.push_back(std::shared_ptr<ActionNode>(new ActionNode(ActionType::ReWildcard)));
                        ++str;
                        continue;
                    }

                    auto tmp = str;
                    while (*tmp && *tmp != '.' && *tmp != '[' && *tmp != ' ')
                    {
                        ++tmp;
                    }
                    if (tmp == str)
                    {
                        goto error;
                    }
                    auto node = std::shared_ptr<ActionNode>(new ActionNode(ActionType::ReValueWithKey));
                    node->actionData.key = new std::string(str, tmp - str);
                    list.push_back(std::move(node));
                    str = tmp;
                    continue;
                }
                else
                {
                    str = JsonUtil::SkipWhiteSpace(str, 1);
                    if (*str == '*')
                    {
                        list.push_back(std::shared_ptr<ActionNode>(new ActionNode(ActionType::Wildcard)));
                        ++str;
                        continue;
                    }

                    auto tmp = str;
                    while (*tmp && *tmp != '.' && *tmp != '[' && *tmp != ' ')
                    {
                        ++tmp;
                    }
                    if (tmp == str)
                    {
                        goto error;
                    }
                    auto node = std::shared_ptr<ActionNode>(new ActionNode(ActionType::ValueWithKey));
                    node->actionData.key = new std::string(str, tmp - str);
                    list.push_back(std::move(node));
                    str = tmp;
                    continue;
                }

            case '[':
                break;

            case ' ':
                continue;

            default:
                goto error;
        }
    }

    return list;

    error:
    list.clear();
    return list;
}
