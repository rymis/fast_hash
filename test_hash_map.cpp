#include "cf_hash.hpp"
#include "testing.hpp"
#include <random>
#include <unordered_map>

TEST(us_hash_map_basic) {
    cfhash::cf_hash_map<int, int> map;

    CHECK_EQ(map.size(), 0u);
    map[1] = 11;
    CHECK_EQ(map.size(), 1u);

    map.emplace(2, 22);
    CHECK_EQ(map.size(), 2u);

    map.emplace(1, 12);
    CHECK_EQ(map.size(), 2u);

    CHECK(map.contains(1));
    CHECK(map.contains(2));
    CHECK(!map.contains(3));
}

TEST(us_hash_map_rehash) {
    cfhash::cf_hash_map<int, int> map{17};

    CHECK_EQ(map.size(), 0u);

    for (int i = 0; i < 100; ++i) {
        map.emplace(i, 1000 + i);
    }

    CHECK_EQ(map.size(), 100u);

    for (int i = 0; i < 100; ++i) {
        CHECK(map.contains(i));
        CHECK_EQ(map.at(i), i + 1000);
    }

    for (int i = 100; i < 200; ++i) {
        CHECK(!map.contains(i));
    }
}

TEST(us_hash_map_more_values) {
    std::mt19937 rnd{65537};

    std::unordered_map<std::uint32_t, std::uint32_t> etalon;
    cfhash::cf_hash_map<std::uint32_t, std::uint32_t> map;

    for (int i = 0; i < 1000000; ++i) {
        etalon.emplace(rnd() & 0xffffff, rnd() % 100000);
    }

    for (auto v : etalon) {
        map.emplace(v.first, v.second);
    }

    for (auto v : etalon) {
        CHECK(map.contains(v.first));
        CHECK_EQ(map.at(v.first), v.second);
    }

    for (int i = 0; i < 1000000; ++i) {
        std::uint32_t v = rnd() & 0xffffff;
        if (etalon.count(v)) {
            CHECK(map.contains(v));
            CHECK_EQ(map.at(v), etalon.at(v));
        } else {
            CHECK(!map.contains(v));
        }
    }
}
