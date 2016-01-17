//
// Created by qife on 16/1/10.
//

#ifndef CPPPARSER_JOBJECT_H
#define CPPPARSER_JOBJECT_H

#include <memory>
#include <map>

#include "JsonReader.h"

namespace JsonCpp
{
    class JObject : public JToken
    {
    private:
        friend class JsonReader;

        std::map<std::string, std::unique_ptr<JToken>> children;
        mutable std::string *objString;

        JObject() { }

        const char *Parse(const char *str);

    public:
        JObject(const char *str)
        {
            auto end = Parse(str);
            JsonUtil::AssertEndStr(end);
        }

        ~JObject()
        {
            if (objString != nullptr)
            {
                delete objString;
                objString = nullptr;
            }
        }

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
    };
}

#endif //CPPPARSER_JOBJECT_H
