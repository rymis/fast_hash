#include "cf_hash.hpp"
#include "testing.hpp"

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
