//
// Created by qife on 16/1/10.
//

#ifndef CPPPARSER_JOBJECT_H
#define CPPPARSER_JOBJECT_H

#include <string>
#include <memory>
#include <map>

#include "JToken.h"

namespace JsonCpp
{
    class JObject : public JToken
    {
    private:
        std::map<std::string, std::unique_ptr<JToken>> children;

        const char *Parse(const char *str);

    public:
        JObject(const char **str) { *str = Parse(*str); }
    };
}

#endif //CPPPARSER_JOBJECT_H
