//
// Created by qife on 16/1/11.
//

#ifndef CPPPARSER_JSONUTIL_H
#define CPPPARSER_JSONUTIL_H

#include <string>
#include <locale>
#include <stack>
#include <queue>
#include <functional>


namespace JsonCpp
{


struct ActionNode;
using NodePtrList = std::vector<std::shared_ptr<ActionNode>>;

/**
 * 解析JPath字符串.
 * @param 待解析的字符串
 * @return 解析结果,如果JPath语法错误,返回空列表;否则返回顺序的节点列表,其中第一项为$符号.
 */
NodePtrList ParseJPath(const char *) const;

friend bool ReadNodeByDotRefer(const char **s, NodePtrList &list, bool isRecursive = false)
{
    auto str = *s;
    if (*str == '*') {
        ActionType t = isRecursive ? ActionType::ReWildcard : ActionType::Wildcard;
        list.push_back(std::shared_ptr<ActionNode>(new ActionNode(t)));
        *s = str + 1;
    } else {
        auto tmp = str;
        while (*tmp && *tmp != '.' && *tmp != '[' && *tmp != ' ') {
            ++tmp;
        }
        if (tmp == str) {
            return false;
        }

        ActionType t = isRecursive ? ActionType::ReValueWithKey : ActionType::ValueWithKey;
        auto node = std::shared_ptr<ActionNode>(new ActionNode(t));
        node->actionData.key = new std::string(str, tmp - str);
        list.push_back(std::move(node));
        *s = tmp;
    }

    return true;
}

enum ActionType
{
    RootItem = 0x00,
    ValueWithKey = 0x01,
    ArrayBySubscript = 0x02,
    Wildcard = 0x04,
    ReValueWithKey = 0x08,
    ReWildcard = 0x10
};

struct SliceData
{
    int start;
    int end;
    unsigned int step;

    SliceData(int start = 0, int end = std::numeric_limits<int>::max(), unsigned step = 1) :
            start(start),
            end(end),
            step(step) { }
};

union FilterData
{
    std::vector<int> *indices;
    SliceData *slice;
    std::deque<Expr::ExprNode> *script;
    Expr::BoolExpression *filter;
};

struct SubscriptData
{
    enum FilterType
    {
        ArrayIndices,
        ArraySlice,
        Script,
        Filter,
        All
    };

    FilterType filterType;
    FilterData filterData;

    ~SubscriptData()
    {
        switch (filterType) {
            case ArrayIndices:
                if (nullptr != filterData.indices) {
                    delete filterData.indices;
                    filterData.indices = nullptr;
                }
                return;
            case ArraySlice:
                if (nullptr != filterData.slice) {
                    delete filterData.slice;
                    filterData.slice = nullptr;
                }
                return;
            case Script:
                if (nullptr != filterData.script) {
                    delete filterData.script;
                    filterData.script = nullptr;
                }
                return;
            case Filter:
                if (nullptr != filterData.filter) {
                    delete filterData.filter;
                    filterData.filter = nullptr;
                }
                return;
            default:
                return;
        }
    }
};

union ActionData
{
    std::string *key;
    SubscriptData *subData;
};

struct ActionNode
{
    ActionType actionType;
    ActionData actionData;

    ActionNode(ActionType t = ActionType::Wildcard) : actionType(t)
    {
        actionData.key = nullptr;
    }

    ActionNode(ActionNode &&node) : actionType(node.actionType)
    {
        actionData.key = node.actionData.key;
        node.actionType = ActionType::Wildcard;
        node.actionData.key = nullptr;
    }

    ~ActionNode()
    {
        if (actionType == ActionType::ValueWithKey || actionType == ActionType::ReValueWithKey) {
            if (nullptr != actionData.key) {
                delete actionData.key;
                actionData.key = nullptr;
            }
        } else if (actionType == ActionType::ArrayBySubscript) {
            if (nullptr != actionData.subData) {
                delete actionData.subData;
                actionData.subData = nullptr;
            }
        }
    }
};


namespace Expr
{

enum ExprType
{
    Numeric,
    Operator,
    Boolean,
    Property
};

union ExprData
{
    char op;
    bool bv;
    double num;
    std::string *prop;
};

struct ExprNode
{
    ExprType type;
    ExprData data;

    ExprNode(ExprType t = ExprType::Numeric) : type(t) { data.prop = nullptr; }

