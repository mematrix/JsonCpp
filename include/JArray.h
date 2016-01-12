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
    };
}

#endif //CPPPARSER_JARRAY_H
