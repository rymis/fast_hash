#include "cf_hash.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <random>

#include "testing.hpp"

double ftime() {
    return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

struct LogTime {
    std::string message;
    double start;
    LogTime(std::string msg)
        : message(msg)
        , start(ftime())
    {
    }

    ~LogTime() {
        double end = ftime();
        std::cout << "[" << message << "] in " << (end - start) << "s" << std::endl;
    }
};

std::mt19937 rng(std::random_device{}());

std::string rand_string(size_t len) {
    std::string s;
    s.resize(len);
    for (size_t i = 0; i < len; ++i) {
        s[i] = 'a' + rng() % 26;
    }
    return s;
}

template <typename Cont>
void test_set(const std::string& name, size_t cnt) {
    std::vector<std::string> keys;
    std::vector<std::string> not_keys;
    for (size_t i = 0; i < cnt; ++i) {
        keys.push_back(rand_string(10));
        not_keys.push_back(rand_string(10));
    }

    {
        LogTime logger("test-" + name + "-" + std::to_string(cnt));

        Cont s;
        for (auto& k : keys) {
            s.insert(k);
        }

        for (size_t i = 0; i < 10; ++i) {
            for (auto& k : keys) {
                s.find(k);
            }

            for (auto& k : not_keys) {
                s.find(k);
            }
        }
    }

}


int main(int argc, const char* argv[]) {
    /*
    test_set<std::unordered_set<std::string>>("unordered_set", 1000000);
    test_set<cfhash::cf_hash_set<std::string>>("cf_hash_set", 1000000);
    */

    TestManager::main(argc, argv);
}
