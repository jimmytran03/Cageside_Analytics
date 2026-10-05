#pragma once
// Minimal header-only test framework: no installs needed, just g++.
// (Straightforward to migrate to GoogleTest/Catch2 later: TEST/CHECK map 1:1.)
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace testing {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline std::vector<std::string>& failures() {
    static std::vector<std::string> messages;
    return messages;
}

inline void fail(const char* file, int line, const std::string& message) {
    std::ostringstream os;
    os << file << ":" << line << ": " << message;
    failures().push_back(os.str());
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

} // namespace testing

#define TEST(name)                                                        \
    static void test_##name();                                            \
    static ::testing::Registrar registrar_##name(#name, test_##name);     \
    static void test_##name()

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) ::testing::fail(__FILE__, __LINE__, "CHECK failed: " #cond); \
    } while (0)

#define CHECK_EQ(a, b)                                                    \
    do {                                                                  \
        const auto va_ = (a);                                            \
        const auto vb_ = (b);                                            \
        if (!(va_ == vb_)) {                                              \
            std::ostringstream os_;                                       \
            os_ << "CHECK_EQ failed: " #a " == " #b " (got " << va_ << " vs " << vb_ << ")"; \
            ::testing::fail(__FILE__, __LINE__, os_.str());               \
        }                                                                 \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                             \
    do {                                                                  \
        const double va_ = (a);                                           \
        const double vb_ = (b);                                           \
        if (!(std::fabs(va_ - vb_) <= (eps))) {                           \
            std::ostringstream os_;                                       \
            os_ << "CHECK_NEAR failed: " #a " ~ " #b " (got " << va_ << " vs " << vb_ << ")"; \
            ::testing::fail(__FILE__, __LINE__, os_.str());               \
        }                                                                 \
    } while (0)

#define CHECK_THROWS(expr)                                                \
    do {                                                                  \
        bool thrown_ = false;                                             \
        try { (void)(expr); } catch (const std::exception&) { thrown_ = true; } \
        if (!thrown_) ::testing::fail(__FILE__, __LINE__, "expected exception: " #expr); \
    } while (0)
