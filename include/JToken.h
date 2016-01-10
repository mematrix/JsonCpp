//
// Created by qife on 16/1/10.
//

#ifndef CPPPARSER_JTOKEN_H
#define CPPPARSER_JTOKEN_H

#include <string>

namespace JsonCpp
{
    enum JValueType
    {
        null,
        boolean,
        number,
        string,
        object,
        array
    };

    class JToken
    {
    protected:

    public:
        virtual JValueType GetType() const = 0;

        // use to get value
        virtual operator bool() const = 0;
        virtual operator double() const = 0;
        virtual operator std::string() const = 0;

        virtual const JToken& operator [](const std::string&) const = 0;
        virtual const JToken& operator [](long) const = 0;
        virtual const JToken& GetValue(const std::string&) const = 0;
        virtual const JToken& GetValue(long) const = 0;

        virtual const std::string& ToString() const = 0;

        virtual ~JToken() { }
    };
}

#endif //CPPPARSER_JTOKEN_H
