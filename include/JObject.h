//
// Created by qife on 16/1/10.
//

#ifndef CPPPARSER_JOBJECT_H
#define CPPPARSER_JOBJECT_H

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
        mutable std::string *fmtString;

        JObject() : children(), objString(nullptr), fmtString(nullptr) { }

        const char *Parse(const char *str);

    protected:
        // JPath syntax parse core.
        virtual const JToken *SelectTokenCore(const NodePtrList &, unsigned int) const override;

        virtual void SelectTokensCore(const NodePtrList &, unsigned int, std::list<const JToken *> &) const override;

        virtual bool GetExprResult(Expr::BoolExpression &) const override;

    public:
        JObject(const char *str) : children(), objString(nullptr), fmtString(nullptr)
        {
            auto end = Parse(str);
            JsonUtil::AssertEndStr(end);
        }

        ~JObject() { Reclaim(); }

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

#endif //CPPPARSER_JOBJECT_H
