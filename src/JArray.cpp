//
// Created by qife on 16/1/12.
//

#include <stack>

const JToken *JArray::SelectTokenCore(const NodePtrList &nodes, unsigned int cur) const
{
    if (nodes.size() == cur)
    {
        return this;
    }

    auto node = nodes[cur];
    if (node->actionType == ActionType::ArrayBySubscript)
    {
        auto data = node->actionData.subData;
        switch (data->filterType)
        {
            case SubscriptData::ArrayIndices:
            {
                auto len = children.size();
                for (const auto &item : *data->filterData.indices)
                {
                    if (item >= 0 && item < len)
                    {
                        if (auto token = children[item]->SelectTokenCore(nodes, cur + 1))
                        {
                            return token;
                        }
                    }
                }
                break;
            }

            case SubscriptData::ArraySlice:
            {
                auto slice = data->filterData.slice;
                if (slice->step == 0)
                {
                    return nullptr;
                }
                auto len = (int)children.size();
                auto end = slice->end >= len ? len :
                           slice->end >= 0 ? slice->end : len + slice->end;
                auto start = slice->start >= 0 ? slice->start : len + slice->start;

                for (start = start >= 0 ? start : 0; start < end; start += slice->step)
                {
                    if (auto token = children[start]->SelectTokenCore(nodes, cur + 1))
                    {
                        return token;
                    }
                }

                break;
            }

            case SubscriptData::Script:
            {
                auto script = data->filterData.script;
                double rel = 0;
                auto getMethod = [this](const std::string &str, double *value) -> bool
                {
                    if (str.compare("length"))
                    {
                        *value = children.size();
                        return true;
                    }
                    return false;
                };
                if (JsonUtil::ComputeRePolish(*script, getMethod, &rel))
                {
                    int index = (int)rel;
                    if (index >= 0 && index < children.size())
                    {
                        if (auto token = children[index]->SelectTokenCore(nodes, cur + 1))
                        {
                            return token;
                        }
                    }
                }

                break;
            }

            case SubscriptData::Filter:
            {
                auto filter = data->filterData.filter;
                for (const auto &child : children)
                {
                    if (child->GetExprResult(*filter))
                    {
                        if (auto token = child->SelectTokenCore(nodes, cur + 1))
                        {
                            return token;
                        }
                    }
                }
                break;
            }

            case SubscriptData::All:
            {
                for (const auto &child : children)
                {
                    if (auto token = child->SelectTokenCore(nodes, cur + 1))
                    {
                        return token;
                    }
                }
                break;
            }
        }
    }

    return nullptr;
}

void JArray::SelectTokensCore(const NodePtrList &nodes, unsigned int cur, std::list<const JToken *> &tokens) const
{
    if (nodes.size() == cur)
    {
        tokens.push_back(this);
        return;
    }

    auto node = nodes[cur];
    if (node->actionType != ActionType::ArrayBySubscript)
    {
        return;
    }

    auto data = node->actionData.subData;
    switch (data->filterType)
    {
        case SubscriptData::ArrayIndices:
        {
            auto len = children.size();
            for (const auto &item : *data->filterData.indices)
            {
                if (item >= 0 && item < len)
                {
                    children[item]->SelectTokensCore(nodes, cur + 1, tokens);
                }
            }
            return;
        }

        case SubscriptData::ArraySlice:
        {
            auto slice = data->filterData.slice;
            if (slice->step == 0)
            {
                return;
            }
            auto len = (int)children.size();
            auto end = slice->end >= len ? len :
                       slice->end >= 0 ? slice->end : len + slice->end;
            auto start = slice->start >= 0 ? slice->start : len + slice->start;

            for (start = start >= 0 ? start : 0; start < end; start += slice->step)
            {
                children[start]->SelectTokensCore(nodes, cur + 1, tokens);
            }

            return;
        }

        case SubscriptData::Script:
        {
            auto script = data->filterData.script;
            double rel = 0;
            auto getMethod = [this](const std::string &str, double *value) -> bool
            {
                if (str.compare("length"))
                {
                    *value = children.size();
                    return true;
                }
                return false;
            };
            if (JsonUtil::ComputeRePolish(*script, getMethod, &rel))
            {
                int index = (int)rel;
                if (index >= 0 && index < children.size())
                {
                    children[index]->SelectTokensCore(nodes, cur + 1, tokens);
                }
            }

            return;
        }

        case SubscriptData::Filter:
        {
            auto filter = data->filterData.filter;
            for (const auto &child : children)
            {
                if (child->GetExprResult(*filter))
                {
                    child->SelectTokensCore(nodes, cur + 1, tokens);
                }
            }

            return;
        }

        case SubscriptData::All:
        {
            for (const auto &child : children)
            {
                child->SelectTokensCore(nodes, cur + 1, tokens);
            }

            return;
        }
    }
}

bool JArray::GetExprResult(Expr::BoolExpression &expr) const
{
    if (expr.type == Expr::BoolOpType::Exist)
    {
        return false;
    }

    auto getMethod = [this](const std::string &str, double *value) -> bool
    {
        if (str.compare("length") == 0)
        {
            *value = children.size();
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

        default:
            return false;
    }
}
