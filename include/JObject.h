//
// Created by qife on 16/1/10.
//

#ifndef CPPPARSER_JOBJECT_H
#define CPPPARSER_JOBJECT_H

#include <string>
#include <map>

#include "JToken.h"

namespace JsonCpp
{
    class JObject : public JToken
    {
    private:
        std::map<std::string, JToken> children;

        void Parse(const std::string& str);

    public:
        JObject(const std::string& str) { Parse(str); }
    };
}

#endif //CPPPARSER_JOBJECT_H
