//
// Created by qife on 16/1/13.
//

bool JValue::GetExprResult(Expr::BoolExpression &expr) const
{
    if (expr.type == Expr::BoolOpType::Exist && expr.leftRePolishExpr.size() == 1)
    {
        Expr::ExprNode &node = expr.leftRePolishExpr.front();
        if (node.type == Expr::ExprType::Property)
        {
            auto prop = node.data.prop;
            if (prop->compare("value") == 0 && autoMem.type == JValueType::Boolean)
            {
                return autoMem.value.bVal;
            }
        }
    }
    else
    {
        auto getMethod = [this](const std::string &str, double *value) -> bool
        {
            if (str.compare("value") == 0 && autoMem.type == JValueType::Number)
            {
                *value = autoMem.value.num;
                return true;
            }
            return false;
        };

        double leftValue = 0.0;
        if (!JsonUtil::ComputeRePolish(expr.leftRePolishExpr, getMethod, &leftValue))
        {
            return false;
        }
        double rightValue = 0.0;
        if (!JsonUtil::ComputeRePolish(*expr.rightRePolishExpr, getMethod, &rightValue))
        {
            return false;
        }

        switch (expr.type)
        {
            case Expr::Greater:
                return leftValue > rightValue;

            case Expr::Less:
                return leftValue < rightValue;

            case Expr::GreaterEqual:
                return leftValue >= rightValue;

            case Expr::LessEqual:
                return leftValue <= rightValue;

            case Expr::Equal:
                return leftValue == rightValue;

            case Expr::NotEqual:
                return leftValue != rightValue;

            case Expr::Exist:
                break;
        }
    }

    return false;
}

JToken::NodePtrList JToken::ParseJPath(const char *str) const
{
    str = JsonUtil::SkipWhiteSpace(str);
    NodePtrList list;
    if (*str == '$')
    {
        list.push_back(std::shared_ptr<ActionNode>(new ActionNode(ActionType::RootItem)));
        // list.push_back(std::make_shared<ActionNode>(ActionType::RootItem)); // Clion bug
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
                                ++str;
                                continue;

                            case ']':
                                break;

                            case ' ':
                                ++str;
                                continue;

                            default:
                                last = str;
                                ++str;
                                continue;
                        }

                        break;
                    }
                    if (last >= paren || op > paren || op > last || !last || !*str)
                    {
                        goto error;
                    }

                    data->filterType = SubscriptData::Filter;
                    if (op)
                    {
                        std::string leftExpr(start, op);
                        data->filterData.filter = new Expr::BoolExpression(opType, true);
                        if (!JsonUtil::GetRePolishExpression(leftExpr.c_str(),
                                                             data->filterData.filter->leftRePolishExpr))
                        {
                            goto error;
                        }

                        unsigned int i = opType == Expr::Less || opType == Expr::Greater ? 1 : 2;
                        std::string rightExpr(op + i, last + 1);
                        if (!JsonUtil::GetRePolishExpression(rightExpr.c_str(),
                                                             *data->filterData.filter->rightRePolishExpr))
                        {
                            goto error;
                        }
                    }
                    else
                    {
                        std::string expr(start, last + 1);
                        data->filterData.filter = new Expr::BoolExpression(opType);
                        if (!JsonUtil::GetRePolishExpression(expr.c_str(), data->filterData.filter->leftRePolishExpr))
                        {
                            goto error;
                        }
                    }
                }
                else if (*str == '(')
                {
                    str = JsonUtil::SkipWhiteSpace(str, 1);
                    const char *start = str;
                    const char *end = nullptr;
                    const char *paren = nullptr;

                    while (*str)
                    {
                        switch (*str)
                        {
                            case '<':
                            case '>':
                            case '=':
                            case '!':
                                goto error;

                            case ')':
                                paren = str;
                                continue;

                            case ' ':
                                continue;

                            case ']':
                                break;

                            default:
                                end = str;
                        }

                        break;
                    }
                    if (end >= paren || !end || !*str)
                    {
                        goto error;
                    }

                    data->filterType = SubscriptData::Script;
                    data->filterData.script = new std::deque<Expr::ExprNode>();
                    std::string expr(start, end + 1);
                    if (!JsonUtil::GetRePolishExpression(expr.c_str(), *data->filterData.script))
                    {
                        goto error;
                    }
                }
                else
                {
                    char *numEnd = nullptr;
                    auto num = (int)std::strtol(str, &numEnd, 0);
                    if (numEnd)
                    {
                        str = JsonUtil::SkipWhiteSpace(numEnd);

                        if (*str == ',' || *str == ']')
                        {
                            data->filterType = SubscriptData::ArrayIndices;
                            data->filterData.indices = new std::vector<int>();
                            data->filterData.indices->push_back(num);

                            if (*str == ',')
                            {
                                do
                                {
                                    numEnd = nullptr;
                                    num = (int)std::strtol(str + 1, &numEnd, 0);
                                    if (!numEnd)
                                    {
                                        goto error;
                                    }
                                    data->filterData.indices->push_back(num);
                                    str = JsonUtil::SkipWhiteSpace(numEnd);
                                } while (*str == ',');

                                if (*str != ']')
                                {
                                    goto error;
                                }
                            }

                            ++str;
                            list.push_back(std::move(node));
                            continue;
                        }
                    }

                    if (*str == ':')
                    {
                        data->filterType = SubscriptData::ArraySlice;
                        data->filterData.slice = new SliceData(num);

                        str = JsonUtil::SkipWhiteSpace(str, 1);
                        if (*str == ':')
                        {
                            char *stepEnd = nullptr;
                            str = JsonUtil::SkipWhiteSpace(str, 1);
                            auto step = std::strtoul(str, &stepEnd, 0);
                            if (stepEnd)
                            {
                                data->filterData.slice->step = (unsigned int)step;
                                str = JsonUtil::SkipWhiteSpace(stepEnd);
                            }
                        }
                        else
                        {
                            char *strEnd = nullptr;
                            auto end = std::strtol(str, &strEnd, 0);
                            if (strEnd)
                            {
                                data->filterData.slice->end = (int)end;
                                str = JsonUtil::SkipWhiteSpace(strEnd);
                                if (*str == ':')
                                {
                                    strEnd = nullptr;
                                    auto step = std::strtoul(str, &strEnd, 0);
                                    if (strEnd)
                                    {
                                        data->filterData.slice->step = (unsigned int)step;
                                        str = JsonUtil::SkipWhiteSpace(strEnd);
                                    }
                                }
                            }
                        }

                        if (*str != ']')
                        {
                            goto error;
                        }
                    }
                    else
                    {
                        goto error;
                    }
                }

                ++str;
                list.push_back(std::move(node));
                continue;
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
