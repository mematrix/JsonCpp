//
// Created by Charles on 16/1/18.
//

#include <iostream>
#include <fstream>
#include <chrono>

#include "JSONConvert.hpp"
#include "JSONQuery.hpp"

using namespace std;
using namespace json;

struct es_search_result
{
    struct shared
    {
        int64_t failed = 0;
        int64_t skipped = 0;
        int64_t successful = 0;
        int64_t total = 0;
    };

    struct beat_info
    {
        string hostname;
        string version;
        string name;
    };

    struct metricset_info
    {
        string module;
        string name;
        string rtt;
    };

    struct fs_used
    {
        int64_t bytes = 0;
        double pct = 0;
    };

    struct file_system_info
    {
        int free_files = 0;
        string type;
        fs_used used;
        string device_name;
        string mount_point;
        int64_t total = 0;
        int64_t files = 0;
        int64_t free = 0;
        int64_t available = 0;
    };

    struct system_info
    {
        file_system_info filesystem;
    };

    struct source
    {
        beat_info beat;
        metricset_info metricset;
        system_info system;
    };

    struct hits_info
    {
        string _id;
        string _index;
        double _score = 0;
        source _source;
        string _type;
    };

    struct hits_overview
    {
        std::list<hits_info> hits;
        double max_score = 0;
        int64_t total = 0;
    };

    hits_overview hits;
    bool time_out = false;
    int64_t took = 0;
    shared _shared;
};

namespace json {
DESERIALIZE_CLASS(es_search_result::shared, DESERIALIZE(failed), DESERIALIZE(skipped), DESERIALIZE(successful), DESERIALIZE(total));

DESERIALIZE_CLASS(es_search_result::beat_info, DESERIALIZE(hostname), DESERIALIZE(version), DESERIALIZE(name));

DESERIALIZE_CLASS(es_search_result::metricset_info, DESERIALIZE(module), DESERIALIZE(name), DESERIALIZE(rtt));

DESERIALIZE_CLASS(es_search_result::fs_used, DESERIALIZE(bytes), DESERIALIZE(pct));

DESERIALIZE_CLASS(es_search_result::file_system_info, DESERIALIZE(free_files), DESERIALIZE(type), DESERIALIZE(used), DESERIALIZE(device_name),
                  DESERIALIZE(mount_point), DESERIALIZE(total), DESERIALIZE(files), DESERIALIZE(free), DESERIALIZE(available));

DESERIALIZE_CLASS(es_search_result::system_info, DESERIALIZE(filesystem));

DESERIALIZE_CLASS(es_search_result::source, DESERIALIZE(beat), DESERIALIZE(metricset), DESERIALIZE(system));

DESERIALIZE_CLASS(es_search_result::hits_info, DESERIALIZE(_id), DESERIALIZE(_index), DESERIALIZE(_score), DESERIALIZE(_source), DESERIALIZE(_type));

DESERIALIZE_CLASS(es_search_result::hits_overview, DESERIALIZE(hits), DESERIALIZE(max_score), DESERIALIZE(total));

DESERIALIZE_CLASS(es_search_result, DESERIALIZE(hits), DESERIALIZE(time_out), DESERIALIZE(took), DESERIALIZE(_shared));
}

class count_timer
{
    using tp = chrono::system_clock::time_point;
    tp start_time;
    tp end_time;

public:
    count_timer() = default;

    void start()
    {
        start_time = chrono::system_clock::now();
    }

    void stop()
    {
        end_time = chrono::system_clock::now();
    }

    void print(const std::string &str)
    {
        auto time_ns = (end_time - start_time).count();
        cout << "(" << str << ") time: " << time_ns << "ns = " << time_ns / 1000.0 << "us = " << time_ns / (1000.0 * 1000) << "ms" << endl;
    }
};


int main(int argc, char **argv)
{
    if (argc != 2) {
        cerr << "usage: " << argv[0] << " json_file_path" << endl;
        return -1;
    }

    count_timer ct;
    ifstream test_file(argv[1]);
    if (!test_file) {
        cerr << "can not open file: " << argv[1] << endl;
        return -1;
    }
    std::string content((std::istreambuf_iterator<char>(test_file)), std::istreambuf_iterator<char>());
    int error_code = 0;

    ct.start();
    auto token = parse(content, &error_code);
    ct.stop();
    ct.print("parse");

    if (token) {
        cout << std::endl;
        ct.start();
        auto non_format_json = to_string(*token);
        ct.stop();
        ct.print("to_string");
        // cout << "non-formatted json: " << non_format_json << std::endl;

        cout << std::endl;
        ct.start();
        auto format_json = to_string(*token, json_format_option::indent_tab);
        ct.stop();
        ct.print("to_string format");
        // cout << "formatted json: " << std::endl << format_json << std::endl;

        cout << std::endl;
        es_search_result result;
        ct.start();
        deserialize(result, *token);
        ct.stop();
        ct.print("deserialize");
        cout << "took: " << result.took << "; hits item count: " << result.hits.hits.size() << endl;

        cout << std::endl;
        const char *dot_json_path = "$.hits.hits[*]._source.system.filesystem.used.bytes";
        ct.start();
        auto query_result = select_tokens(*token, dot_json_path);
        ct.stop();
        ct.print("dot_json_path");
        cout << "query result count: " << query_result.size() << endl;

        cout << std::endl;
        const char *bracket_json_path = "$['hits']['hits'][*]['_source']['system']['filesystem']['used']['bytes']";
        ct.start();
        auto b_query_result = select_tokens(*token, bracket_json_path);
        ct.stop();
        ct.print("bracket_json_path");
        cout << "query result count: " << b_query_result.size() << endl;
    } else {
        cerr << "parse failed. error code: " << error_code << ", msg: " << get_error_info(error_code) << std::endl;
    }

    // cin.get();

    return 0;
}
