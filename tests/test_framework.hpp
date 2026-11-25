#pragma once
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tf {

struct TestCase { std::string name; std::function<void()> fn; };

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

inline int run() {
    int passed = 0, failed = 0;
    for (auto& t : registry()) {
        try {
            t.fn();
            std::cout << "[PASS] " << t.name << "\n";
            ++passed;
        } catch (const std::exception& e) {
            std::cout << "[FAIL] " << t.name << " : " << e.what() << "\n";
            ++failed;
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}

} // namespace tf

#define TEST(name) \
    static void name(); \
    static ::tf::Registrar reg_##name(#name, name); \
    static void name()

#define ASSERT_TRUE(x)  do { if (!(x))  throw std::runtime_error("ASSERT_TRUE failed: "  #x); } while(0)
#define ASSERT_FALSE(x) do { if ( (x))  throw std::runtime_error("ASSERT_FALSE failed: " #x); } while(0)
#define ASSERT_EQ(a,b)  do { if ((a) != (b)) throw std::runtime_error(std::string("ASSERT_EQ failed: ") + #a + " != " + #b); } while(0)
#define ASSERT_NEAR(a,b,eps) do { if (std::fabs((a)-(b)) > (eps)) throw std::runtime_error(std::string("ASSERT_NEAR failed: ") + #a + " vs " + #b); } while(0)