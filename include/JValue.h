//
// Created by qife on 16/1/13.
//

#ifndef CPPPARSER_JVALUE_H
#define CPPPARSER_JVALUE_H

#include "JsonReader.h"

namespace JsonCpp
{
    class JValue : JToken
    {
    private:
        friend class JsonReader;

        union Value
        {
            bool bVal;
            std::string *pStr;
            double num;
        };

        struct AutoMemManage
        {
            JValueType type;
            Value value;

            AutoMemManage(JValueType t = JValueType::null) : type(t) { }

            ~AutoMemManage()
            {
                if (type == JValueType::string && value.pStr != nullptr)
                {
                    delete value.pStr;
                    value.pStr = nullptr;
                }
            }
        };

        AutoMemManage autoMem;

        JValue() { }

        const char *Parse(const char *);

    public:
        JValue(const char *str)
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

#endif //CPPPARSER_JVALUE_H