    ExprNode(ExprNode &&node) : type(node.type)
    {
        switch (node.type)
        {
            case Numeric:
                data.num = node.data.num;
                node.data.num = 0.0;
                break;

            case Operator:
                data.op = node.data.op;
                node.data.op = '\0';
                break;

            case Boolean:
                data.bv = node.data.bv;
                node.data.bv = false;
                break;

            case Property:
                data.prop = node.data.prop;
                node.data.prop = nullptr;
                break;
        }

        node.type = Numeric;
    }

    ~ExprNode()
    {
        if (type == ExprType::Property && data.prop)
        {
            delete data.prop;
            data.prop = nullptr;
        }
    }
};

enum BoolOpType
{
    Greater,
    Less,
    GreaterEqual,
    LessEqual,
    Equal,
    NotEqual,
    Exist
};

struct BoolExpression
{
    BoolOpType type;
    std::deque<ExprNode> leftRePolishExpr;
    std::deque<ExprNode> *rightRePolishExpr;

    BoolExpression(BoolOpType t = BoolOpType::Exist, bool hasRight = false) : type(t)
    {
        if (hasRight)
        {
            rightRePolishExpr = new std::deque<ExprNode>();
        }
    }

    BoolExpression(BoolExpression &&expr) :
            type(expr.type), leftRePolishExpr(std::move(expr.leftRePolishExpr)),
            rightRePolishExpr(expr.rightRePolishExpr)
    {
        expr.type = BoolOpType::Exist;
        rightRePolishExpr = nullptr;
    }

    ~BoolExpression()
    {
        if (rightRePolishExpr)
        {
            delete rightRePolishExpr;
            rightRePolishExpr = nullptr;
        }
    }
};
}
    struct JsonUtil
    {
        using NodeDeque = std::deque<Expr::ExprNode>;

        static bool GetRePolishExpression(const char *str, NodeDeque &nodes)
        {
            using namespace Expr;

            int paren = 0;
            std::queue<ExprNode> exprNodes;     // 表达式节点
            auto tmp = SkipWhiteSpace(str);
            tmp = TestSignDigit(tmp, exprNodes);

            while (true)
            {
                tmp = SkipWhiteSpace(tmp);
                if (*tmp == '(')
                {
                    ExprNode node(ExprType::Operator);
                    node.data.op = '(';
                    exprNodes.push(std::move(node));

                    tmp = SkipWhiteSpace(tmp, 1);
                    tmp = TestSignDigit(tmp, exprNodes);
                    ++paren;
                }

                if (!ReadNumOrProp(&tmp, exprNodes))
                {
                    return false;
                }

                tmp = SkipWhiteSpace(tmp);
                if (*tmp == ')')
                {
                    if (--paren < 0)
                    {
                        return false;
                    }

                    ExprNode node(ExprType::Operator);
                    node.data.op = ')';
                    exprNodes.push(std::move(node));

                    tmp = SkipWhiteSpace(tmp, 1);
                }

                if (*tmp)
                {
                    if (!ReadOperator(&tmp, exprNodes))
                    {
                        return false;
                    }
                }
                else
                {
                    break;
                }
            }
            if (paren != 0)
            {
                return false;
            }

            std::stack<ExprNode> opStack;   // 运算符暂存堆栈
            while (!exprNodes.empty())
            {
                ExprNode &node = exprNodes.front();

                if (node.type == ExprType::Operator)
                {
                    char op = node.data.op;
                    if (opStack.empty() || op == '(')
                    {
                        opStack.push(std::move(node));
                        exprNodes.pop();
                        continue;
                    }
                    if (op == ')')
                    {
                        while (!opStack.empty())
                        {
                            ExprNode &tmpOpNode = opStack.top();
                            if (tmpOpNode.data.op != '(')
                            {
                                nodes.push_back(std::move(tmpOpNode));
                                opStack.pop();
                            }
                            else
                            {
                                opStack.pop();
                                break;
                            }
                        }

                        exprNodes.pop();
                        continue;
                    }

                    ExprNode &opNode = opStack.top();
                    if (opNode.data.op == '(')
                    {
                        opStack.push(std::move(node));
                        exprNodes.pop();
                        continue;
                    }
                    if (OpLessOrEqual(op, opNode.data.op))
                    {
                        nodes.push_back(std::move(opNode));
                        opStack.pop();
                        while (!opStack.empty())
                        {
                            ExprNode &tmpOpNode = opStack.top();
                            if (OpLessOrEqual(op, tmpOpNode.data.op))
                            {
                                nodes.push_back(std::move(tmpOpNode));
                                opStack.pop();
                            }
                            else
                            {
                                break;
                            }
                        }

                        opStack.push(std::move(node));
                        exprNodes.pop();
                    }
                    else
                    {
                        opStack.push(std::move(node));
                        exprNodes.pop();
                    }
                }
                else
                {
                    nodes.push_back(std::move(node));
                    exprNodes.pop();
                }
            }
            while (!opStack.empty())
            {
                nodes.push_back(std::move(opStack.top()));
                opStack.pop();
            }

            return true;
        }

