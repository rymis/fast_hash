#include "cf_hash.hpp"
#include "testing.hpp"
#include <random>
#include <unordered_set>

TEST(us_hash_set_basic) {
    cfhash::cf_hash_set<int> set;

    CHECK_EQ(set.size(), 0u);

    set.insert(1);
    CHECK_EQ(set.size(), 1u);

    CHECK_EQ(set.count(1), 1u);
    CHECK_EQ(set.count(2), 0u);
    CHECK(!set.contains(2));
    CHECK(set.contains(1));

    set.erase(1);

    CHECK_EQ(set.size(), 0u);
    CHECK(!set.contains(1));
}

TEST(us_hash_set_rehash) {
    cfhash::cf_hash_set<int> set{16};

    CHECK_EQ(set.size(), 0u);

    for (int i = 0; i < 20; ++i) {
        set.insert(i);
    }

    CHECK_EQ(set.size(), 20u);

    for (int i = 0; i < 20; ++i) {
        CHECK(set.contains(i));
    }

    for (int i = 20; i < 40; ++i) {
        CHECK(!set.contains(i));
    }
}

TEST(us_hash_set_more_values) {
    std::mt19937 rnd{65537};

    std::unordered_set<std::uint32_t> etalon;
    cfhash::cf_hash_set<std::uint32_t> set;

    for (int i = 0; i < 1000000; ++i) {
        etalon.insert(rnd() & 0xffffff);
    }

    for (auto v : etalon) {
        set.insert(v);
    }

    for (auto v : etalon) {
        CHECK(set.contains(v));
    }

    for (int i = 0; i < 1000000; ++i) {
        std::uint32_t v = rnd() & 0xffffff;
        if (etalon.count(v)) {
            CHECK(set.contains(v));
        } else {
            CHECK(!set.contains(v));
        }
    }

    for (int i = 0; i < 10000; ++i) {
        std::uint32_t v = rnd() & 0xffffff;
        set.erase(v);
        etalon.erase(v);
    }

    for (int i = 0; i < 1000000; ++i) {
        std::uint32_t v = rnd() & 0xffffff;
        if (etalon.count(v)) {
            CHECK(set.contains(v));
        } else {
            CHECK(!set.contains(v));
        }
    }
}

TEST(us_hash_set_iterator) {
    cfhash::cf_hash_set<std::uint32_t> set;

    set.insert(1);
    set.insert(2);
    set.insert(3);
    set.insert(2);
    set.insert(1);

    auto it = set.begin();
    CHECK(it != set.end());
    CHECK(it == set.begin());
    CHECK(*it < 4 && *it > 0);

    ++it;
    CHECK(it != set.end());
    CHECK(it != set.begin());
    CHECK(*it < 4 && *it > 0);

    ++it;
    CHECK(it != set.end());
    CHECK(it != set.begin());
    CHECK(*it < 4 && *it > 0);

    ++it;
    CHECK(it == set.end());
    CHECK(it != set.begin());
}
