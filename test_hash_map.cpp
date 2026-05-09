#include "cf_hash.hpp"
#include "testing.hpp"

TEST(us_hash_map_basic) {
    cfhash::cf_hash_map<int, int> map;

    CHECK_EQ(map.size(), 0u);
    map[1] = 11;
    CHECK_EQ(map.size(), 1u);

    map.emplace(2, 22);
    CHECK_EQ(map.size(), 2u);

    map.emplace(1, 12);
    CHECK_EQ(map.size(), 1u);

    CHECK(map.contains(1));
    CHECK(map.contains(2));
    CHECK(!map.contains(3));

}
