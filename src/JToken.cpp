//
// Created by qife on 16/1/27.
//

#include "JToken.h"

using namespace JsonCpp;

NodePtrList JToken::ParseJPath(const char *str) const
{
    NodePtrList list;
    if (*str == '$')
    {
        ++str;
    }
    else
    {
        if (*str == '.' || *str == '[')
        {
            goto ret;
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
                        list.push_back(std::shared_ptr(new ActionNode(ActionType::ReWildcard)));
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
        }
    }

    error:
    list.clear();

    ret:
    return list;
}
