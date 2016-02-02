//
// Created by qife on 16/1/12.
//

#include <stack>

#include "JArray.h"

using namespace JsonCpp;

const char *JArray::Parse(const char *str)
{
    str = JsonUtil::SkipWhiteSpace(str);
    JsonUtil::AssertEqual<JValueType::Array>(*str, '[');
    // empty array
    str = JsonUtil::SkipWhiteSpace(str, 1);
    if (*str == ']')
    {
        return str + 1;
    }

    while (true)
    {
        children.push_back(JsonReader::ReadToken(&str));

        str = JsonUtil::SkipWhiteSpace(str);
        if (*str == ',')
        {
            ++str;
            continue;
        }

        JsonUtil::AssertEqual<JValueType::Array>(*str, ']');
        return str + 1;
    }
}

const JToken *JArray::SelectTokenCore(const NodePtrList &nodes, unsigned int cur) const
{
    if (nodes.size() == cur)
    {
        return this;
    }

    auto node = nodes[cur - 1];
    if (node->actionType == ActionType::ArrayBySubscript)
    {
        auto data = node->actionData.subData;
        switch (data->filterType)
        {
            case SubscriptData::ArrayIndices:
            {
                auto len = children.size();
                for (const auto &item : *data->filterData.indices)
                {
                    if (item >= 0 && item < len)
                    {
                        if (auto token = children[item]->SelectTokenCore(nodes, cur + 1))
                        {
                            return token;
                        }
                    }
                }
                break;
            }

            case SubscriptData::ArraySlice:
            {
                auto slice = data->filterData.slice;
                if (slice->step == 0)
                {
                    return nullptr;
                }
                auto len = (int)children.size();
                auto end = slice->end >= len ? len :
                           slice->end >= 0 ? slice->end : len + slice->end;
                auto start = slice->start >= 0 ? slice->start : len + slice->start;

                for (start = start >= 0 ? start : 0; start < end; start += slice->step)
                {
                    if (auto token = children[start]->SelectTokenCore(nodes, cur + 1))
                    {
                        return token;
                    }
                }

                break;
            }

            case SubscriptData::Script:
            {
                auto script = data->filterData.script;
                int index = 0;
                auto getMethod = [&children](const std::string &str, int *value) -> bool
                {
                    if (str.compare("length"))
                    {
                        *value = (int)children.size();
                        return true;
                    }
                    return false;
                };
                if (JsonUtil::ComputeRePolish(*script, getMethod, &index))
                {
                    if (index >= 0 && index < children.size())
                    {
                        if (auto token = children[index]->SelectTokenCore(nodes, cur + 1))
                        {
                            return token;
                        }
                    }
                }

                break;
            }

            case SubscriptData::Filter:
            {
                auto filter = data->filterData.filter;
                for (const auto &child : children)
                {
                    if (child->GetExprResult(*filter))
                    {
                        if (auto token = child->SelectTokenCore(nodes, cur + 1))
                        {
                            return token;
                        }
                    }
                }
                break;
            }

            case SubscriptData::All:
            {
                for (const auto &child : children)
                {
                    if (auto token = child->SelectTokenCore(nodes, cur + 1))
                    {
                        return token;
                    }
                }
                break;
            }
        }
    }

    return nullptr;
}

void JArray::SelectTokensCore(const NodePtrList &, unsigned int, std::list<const JToken *> &) const
{

}

JValueType JArray::GetType() const
{
    return JValueType::Array;
}

JArray::operator bool() const
{
    return !children.empty();
}

JArray::operator double() const
{
    return children.size();
}

JArray::operator std::string() const
{
    return ToString();
}

const JToken &JArray::operator[](const std::string &str) const
{
    return GetValue(str);
}

const JToken &JArray::operator[](unsigned long index) const
{
    return GetValue(index);
}

const JToken &JArray::GetValue(const std::string &str) const
{
    throw JsonException("Access not support");
}

const JToken &JArray::GetValue(unsigned long index) const
{
    return children.at(index).operator*();
}

const std::string &JArray::ToString() const
{
    if (nullptr == aryString)
    {
        aryString = new std::string();
        aryString->push_back('[');
        if (!children.empty())
        {
            for (const auto &child : children)
            {
                aryString->append(child->ToString());
                aryString->push_back(',');
            }
            aryString->pop_back();  // remove last ','
        }
        aryString->push_back(']');
    }

    return *aryString;
}

const std::string &JArray::ToFormatString() const
{
    if (nullptr == fmtString)
    {
        fmtString = new std::string();
        fmtString->append("[\n");
        if (!children.empty())
        {
            for (const auto &child : children)
            {
                for (const auto &c : child->ToFormatString())
                {
                    if (c == '\n')
                    {
                        fmtString->append("\n\t");
                    }
                    else
                    {
                        fmtString->push_back(c);
                    }
                }
                fmtString->append(",\n");
            }
            fmtString->pop_back();
            fmtString->pop_back();
            fmtString->push_back('\n');
        }
        fmtString->push_back(']');
    }

    return *fmtString;
}

void JArray::Reclaim() const
{
    if (aryString)
    {
        delete aryString;
        aryString = nullptr;
    }
    if (fmtString)
    {
        delete fmtString;
        fmtString = nullptr;
    }
}
