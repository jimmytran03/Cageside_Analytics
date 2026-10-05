#include "TestFramework.hpp"

// Usage: ./ufc_tests [name-filter]
int main(int argc, char** argv) {
    const std::string filter = argc > 1 ? argv[1] : "";
    int ran = 0;
    int failed = 0;

    for (const auto& test : testing::registry()) {
        if (!filter.empty() && test.name.find(filter) == std::string::npos) continue;

        testing::failures().clear();
        try {
            test.fn();
        } catch (const std::exception& e) {
            testing::fail("(uncaught)", 0, e.what());
        } catch (...) {
            testing::fail("(uncaught)", 0, "unknown exception");
        }

        ++ran;
        if (testing::failures().empty()) {
            std::cout << "[ PASS ] " << test.name << '\n';
        } else {
            ++failed;
            std::cout << "[ FAIL ] " << test.name << '\n';
            for (const auto& message : testing::failures()) std::cout << "         " << message << '\n';
        }
    }

    std::cout << "\n" << (ran - failed) << " / " << ran << " tests passed\n";
    return failed == 0 ? 0 : 1;
}
