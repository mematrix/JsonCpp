//
// Created by qife on 16/1/11.
//

#ifndef CPPPARSER_JSONREADER_H
#define CPPPARSER_JSONREADER_H

#include "JToken.h"

namespace JsonCpp
{
    class JsonReader
    {
    public:
        /**
         * @param str 字符串开始位置
         * @param pToken 如果读取成功,返回new JToken对象
         * @return 读取结束后字符串指针,如果读取到字符串结尾,则返回null
         */
        static const char* ReadToken(const char *str, JToken **pToken);
    };
}

#endif //CPPPARSER_JSONREADER_H
