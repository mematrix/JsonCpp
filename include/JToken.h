//
// Created by qife on 16/1/10.
//

#ifndef CPPPARSER_JTOKEN_H
#define CPPPARSER_JTOKEN_H

#include <string>
#include <list>
#include <limits>

#include "JsonUtil.h"

namespace JsonCpp
{
    class JToken
    {
    private:
        /*
         * 解析JPath字符串.
         * @param 待解析的字符串
         * @return 解析结果,如果JPath语法错误,返回空列表;否则返回顺序的节点列表,其中第一项为$符号.
         */
        std::list<ActionNode> ParseJPath(const char *) const;

    protected:
        // JPath syntax parse core.
        virtual const JToken *SelectTokenCore(std::list<ActionNode> &) const = 0;

        virtual void SelectTokensCore(std::list<ActionNode> &, std::list<const JToken *> &) const = 0;

    public:
        virtual JValueType GetType() const = 0;

        // use to get value
        virtual operator bool() const = 0;

        virtual operator double() const = 0;

        virtual operator std::string() const = 0;

        // use to access
        virtual const JToken &operator[](const std::string &) const = 0;

        virtual const JToken &operator[](unsigned long) const = 0;

        virtual const JToken &GetValue(const std::string &) const = 0;

        virtual const JToken &GetValue(unsigned long) const = 0;

        // JPath access : $ symbol will be treated as the call object
        const JToken *SelectToken(const std::string &str) const
        {
            auto nodes = ParseJPath(str.c_str());
            if (nodes.size() == 0)
            {
                return nullptr;
            }
            nodes.pop_front();
            return SelectTokenCore(nodes);
        }

        std::list<const JToken *> SelectTokens(const std::string &str) const
        {
            auto nodes = ParseJPath(str.c_str());
            std::list<const JToken *> tokens;
            if (nodes.size() != 0)
            {
                nodes.pop_front();
                SelectTokensCore(nodes, tokens);
            }
            return tokens;
        }

        // for format
        virtual const std::string &ToString() const = 0;

        virtual const std::string &ToFormatString() const = 0;

        // reclaim unnecessary memory
        virtual void Reclaim() const = 0;

        virtual ~JToken() { }
    };
}

#endif //CPPPARSER_JTOKEN_H
