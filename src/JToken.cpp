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
                    str = JsonUtil::SkipWhiteSpace(str, 2);
                    if (!ReadNodeByDotRefer(&str, list, true))
                    {
                        goto error;
                    }
                    continue;
                }
                else
                {
                    str = JsonUtil::SkipWhiteSpace(str, 1);
                    if (!ReadNodeByDotRefer(&str, list))
                    {
                        goto error;
                    }
                    continue;
                }

            case '[':
            {
                str = JsonUtil::SkipWhiteSpace(str, 1);
                auto node = std::shared_ptr<ActionNode>(new ActionNode(ActionType::ArrayBySubscript));
                node->actionData.subData = new SubscriptData();
                auto data = node->actionData.subData;
                if (*str == '*')
                {
                    data->filterType = SubscriptData::All;
                }
                else if (*str == '?')
                {
                    str = JsonUtil::SkipWhiteSpace(str, 1);
                    if (*str != '(')
                    {
                        goto error;
                    }
                    str = JsonUtil::SkipWhiteSpace(str, 1);

                    Expr::BoolOpType opType = Expr::Exist;
                    const char *start = str;
                    const char *last = nullptr;
                    const char *paren = nullptr;
                    const char *op = nullptr;
                    while (*str)
                    {
                        switch (*str)
                        {
                            case '>':
                            {
                                if (opType != Expr::Exist)
                                {
                                    goto error;
                                }
                                op = str;
                                if (*++str == '=')
                                {
                                    opType = Expr::GreaterEqual;
                                    ++str;
                                }
                                else
                                {
                                    opType = Expr::Greater;
                                }
                                continue;
                            }

                            case '<':
                            {
                                if (opType != Expr::Exist)
                                {
                                    goto error;
                                }
                                op = str;
                                if (*++str == '=')
                                {
                                    opType = Expr::LessEqual;
                                    ++str;
                                }
                                else
                                {
                                    opType = Expr::Less;
                                }
                                continue;
                            }

                            case '=':
                            case '!':
                            {
                                if (opType != Expr::Exist)
                                {
                                    goto error;
                                }
                                op = str;
                                if (*(str + 1) != '=')
                                {
                                    goto error;
                                }
                                opType = *str == '=' ? Expr::Equal : Expr::NotEqual;
                                str += 2;
                                continue;
                            }

                            case ')':
                                paren = str;
                                continue;

                            case ']':
                                break;

                            case ' ':
                                continue;

                            default:
                                last = str;
                        }

                        if (last >= paren || op > paren || op > last || !last)
                        {
                            goto error;
                        }
                        break;
                    }

                    if (op)
                    {
                        std::string leftExpr(start, op - start);
                        data->filterData.filter = new Expr::BoolExpression(opType, true);
                        JsonUtil::GetRePolishExpression(leftExpr.c_str(), data->filterData.filter->leftRePolishExpr);
                    }
                }
            }

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