        static bool ComputeRePolish(const NodeDeque &nodes, const std::function<bool(const std::string &, double *)> &get, double *result)
        {
            std::stack<double> computeStack;

            for (const auto &item : nodes)
            {
                switch (item.type)
                {
                    case Expr::Numeric:
                    {
                        computeStack.push(item.data.num);
                        continue;
                    }

                    case Expr::Property:
                    {
                        double value = 0.0;
                        if (get(*item.data.prop, &value))
                        {
                            computeStack.push(value);
                            continue;
                        }
                        else
                        {
                            return false;
                        }
                    }

                    case Expr::Operator:
                    {
                        if (computeStack.size() < 2)
                        {
                            return false;
                        }

                        double num2 = computeStack.top();
                        computeStack.pop();
                        double num1 = computeStack.top();
                        computeStack.pop();

                        switch (item.data.op)
                        {
                            case '+':
                                computeStack.push(num1 + num2);
                                continue;

                            case '-':
                                computeStack.push(num1 - num2);
                                continue;

                            case '*':
                                computeStack.push(num1 * num2);
                                continue;

                            case '/':
                                computeStack.push(num1 / num2);
                                continue;

                            case '%':
                                computeStack.push((int)num1 % (int)num2); // TODO: test int value
                                continue;

                            default:
                                return false;
                        }
                    }

                    case Expr::Boolean:
                        return false;
                }
            }

            if (computeStack.size() == 1)
            {
                *result = computeStack.top();
                return true;
            }

            return false;
        }

    private:
        static int GetOperatorPriority(char op)
        {
            switch (op)
            {
                case '+':
                case '-':
                    return 1;

                case '*':
                case '/':
                case '%':
                    return 3;

                default:
                    return 0;
            }
        }

        static bool OpLessOrEqual(char op1, char op2)
        {
            int p1 = GetOperatorPriority(op1);
            int p2 = GetOperatorPriority(op2);

            return p1 <= p2;
        }

        static const char *TestSignDigit(const char *str, std::queue<Expr::ExprNode> &nodes)
        {
            using namespace Expr;

            if (*str == '-')
            {
                ExprNode node(ExprType::Numeric);
                node.data.num = 0;
                nodes.push(std::move(node));

                node.type = ExprType::Operator;
                node.data.op = '-';
                nodes.push(std::move(node));

                return str + 1;
            }
            if (*str == '+')
            {
                return str + 1;
            }

            return str;
        }

        static bool ReadNumOrProp(const char **str, std::queue<Expr::ExprNode> &nodes)
        {
            using namespace Expr;

            auto tmp = *str;
            tmp = SkipWhiteSpace(tmp);

            if (*tmp == '@')
            {
                if (*(tmp + 1) != '.')
                {
                    return false;
                }
                tmp += 2;

                unsigned int len = 0;
                auto start = tmp;
                while (true)
                {
                    switch (*start)
                    {
                        case '+':
                        case '-':
                        case '*':
                        case '/':
                        case '%':
                        case ')':
                        case ' ':
                            break;

                        case '(':
                            return false;

                        default:
                            ++len;
                            ++start;
                            continue;
                    }

                    if (len == 0)
                    {
                        return false;
                    }

                    ExprNode node(ExprType::Property);
                    node.data.prop = new std::string(tmp, len);
                    nodes.push(std::move(node));
                    break;
                }

                *str = start;
                return true;
            }
            else
            {
                if (!std::isdigit(*tmp))
                {
                    return false;
                }

                char *end = nullptr;
                auto num = std::strtod(tmp, &end);
                if (nullptr == end)
                {
                    return false;
                }

                ExprNode node(ExprType::Numeric);
                node.data.num = num;
                nodes.push(std::move(node));

                *str = end;
                return true;
            }
        }

        static bool ReadOperator(const char **str, std::queue<Expr::ExprNode> &nodes)
        {
            using namespace Expr;

            auto tmp = *str;

            switch (*tmp)
            {
                case '+':
                case '-':
                case '*':
                case '/':
                case '%':
                {
                    ExprNode node(ExprType::Operator);
                    node.data.op = *tmp;
                    nodes.push(std::move(node));

                    *str = tmp + 1;
                    return true;
                }

                default:
                    return false;
            }
        }
    };
}

#endif //CPPPARSER_JSONUTIL_H
