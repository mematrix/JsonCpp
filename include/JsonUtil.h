//
// Created by qife on 16/1/11.
//

#ifndef CPPPARSER_JSONUTIL_H
#define CPPPARSER_JSONUTIL_H

#include <string>
#include <locale>
#include <queue>

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
        using namespace Expr;

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

        static bool GetRePolishExpression(const char *str, std::queue<ExprNode> &nodes)
        {
            int paren = 0;
            auto tmp = SkipWhiteSpace(str);
            std::queue<ExprNode> exprNodes;
            tmp = TestSignDigit(tmp, exprNodes);

            while (true)
            {
                tmp = SkipWhiteSpace(tmp);
                if (*tmp == '(')
                {
                    ExprNode node(ExprType::Operator);
                    node.data.op = '(';
                    exprNodes.push(std::move(node));

                    ++tmp;
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

                    ++tmp;
                }

                tmp = SkipWhiteSpace(tmp);
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

            // TODO: 生成逆波兰表达式结果.压入到返回队列中
            return true;
        }

    private:
        static const char *TestSignDigit(const char *str, std::queue<ExprNode> &nodes)
        {
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

        static bool ReadNumOrProp(const char **str, std::queue<ExprNode> &nodes)
        {
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

        static bool ReadOperator(const char **str, std::queue<ExprNode> &nodes)
        {
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
