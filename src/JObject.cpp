//
// Created by qife on 16/1/11.
//

#include <string>
#include <locale>
#include <stack>
#include <queue>
#include <functional>

using NodeDeque = std::deque<Expr::ExprNode>;

static bool GetRePolishExpression(const char *str, NodeDeque &nodes)
{
    using namespace Expr;

    int paren = 0;
    std::queue<ExprNode> exprNodes;     // 表达式节点
    auto tmp = SkipWhiteSpace(str);
    tmp = TestSignDigit(tmp, exprNodes);

    while (true) {
        tmp = SkipWhiteSpace(tmp);
        if (*tmp == '(') {
            ExprNode node(ExprType::Operator);
            node.data.op = '(';
            exprNodes.push(std::move(node));

            tmp = SkipWhiteSpace(tmp, 1);
            tmp = TestSignDigit(tmp, exprNodes);
            ++paren;
        }

        if (!ReadNumOrProp(&tmp, exprNodes)) {
            return false;
        }

        tmp = SkipWhiteSpace(tmp);
        if (*tmp == ')') {
            if (--paren < 0) {
                return false;
            }

            ExprNode node(ExprType::Operator);
            node.data.op = ')';
            exprNodes.push(std::move(node));

            tmp = SkipWhiteSpace(tmp, 1);
        }

        if (*tmp) {
            if (!ReadOperator(&tmp, exprNodes)) {
                return false;
            }
        } else {
            break;
        }
    }
    if (paren != 0) {
        return false;
    }

    std::stack<ExprNode> opStack;   // 运算符暂存堆栈
    while (!exprNodes.empty()) {
        ExprNode &node = exprNodes.front();

        if (node.type == ExprType::Operator) {
            char op = node.data.op;
            if (opStack.empty() || op == '(') {
                opStack.push(std::move(node));
                exprNodes.pop();
                continue;
            }
            if (op == ')') {
                while (!opStack.empty()) {
                    ExprNode &tmpOpNode = opStack.top();
                    if (tmpOpNode.data.op != '(') {
                        nodes.push_back(std::move(tmpOpNode));
                        opStack.pop();
                    } else {
                        opStack.pop();
                        break;
                    }
                }

                exprNodes.pop();
                continue;
            }

            ExprNode &opNode = opStack.top();
            if (opNode.data.op == '(') {
                opStack.push(std::move(node));
                exprNodes.pop();
                continue;
            }
            if (OpLessOrEqual(op, opNode.data.op)) {
                nodes.push_back(std::move(opNode));
                opStack.pop();
                while (!opStack.empty()) {
                    ExprNode &tmpOpNode = opStack.top();
                    if (OpLessOrEqual(op, tmpOpNode.data.op)) {
                        nodes.push_back(std::move(tmpOpNode));
                        opStack.pop();
                    } else {
                        break;
                    }
                }

                opStack.push(std::move(node));
                exprNodes.pop();
            } else {
                opStack.push(std::move(node));
                exprNodes.pop();
            }
        } else {
            nodes.push_back(std::move(node));
            exprNodes.pop();
        }
    }
    while (!opStack.empty()) {
        nodes.push_back(std::move(opStack.top()));
        opStack.pop();
    }

    return true;
}

static bool ComputeRePolish(const NodeDeque &nodes, const std::function<bool(const std::string &, double *)> &get, double *result)
{
    std::stack<double> computeStack;

    for (const auto &item : nodes) {
        switch (item.type) {
            case Expr::Numeric: {
                computeStack.push(item.data.num);
                continue;
            }

            case Expr::Property: {
                double value = 0.0;
                if (get(*item.data.prop, &value)) {
                    computeStack.push(value);
                    continue;
                } else {
                    return false;
                }
            }

            case Expr::Operator: {
                if (computeStack.size() < 2) {
                    return false;
                }

                double num2 = computeStack.top();
                computeStack.pop();
                double num1 = computeStack.top();
                computeStack.pop();

                switch (item.data.op) {
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

    if (computeStack.size() == 1) {
        *result = computeStack.top();
        return true;
    }

    return false;
}

static int GetOperatorPriority(char op)
{
    switch (op) {
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

    if (*str == '-') {
        ExprNode node(ExprType::Numeric);
        node.data.num = 0;
        nodes.push(std::move(node));

        node.type = ExprType::Operator;
        node.data.op = '-';
        nodes.push(std::move(node));

        return str + 1;
    }
    if (*str == '+') {
        return str + 1;
    }

    return str;
}

static bool ReadNumOrProp(const char **str, std::queue<Expr::ExprNode> &nodes)
{
    using namespace Expr;

    auto tmp = *str;
    tmp = SkipWhiteSpace(tmp);

    if (*tmp == '@') {
        if (*(tmp + 1) != '.') {
            return false;
        }
        tmp += 2;

        unsigned int len = 0;
        auto start = tmp;
        while (true) {
            switch (*start) {
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

            if (len == 0) {
                return false;
            }

            ExprNode node(ExprType::Property);
            node.data.prop = new std::string(tmp, len);
            nodes.push(std::move(node));
            break;
        }

        *str = start;
        return true;
    } else {
        if (!std::isdigit(*tmp)) {
            return false;
        }

        char *end = nullptr;
        auto num = std::strtod(tmp, &end);
        if (nullptr == end) {
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

    switch (*tmp) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '%': {
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
