//
// Created by qife on 16/1/18.
//

#include <iostream>

#include "JSONConvert.hpp"

using namespace std;
using namespace json;


struct test
{
    struct inner {
        std::unique_ptr<std::string> er;
        std::shared_ptr<std::string> msg;
    };

    int aaa;
    std::string bbb;
    bool ccc;
    int ddd[6];
    inner eee;
};

#define DESERIALIZE_A DESERIALIZE(aaa)
#define DESERIALIZE_B DESERIALIZE(bbb)

namespace json {
DESERIALIZE_CLASS(test::inner, DESERIALIZE(er), DESERIALIZE(msg));
DESERIALIZE_CLASS(test, DESERIALIZE_A, DESERIALIZE_B, DESERIALIZE(ccc), DESERIALIZE(ddd), DESERIALIZE(eee));
}


int main()
{
    const char *json_str = "{\"aaa\":11,\"bbb\":\"test\",\"ccc\":true,\"ddd\":[23,34,45,  12],\"eee\":{ \"er\":null , \"msg\": \"error message. eee\"}}";
    int error_code = 0;
    auto token = Parse(json_str, &error_code);
    if (token) {
        cout << "parse success." << std::endl;
        cout << "non-formatted json: " << ToString(*token) << std::endl;
        cout << "formatted json: " << std::endl;
        cout << ToString(*token, JsonFormatOption::IndentTab) << std::endl;
        cout << std::endl;
        cout << "deserialize..." <<std::endl;
        test t;
        deserialize(t, *token);
        cout << "aaa: " << t.aaa << ", bbb: " << t.bbb << ", ccc: " << boolalpha << t.ccc << ", ddd: [";
        for (int i = 0; i < 5; ++i) {
            cout << t.ddd[i] << ", ";
        }
        cout << t.ddd[5] << "]";
        cout << ", eee: {er: " << (t.eee.er == nullptr ? "null" : *(t.eee.er)) << ", msg: " << (t.eee.msg == nullptr ? "null" : *(t.eee.msg)) << "}" << std::endl;
    } else {
        cerr << "parse failed. error code: " << error_code << ", msg: " << GetErrorInfo(error_code) << std::endl;
    }

    /* try {
        JObject obj();

        // output: {"aaa":11,"bbb":"test","ccc":true,"ddd":[23,34,45,12],"eee":{"er":null}}
        cout << obj.ToString() << endl;

        auto token = obj.SelectToken("$.ddd[2]");
        if (token) {
            // output: $.ddd[2]: 45
            cout << "$.ddd[2]: " << token->ToString() << endl;
        } else {
            cout << "no token found!" << endl;
        }

        auto tokens = obj.SelectTokens("$.ddd[:-1]");
        if (tokens.size() != 0) {
            // output: $.ddd[:-1]: 23 34 45
            cout << "$.ddd[:-1]: ";
            for (auto t : tokens) {
                cout << t->ToString() << " ";
            }
            cout << endl;
        } else {
            cout << "no tokens found!" << endl;
        }

        auto filterTokens = obj.SelectTokens("$.ddd[?(@.value < 30)]");
        if (filterTokens.size() != 0) {
            // output: $.ddd[?(@.value < 30)]: 23 12
            cout << "$.ddd[?(@.value < 30)]: ";
            for (auto t:filterTokens) {
                cout << t->ToString() << " ";
            }
            cout << endl;
        } else {
            cout << "no tokens found by filter!" << endl;
        }
    }
    catch (JsonException e) {
        cout << e.message << endl;
        return -1;
    } */

    return 0;
}
