#pragma once

#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
#include <type_traits>


class TestManager {
    std::string name_;
    static std::map<std::string, void(*)()>& tests() {
        static std::map<std::string, void (*)()> res;
        return res;
    }
public:
    static void add(const std::string& name, void (*f)()) {
        auto& t = tests();
        if (t.count(name)) {
            std::cerr << "ERROR: trying to register test with the same name: " << name << std::endl;
            std::exit(1);
        }

        t.emplace(name, f);
    }

    static void run_all() {
        for (const auto& item : tests()) {
            std::cout << "[TEST: " << item.first << "]" << std::endl;
            try {
                item.second();
                std::cout << "[SUCCESS: " << item.first << "]" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[FAILED: " << item.first << "] " << e.what() << std::endl;
            }
        }
    }

    static void successful_check() {
    }

    static void main(int argc, const char* argv[]) {
        run_all();
    }
};

class TestRegisterHelper {
public:
    TestRegisterHelper(const std::string& name, void (*f)()) {
        TestManager::add(name, f);
    }
};

template <typename T>
struct TestCanPrint {
    template <typename T2>
    static std::true_type check(T2*, std::remove_reference_t<decltype(std::cout << std::declval<T2>())> * = nullptr);
    static std::false_type check(...);

    static constexpr bool value = decltype(check((T*)nullptr))::value;
};

template <typename T, bool can = TestCanPrint<T>::value>
struct TestPrinter;

template <typename T>
struct TestPrinter<T, true> {
    std::string operator()(const T& val) {
        std::ostringstream out;
        out << val;
        return out.str();
    }
};

template <typename T>
struct TestPrinter<T, false> {
    std::string operator()(const T& val) {
        return "[CAN'T PRINT]";
    }
};

template <typename T>
inline std::string ToString(const T& val) {
    TestPrinter<T> printer;
    return printer(val);
}

#define TEST(name) \
    void simple_test__##name(); \
    static TestRegisterHelper simple_test__##name##__register{#name, &simple_test__##name}; \
    void simple_test__##name()

#define CHECK(some) \
    do { \
        if (!(some)) { \
            throw std::runtime_error(__FILE__ ":" + std::to_string(__LINE__) + "] " + std::string("Assertion failed: ") + #some); \
        } else { \
            TestManager::successful_check(); \
        } \
    } while (0)

#define CHECK_EQ(left_val, right_val) \
    do { \
        auto lval__ = (left_val); \
        auto rval__ = (right_val); \
        if (lval__ != rval__) { \
            throw std::runtime_error(__FILE__ ":" + std::to_string(__LINE__) + "] " + std::string("Not equal: ") + ToString(lval__) + " != " + ToString(rval__)); \
        } else { \
            TestManager::successful_check(); \
        } \
    } while (0)
