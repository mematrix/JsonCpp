//
// Created by qife on 16/1/11.
//

using namespace JsonCpp;
using pToken = std::unique_ptr<JToken>;
using tokenItem = std::pair<std::string, pToken>;

const JToken *JObject::SelectTokenCore(const NodePtrList &nodes, unsigned int cur) const
{
    if (nodes.size() == cur)
    {
        return this;
    }

    auto node = nodes[cur];
    switch (node->actionType)
    {
        case ActionType::ValueWithKey:
        {
            auto child = children.find(*node->actionData.key);
            if (child != children.end())
            {
                return child->second->SelectTokenCore(nodes, cur + 1);
            }
            break;
        }

        case ArrayBySubscript:
        case RootItem:
        {
            return nullptr;
        }

        case Wildcard:
        {
            for (const auto &child : children)
            {
                if (auto token = child.second->SelectTokenCore(nodes, cur + 1))
                {
                    return token;
                }
            }
            break;
        }

        case ReValueWithKey:
        {
            auto child = children.find(*node->actionData.key);
            if (child != children.end())
            {
                if (auto token = child->second->SelectTokenCore(nodes, cur + 1))
                {
                    return token;
                }
            }
            for (const auto &item : children)
            {
                if (auto token = item.second->SelectTokenCore(nodes, cur))
                {
                    return token;
                }
            }
            break;
        }

        case ReWildcard:
        {
            for (const auto &child : children)
            {
                // test whether the child matches
                if (auto curToken = child.second->SelectTokenCore(nodes, cur + 1))
                {
                    return curToken;
                }
                // test child's children
                if (auto nxtToken = child.second->SelectTokenCore(nodes, cur))
                {
                    return nxtToken;
                }
            }
            break;
        }
    }

    return nullptr;
}

void JObject::SelectTokensCore(const NodePtrList &nodes, unsigned int cur, std::list<const JToken *> &tokens) const
{
    if (nodes.size() == cur)
    {
        tokens.push_back(this);
        return;
    }

    auto node = nodes[cur];
    switch (node->actionType)
    {
        case ValueWithKey:
        {
            auto child = children.find(*node->actionData.key);
            if (child != children.end())
            {
                child->second->SelectTokensCore(nodes, cur + 1, tokens);
            }
            break;
        }

        case RootItem:
        case ArrayBySubscript:
            break;

        case Wildcard:
        {
            for (const auto &child : children)
            {
                child.second->SelectTokensCore(nodes, cur + 1, tokens);
            }
            break;
        }

        case ReValueWithKey:
        {
            auto child = children.find(*node->actionData.key);
            if (child != children.end())
            {
                child->second->SelectTokensCore(nodes, cur + 1, tokens);
            }
            for (const auto &item : children)
            {
                item.second->SelectTokensCore(nodes, cur, tokens);
            }
            break;
        }

        case ReWildcard:
        {
            for (const auto &child : children)
            {
                child.second->SelectTokensCore(nodes, cur + 1, tokens);
                child.second->SelectTokensCore(nodes, cur, tokens);
            }
            break;
        }
    }
}

bool JObject::GetExprResult(Expr::BoolExpression &expr) const
{
    if (expr.type == Expr::BoolOpType::Exist && expr.leftRePolishExpr.size() == 1)
    {
        Expr::ExprNode &node = expr.leftRePolishExpr.front();
        if (node.type == Expr::ExprType::Property)
        {
            auto prop = node.data.prop;
            return children.find(*prop) != children.end();
        }
    }
    else
    {
        auto getMethod = [this](const std::string &str, double *value) -> bool
        {
            auto child = children.find(str);
            if (child != children.end() && child->second->GetType() == JValueType::Number)
            {
                *value = (double)*child->second;
                return true;
            }

            if (str.compare("count"))
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
                return leftValue == rightValue;

            case Expr::Exist:break;
        }
    }

    return false;
}
