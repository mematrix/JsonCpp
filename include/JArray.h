//
// Created by qife on 16/1/12.
//

#ifndef CPPPARSER_JARRAY_H
#define CPPPARSER_JARRAY_H

#include <vector>
#include <memory>

#include "JsonReader.h"

namespace JsonCpp
{
    class JArray : public JToken
    {
    private:
        friend class JsonReader;

        std::vector<std::unique_ptr<JToken>> children;

        JArray() { }

        const char *Parse(const char *);

    public:
        JArray(const char *str)
        {
            auto end = Parse(str);
            JsonUtil::AssertEndStr(end);
        }

        virtual JValueType GetType() const = 0;

        // use to get value
        virtual operator bool() const = 0;

        virtual operator double() const = 0;

        virtual operator std::string() const = 0;

        // use to access
        virtual const JToken &operator[](const std::string &) const = 0;

        virtual const JToken &operator[](long) const = 0;

        virtual const JToken &GetValue(const std::string &) const = 0;

        virtual const JToken &GetValue(long) const = 0;

        // for format
        virtual const std::string &ToString() const = 0;
    };
}

#endif //CPPPARSER_JARRAY_H
