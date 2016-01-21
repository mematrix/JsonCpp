//
// Created by qife on 16/1/10.
//

#ifndef CPPPARSER_JTOKEN_H
#define CPPPARSER_JTOKEN_H

#include <string>
#include <vector>

#include "JsonUtil.h"

namespace JsonCpp
{
    class JToken
    {
    protected:
        // TODO: 添加内部类型,用于将传入字符串解析成语法树,再调用Core方法.
        // JPath syntax parse core.
        virtual const JToken *SelectTokenCore() const = 0;

        virtual const std::vector<const JToken &> SelectTokensCore() const = 0;

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

        // JPath access : ignore root syntax($)
        virtual const JToken *SelectToken(const std::string &) const = 0;

        virtual std::vector<const JToken &> SelectTokens(const std::string &) const = 0;

        // for format
        virtual const std::string &ToString() const = 0;

        virtual const std::string &ToFormatString() const = 0;

        // reclaim unnecessary memory
        virtual void Reclaim() const = 0;

        virtual ~JToken() { }
    };
}

#endif //CPPPARSER_JTOKEN_H
