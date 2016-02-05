//
// Created by qife on 16/1/18.
//

#include <iostream>

#include "JObject.h"

using namespace std;
using namespace JsonCpp;

int main(void)
{
    try
    {
        JObject obj("{\"aaa\":11,\"bbb\":\"test\",\"ccc\":true,\"ddd\":[23,34,45,  12],\"eee\":{ \"er\":null }}");

        // output: {"aaa":11,"bbb":"test","ccc":true,"ddd":[23,34,45,12],"eee":{"er":null}}
        cout << obj.ToString() << endl;

        auto token = obj.SelectToken("$.ddd[2]");
        if (token)
        {
            // output: $.ddd[2]: 45
            cout << "$.ddd[2]: " << token->ToString() << endl;
        }
        else
        {
            cout << "no token found!" << endl;
        }

        auto tokens = obj.SelectTokens("$.ddd[:-1]");
        if (tokens.size() != 0)
        {
            // output: $.ddd[:-1]: 23 34 45
            cout << "$.ddd[:-1]: ";
            for (auto t : tokens)
            {
                cout << t->ToString() << " ";
            }
            cout << endl;
        }
        else
        {
            cout << "no tokens found!" << endl;
        }

        auto filterTokens = obj.SelectTokens("$.ddd[?(@.value < 30)]");
        if (filterTokens.size() != 0)
        {
            // output: $.ddd[?(@.value < 30)]: 23 12
            cout << "$.ddd[?(@.value < 30)]: ";
            for (auto t:filterTokens)
            {
                cout << t->ToString() << " ";
            }
            cout << endl;
        }
        else
        {
            cout << "no tokens found by filter!" << endl;
        }
    }
    catch (JsonException e)
    {
        cout << e.message << endl;
        return -1;
    }

    return 0;
}
