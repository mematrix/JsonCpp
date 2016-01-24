//
// Created by qife on 16/1/11.
//

#ifndef CPPPARSER_JVALUETYPE_H
#define CPPPARSER_JVALUETYPE_H

#include <string>

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

    namespace Expr
    {
        enum ExprType
        {
            Operator,
            Numeric,
            Boolean,
            Property
        };

        union ExprData
        {
            char op;
            bool bv;
            double num;
            std::string *prop;
        };

        struct ExprNode
        {
            ExprType type;
            ExprData data;

            ~ExprNode()
            {
                if (type == ExprType::Property && data.prop)
                {
                    delete data.prop;
                    data.prop = nullptr;
                }
            }
        };

        enum BoolOpType
        {
            Greater,
            Less,
            GreaterEqual,
            LessEqual,
            Equal,
            NotEqual
        };
    }
}

#endif //CPPPARSER_JVALUETYPE_H
