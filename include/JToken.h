//
// Created by qife on 16/1/10.
//

#ifndef CPPPARSER_JTOKEN_H
#define CPPPARSER_JTOKEN_H

#include <string>

#include "JsonUtil.h"

namespace JsonCpp
{
    class JToken
    {
    protected:

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

        // JPath access
        virtual const JToken &SelectToken(const std::string &) const = 0;

        virtual const JToken &SelectTokens(const std::string &) const = 0;

        // for format
        virtual const std::string &ToString() const = 0;

        virtual const std::string &ToFormatString() const = 0;

        // reclaim unnecessary memory
        virtual void Reclaim() const = 0;

        virtual ~JToken() { }
    };
}

#endif //CPPPARSER_JTOKEN_H
