CXX ?= g++

tst: test.cpp test_hash_set.cpp test_hash_map.cpp cf_hash.hpp
	$(CXX) -Wall -std=c++17 -O3 test.cpp test_hash_set.cpp test_hash_map.cpp -o tst

clean:
	rm -f tst

.PHONY: clean
