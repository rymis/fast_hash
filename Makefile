CXX ?= g++

all: tst tst_dbg

tst: test.cpp test_hash_set.cpp test_hash_map.cpp cf_hash.hpp
	$(CXX) -Wall -std=c++17 -O3 test.cpp test_hash_set.cpp test_hash_map.cpp -o tst

tst_dbg: test.cpp test_hash_set.cpp test_hash_map.cpp cf_hash.hpp
	$(CXX) -Wall -std=c++17 -g -O0 test.cpp test_hash_set.cpp test_hash_map.cpp -o tst_dbg

clean:
	rm -f tst tst_dbg

.PHONY: clean all
