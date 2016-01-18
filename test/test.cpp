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
    }
    catch (JsonException e)
    {
        cout << e.message << endl;
        return -1;
    }

    return 0;
}
