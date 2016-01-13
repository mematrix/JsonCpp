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
    };
}

#endif //CPPPARSER_JVALUE_H
