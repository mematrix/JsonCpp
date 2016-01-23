//
// Created by qife on 16/1/12.
//

#ifndef CPPPARSER_JARRAY_H
#define CPPPARSER_JARRAY_H

#include "JsonReader.h"

namespace JsonCpp
{
    class JArray : public JToken
    {
    private:
        friend class JsonReader;

        std::vector<std::unique_ptr<JToken>> children;
        mutable std::string *aryString;
        mutable std::string *fmtString;

        JArray() : children(), aryString(nullptr), fmtString(nullptr) { }

        const char *Parse(const char *);

    protected:
        // JPath syntax parse core.
        virtual const JToken *SelectTokenCore(const NodePtrList &, unsigned int) const override;

        virtual void SelectTokensCore(const NodePtrList &, unsigned int, std::list<const JToken *> &) const override;

    public:
        JArray(const char *str) : children(), aryString(nullptr), fmtString(nullptr)
        {
            auto end = Parse(str);
            JsonUtil::AssertEndStr(end);
        }

        ~JArray() { Reclaim(); }

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

#endif //CPPPARSER_JARRAY_H
