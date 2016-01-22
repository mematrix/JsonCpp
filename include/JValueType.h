//
// Created by qife on 16/1/11.
//

#ifndef CPPPARSER_JVALUETYPE_H
#define CPPPARSER_JVALUETYPE_H

namespace JsonCpp
{
    enum JValueType
    {
        Null,
        Boolean,
        Number,
        String,
        Object,
        Array
    };

    enum AccessMode
    {
        Normal = 0x00,
        Recursive = 0x01,
        ModeMask = 0x01
    };

    enum AccessType
    {
        ValueWithKey = 0x02,
        ArrayBySubscript = 0x04,
        Wildcard = 0x08,
        TypeMask = 0x0e
    };

    enum ActionType
    {
        NmValueWithKey = AccessMode::Normal | AccessType::ValueWithKey,
        NmArrayBySubscript = AccessMode::Normal | AccessType::ArrayBySubscript,
        NmWildcard = AccessMode::Normal | AccessType::Wildcard,
        ReValueWithKey = AccessMode::Recursive | AccessType::ValueWithKey,
        ReArrayBySubscript = AccessMode::Recursive | AccessType::ArrayBySubscript,
        ReWildcard = AccessMode::Recursive | AccessType::Wildcard
    };

    enum FilterType
    {
        ArrayIndices,
        ArraySlice,
        Script,
        Filter,
        All
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
            if (actionType & AccessType::ValueWithKey)
            {
                if (nullptr != actionData.key)
                {
                    delete actionData.key;
                    actionData.key = nullptr;
                }
            }
            else if (actionType & AccessType::ArrayBySubscript)
            {
                if (nullptr != actionData.subData)
                {
                    delete actionData.subData;
                    actionData.subData = nullptr;
                }
            }
        }
    };
}

#endif //CPPPARSER_JVALUETYPE_H
