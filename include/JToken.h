//
// Created by qife on 16/1/10.
//

#ifndef CPPPARSER_JTOKEN_H
#define CPPPARSER_JTOKEN_H

#include <string>
#include <vector>
#include <list>
#include <limits>
#include <memory>

#include "JsonUtil.h"

namespace JsonCpp
{
    class JToken
    {
        using NodePtrList = std::vector<std::shared_ptr<ActionNode>>;

    private:
        /*
         * 解析JPath字符串.
         * @param 待解析的字符串
         * @return 解析结果,如果JPath语法错误,返回空列表;否则返回顺序的节点列表,其中第一项为$符号.
         */
        NodePtrList ParseJPath(const char *) const;

    protected:
        enum ActionType
        {
            ValueWithKey = 0x01,
            ArrayBySubscript = 0x02,
            Wildcard = 0x04,
            ReValueWithKey = 0x08,
            ReWildcard = 0x10
        };

        struct SliceData
        {
            int start;
            int end;
            unsigned int step;

            SliceData(int start = 0, int end = std::numeric_limits<int>::max(), unsigned step = 1) :
                    start(start),
                    end(end),
                    step(step) { }
        };

        union FilterData
        {
            std::vector<int> *indices;
            SliceData *slice;
            std::string *script;
            std::string *filter;
        };

        struct SubscriptData
        {
            enum FilterType
            {
                ArrayIndices,
                ArraySlice,
                Script,
                Filter,
                All
            };

            FilterType filterType;
            FilterData filterData;

            ~SubscriptData()
            {
                switch (filterType)
                {
                    case ArrayIndices:
                        if (nullptr != filterData.indices)
                        {
                            delete filterData.indices;
                            filterData.indices = nullptr;
                        }
                        return;
                    case ArraySlice:
                        if (nullptr != filterData.slice)
                        {
                            delete filterData.slice;
                            filterData.slice = nullptr;
                        }
                        return;
                    case Script:
                        if (nullptr != filterData.script)
                        {
                            delete filterData.script;
                            filterData.script = nullptr;
                        }
                        return;
                    case Filter:
                        if (nullptr != filterData.filter)
                        {
                            delete filterData.filter;
                            filterData.filter = nullptr;
                        }
                        return;
                    default:
                        return;
                }
            }
        };

        union ActionData
        {
            std::string *key;
            SubscriptData *subData;
        };

        struct ActionNode
        {
            ActionType actionType;
            ActionData actionData;

            ~ActionNode()
            {
                if (actionType == ActionType::ValueWithKey || actionType == ActionType::ReValueWithKey)
                {
                    if (nullptr != actionData.key)
                    {
                        delete actionData.key;
                        actionData.key = nullptr;
                    }
                }
                else if (actionType == ActionType::ArrayBySubscript)
                {
                    if (nullptr != actionData.subData)
                    {
                        delete actionData.subData;
                        actionData.subData = nullptr;
                    }
                }
            }
        };


        // JPath syntax parse core.
        virtual const JToken *SelectTokenCore(const NodePtrList &, unsigned int) const = 0;

        virtual void SelectTokensCore(const NodePtrList &, unsigned int, std::list<const JToken *> &) const = 0;

    public:
        virtual JValueType GetType() const = 0;

        // use to get value
        virtual operator bool() const = 0;

        virtual operator double() const = 0;

        virtual operator std::string() const = 0;

        // use to access
        virtual const JToken &operator[](const std::string &) const = 0;

        virtual const JToken &operator[](unsigned long) const = 0;

        virtual const JToken &GetValue(const std::string &) const = 0;

        virtual const JToken &GetValue(unsigned long) const = 0;

        // JPath access : $ symbol will be treated as the call object
        const JToken *SelectToken(const std::string &str) const
        {
            auto nodes = ParseJPath(str.c_str());
            if (nodes.empty())
            {
                return nullptr;
            }
            return SelectTokenCore(nodes, 1);
        }

        std::list<const JToken *> SelectTokens(const std::string &str) const
        {
            auto nodes = ParseJPath(str.c_str());
            std::list<const JToken *> tokens;
            if (!nodes.empty())
            {
                SelectTokensCore(nodes, 1, tokens);
            }
            return tokens;
        }

        // for format
        virtual const std::string &ToString() const = 0;

        virtual const std::string &ToFormatString() const = 0;

        // reclaim unnecessary memory
        virtual void Reclaim() const = 0;

        virtual ~JToken() { }
    };
}

#endif //CPPPARSER_JTOKEN_H
