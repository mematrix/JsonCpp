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

        JObject() { }

        const char *Parse(const char *str);

    public:
        JObject(const char *str)
        {
            auto end = Parse(str);
            JsonUtil::AssertEndStr(end);
        }
    };
}

#endif //CPPPARSER_JOBJECT_H
