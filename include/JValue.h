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

            AutoMemManage(JValueType t = JValueType::Null) : type(t) { }

            ~AutoMemManage()
            {
                if (type == JValueType::String && value.pStr != nullptr)
                {
                    delete value.pStr;
                    value.pStr = nullptr;
                }
            }
        };

        AutoMemManage autoMem;
        mutable std::string *valString;

        JValue() : valString(nullptr) { }

        const char *Parse(const char *);

    protected:
        // JPath syntax parse core.
        virtual const JToken *SelectTokenCore(std::list<ActionNode> &) const override;

        virtual void SelectTokensCore(std::list<ActionNode> &, std::list<const JToken *> &) const override;

    public:
        JValue(const char *str) : valString(nullptr)
        {
            auto end = Parse(str);
            JsonUtil::AssertEndStr(end);
        }

        ~JValue() { Reclaim(); }

        virtual JValueType GetType() const override;

        // use to get value
        virtual operator bool() const override;

        virtual operator double() const override;

        virtual operator std::string() const override;

        // use to access
        virtual const JToken &operator[](const std::string &) const override;

        virtual const JToken &operator[](unsigned long) const override;

        virtual const JToken &GetValue(const std::string &) const override;

        virtual const JToken &GetValue(unsigned long) const override;

        // for format
        virtual const std::string &ToString() const override;

        virtual const std::string &ToFormatString() const override;

        // reclaim unnecessary memory
        virtual void Reclaim() const override;
    };
}

#endif //CPPPARSER_JVALUE_H
