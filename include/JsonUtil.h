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

#include "JValueType.h"
#include "JsonException.h"

#define MAKE_VALUE_TYPE_STR(infoStr) Invalid character when parse json##infoStr

#define MAKE_STR(str) #str

#define MAKE_VALUE_TYPE_INFO(type, infoStr) \
    template<> struct Info<type> { static const char*GetInfo() { return MAKE_STR(MAKE_VALUE_TYPE_STR(infoStr)); } }

namespace JsonCpp
{
    template<JValueType type>
    struct Info
    {
        static const char *GetInfo() { return "Error when parse json value"; }
    };

    MAKE_VALUE_TYPE_INFO(JValueType::Object, _object);

    MAKE_VALUE_TYPE_INFO(JValueType::Array, _array);

    struct JsonUtil
    {
        using NodeDeque = std::deque<Expr::ExprNode>;

        static const char *SkipWhiteSpace(const char *str)
        {
            while (std::isspace(*str))
            {
                ++str;
            }
            return str;
        }

        static const char *SkipWhiteSpace(const char *str, unsigned int start)
        {
            str += start;

            return SkipWhiteSpace(str);
        }

        template<JValueType type = JValueType::Null>
        static void AssertEqual(char a, char b)
        {
            if (a != b)
            {
                throw JsonException(Info<type>::GetInfo());
            }
        }

        static void Assert(bool b)
        {
            if (!b)
            {
                throw JsonException("Incorrect json string: bool");
            }
        }

        static void AssertEndStr(const char *str)
        {
            str = SkipWhiteSpace(str);
            if (*str != '\0')
            {
                throw JsonException("Incorrect json string: end");
            }
        }


        static int Unicode2ToUtf8(unsigned short uChar, char *utf8Str)
        {
            if (uChar <= 0x007f)
            {
                // * U-00000000 - U-0000007F:  0xxxxxxx
                *utf8Str = static_cast<char>(uChar);
                return 1;
            }

            if (uChar <= 0x07ff)
            {
                // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
                utf8Str[1] = static_cast<char>((uChar & 0x3f) | 0x80);
                utf8Str[0] = static_cast<char>(((uChar >> 6) & 0x1f) | 0xc0);
                return 2;
            }

            // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
            utf8Str[2] = static_cast<char>((uChar & 0x3f) | 0x80);
            utf8Str[1] = static_cast<char>(((uChar >> 6) & 0x3f) | 0x80);
            utf8Str[0] = static_cast<char>(((uChar >> 12) & 0x0f) | 0xe0);
            return 3;
        }

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
