//
// Created by qife on 16/1/27.
//

#include "JToken.h"

using namespace JsonCpp;

NodePtrList JToken::ParseJPath(const char *str) const
{
    NodePtrList list;
    if (*str == '$')
    {
        ++str;
    }
    else
    {
        if (*str == '.' || *str == '[')
        {
            goto ret;
        }
    }

    reterr:
    list.clear();

    ret:
    return list;
}
