//
// Created by qife on 16/1/11.
//

#ifndef CPPPARSER_JSONEXCEPTION_H
#define CPPPARSER_JSONEXCEPTION_H

namespace JsonCpp
{
    struct JsonException
    {
        const char *message;

        JsonException(const char *msg) : message(msg) { }
    };
}

#endif //CPPPARSER_JSONEXCEPTION_H
