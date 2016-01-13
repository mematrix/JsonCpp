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
         * @param str 字符串指针,传入字符串开始位置;读取成功后,将会被设置为结束后下一个字符位置
         * @return 一个unique_ptr值,其中包装了new JToken对象
         * @exception JsonException
         */
        static std::unique_ptr<JToken> &ReadToken(const char **str);
    };
}

#endif //CPPPARSER_JSONREADER_H
