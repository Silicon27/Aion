//
// Created by David Yang on 2025-11-29.
//

//
// AION Test Framework - A modern C++20 testing library
// File: aion_test.hpp
//

#ifndef AION_TEST_HPP
#define AION_TEST_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <iomanip>
#include <stdexcept>
#include <optional>
#include <any>
#include <map>
#include <algorithm>
#include <memory>

#include <regex>

namespace aion::test {

// ============================================================================
// ANSI Color Codes
// ============================================================================
namespace color {
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* DIM = "\033[2m";
    constexpr const char* UNDERLINE = "\033[4m";
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* WHITE = "\033[37m";
    constexpr const char* GRAY = "\033[90m";
    constexpr const char* BG_RED = "\033[41m";
    constexpr const char* BG_GREEN = "\033[42m";
}

// ============================================================================
// Test Configuration & Logging
// ============================================================================
enum class LogLevel {
    NONE = 0,
    ERROR = 1,
    WARN = 2,
    SUCCESS = 3,
    INFO = 4,
    DEBUG = 5,
    TRACE = 6
};

struct TestOptions {
    bool verbose = false;
    bool stop_on_fail = false;
    bool use_color = true;
    LogLevel log_level = LogLevel::INFO;
    std::string suite_filter = "";
    std::string test_filter = "";
    std::ostream* output_stream = &std::cout;
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_options(const TestOptions& opts) { options = &opts; }

    void log(LogLevel level, const std::string& message, int indent_level = 0) {
        if (!options || level > options->log_level) return;

        std::ostream& os = *(options->output_stream);
        
        std::string indent;
        for (int i = 0; i < indent_level; ++i) {
            indent += "  ";
        }

        if (options->use_color) {
            os << get_level_color(level);
        }

        if (indent_level > 0) {
            os << indent << "└─ ";
        }

        os << message;

        if (options->use_color) {
            os << color::RESET;
        }
        os << "\n";
    }

    void suite(const std::string& name) {
        log(LogLevel::INFO, "Suite: " + name, 0);
    }

    void test(const std::string& name, bool passed, double duration_ms, int indent = 1) {
        std::stringstream ss;
        ss << (passed ? "✓ " : "✗ ") << name << " (" << std::fixed << std::setprecision(2) << duration_ms << "ms)";
        log(passed ? LogLevel::SUCCESS : LogLevel::ERROR, ss.str(), indent);
    }

private:
    const TestOptions* options = nullptr;

    const char* get_level_color(LogLevel level) {
        switch (level) {
            case LogLevel::ERROR:   return color::RED;
            case LogLevel::WARN:    return color::YELLOW;
            case LogLevel::INFO:    return color::CYAN;
            case LogLevel::SUCCESS: return color::GREEN;
            case LogLevel::DEBUG:   return color::GRAY;
            case LogLevel::TRACE:   return color::DIM;
            default:                return color::RESET;
        }
    }
};

#define AION_LOG(level, msg) aion::test::Logger::instance().log(aion::test::LogLevel::level, msg, 2)
#define AION_INFO(msg) AION_LOG(INFO, msg)
#define AION_DEBUG(msg) AION_LOG(DEBUG, msg)

// ============================================================================
// Test Result
// ============================================================================
struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    std::string file;
    int line;
    double duration_ms;
    std::optional<std::string> expected;
    std::optional<std::string> actual;
};

// ============================================================================
// Assertion Exception
// ============================================================================
class AssertionFailure : public std::exception {
public:
    AssertionFailure(std::string msg, std::string file, int line)
        : message(std::move(msg)), filename(std::move(file)), line_number(line) {}

    const char* what() const noexcept override { return message.c_str(); }
    const std::string& file() const { return filename; }
    int line() const { return line_number; }

private:
    std::string message;
    std::string filename;
    int line_number;
};

// ============================================================================
// Test Suite
// ============================================================================
class TestSuite {
public:
    explicit TestSuite(std::string name) : suite_name(std::move(name)) {}

    void add_test(std::string name, std::function<void()> test_fn) {
        tests.push_back({std::move(name), std::move(test_fn)});
    }

    void setup(std::function<void()> fn) { setup_fn = std::move(fn); }
    void teardown(std::function<void()> fn) { teardown_fn = std::move(fn); }
    void before_each(std::function<void()> fn) { before_each_fn = std::move(fn); }
    void after_each(std::function<void()> fn) { after_each_fn = std::move(fn); }

    [[nodiscard]] const std::string& get_name() const { return suite_name; }

    std::vector<TestResult> run(const TestOptions& opts = {}) {
        std::vector<TestResult> results;
        Logger::instance().suite(suite_name);

        if (setup_fn) {
            try {
                setup_fn();
            } catch (const std::exception& e) {
                Logger::instance().log(LogLevel::ERROR, "Setup failed: " + std::string(e.what()), 1);
                return results;
            }
        }

        for (size_t i = 0; i < tests.size(); ++i) {
            const auto& [name, test_fn] = tests[i];

            // Filter
            if (!opts.test_filter.empty() && name.find(opts.test_filter) == std::string::npos) {
                continue;
            }

            if (before_each_fn) before_each_fn();

            auto start = std::chrono::high_resolution_clock::now();
            TestResult result{name, true, "", "", 0, 0.0};

            try {
                test_fn();
                result.passed = true;
                result.message = "Test passed";
            } catch (const AssertionFailure& e) {
                result.passed = false;
                result.message = e.what();
                result.file = e.file();
                result.line = e.line();
            } catch (const std::exception& e) {
                result.passed = false;
                result.message = std::string("Unexpected exception: ") + e.what();
            } catch (...) {
                result.passed = false;
                result.message = "Unknown exception thrown";
            }

            auto end = std::chrono::high_resolution_clock::now();
            result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

            if (after_each_fn) after_each_fn();

            results.push_back(result);
            Logger::instance().test(name, result.passed, result.duration_ms, 1);

            if (!result.passed) {
                print_failure_details(result);
                if (opts.stop_on_fail) break;
            }
        }

        if (teardown_fn) teardown_fn();

        return results;
    }

private:
    std::string suite_name;
    std::vector<std::pair<std::string, std::function<void()>>> tests;
    std::function<void()> setup_fn;
    std::function<void()> teardown_fn;
    std::function<void()> before_each_fn;
    std::function<void()> after_each_fn;

    void print_failure_details(const TestResult& result) {
        Logger::instance().log(LogLevel::ERROR, result.message, 2);
        if (!result.file.empty()) {
            std::stringstream ss;
            ss << "at " << result.file << ":" << result.line;
            Logger::instance().log(LogLevel::TRACE, ss.str(), 2);
        }
        if (result.expected.has_value()) {
            Logger::instance().log(LogLevel::WARN, "Expected: " + result.expected.value(), 2);
        }
        if (result.actual.has_value()) {
            Logger::instance().log(LogLevel::WARN, "Actual:   " + result.actual.value(), 2);
        }
    }
};

// ============================================================================
// Test Runner
// ============================================================================
class TestRunner {
public:
    static TestRunner& instance() {
        static TestRunner runner;
        return runner;
    }

    void add_suite(std::shared_ptr<TestSuite> suite) {
        suites.push_back(suite);
    }

    void add_suite(std::unique_ptr<TestSuite> suite) {
        suites.push_back(std::move(suite));
    }

    void list_tests() {
        std::cout << "\n" << color::CYAN << color::BOLD
                  << "Available Test Suites:" << color::RESET << "\n";
        for (const auto& suite : suites) {
            std::cout << "  - " << suite->get_name() << "\n";
        }
        std::cout << "\n";
    }

    int run_all(const TestOptions& opts = {}) {
        Logger::instance().set_options(opts);

        if (opts.use_color) std::cout << color::YELLOW << color::BOLD;
        std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n"
                  << "║                         AION TEST FRAMEWORK                               ║\n"
                  << "╚═══════════════════════════════════════════════════════════════════════════╝";
        if (opts.use_color) std::cout << color::RESET;
        std::cout << "\n\n";

        int total_passed = 0;
        int total_failed = 0;
        double total_time = 0.0;

        for (auto& suite : suites) {
            // Suite Filter
            if (!opts.suite_filter.empty() && suite->get_name().find(opts.suite_filter) == std::string::npos) {
                continue;
            }

            auto results = suite->run(opts);

            for (const auto& result : results) {
                if (result.passed) {
                    total_passed++;
                } else {
                    total_failed++;
                    if (opts.stop_on_fail) {
                        print_summary(total_passed, total_failed, total_time, opts);
                        return 1;
                    }
                }
                total_time += result.duration_ms;
            }
        }

        print_summary(total_passed, total_failed, total_time, opts);
        return total_failed > 0 ? 1 : 0;
    }

    // Overload for backward compatibility
    int run_all(bool verbose) {
        TestOptions opts;
        opts.verbose = verbose;
        opts.log_level = verbose ? LogLevel::DEBUG : LogLevel::INFO;
        return run_all(opts);
    }

private:
    std::vector<std::shared_ptr<TestSuite>> suites;

    void print_summary(int passed, int failed, double total_time, const TestOptions& opts) {
        const int box_width = 75;
        auto horizontal_line = []() {
            for (int i = 0; i < 75; ++i) std::cout << "═";
        };
        
        std::cout << "\n";
        if (opts.use_color) std::cout << color::BOLD;
        std::cout << "╔"; horizontal_line(); std::cout << "╗\n";

        // Results line
        std::stringstream ss_res;
        ss_res << "RESULTS: " << passed << " PASSED, " << failed << " FAILED (Total: " << (passed + failed) << ")";
        std::string res_str = ss_res.str();
        
        std::cout << "║ " << res_str << std::string(box_width - res_str.length() - 1, ' ') << "║\n";

        // Time line
        std::stringstream ss_time;
        ss_time << "Time: " << std::fixed << std::setprecision(2) << total_time << "ms";
        std::string time_str = ss_time.str();
        std::cout << "║ " << time_str << std::string(box_width - time_str.length() - 1, ' ') << "║\n";

        std::cout << "╚"; horizontal_line(); std::cout << "╝\n";
        if (opts.use_color) std::cout << color::RESET;
        std::cout << "\n";
    }
};

// ============================================================================
// Assertion Macros
// ============================================================================
#define AION_TEST_SUITE(name) \
    auto suite_##name = std::make_shared<aion::test::TestSuite>(#name); \
    struct SuiteRegistrar_##name { \
        SuiteRegistrar_##name() { \
            aion::test::TestRunner::instance().add_suite(suite_##name); \
        } \
    }; \
    static SuiteRegistrar_##name registrar_##name;

#define AION_TEST(suite_name, test_name) \
    void test_##suite_name##_##test_name(); \
    struct TestRegistrar_##suite_name##_##test_name { \
        TestRegistrar_##suite_name##_##test_name() { \
            suite_##suite_name->add_test(#test_name, test_##suite_name##_##test_name); \
        } \
    }; \
    static TestRegistrar_##suite_name##_##test_name registrar_##suite_name##_##test_name; \
    void test_##suite_name##_##test_name()

#define AION_SETUP(suite_name) \
    void setup_##suite_name(); \
    struct SetupRegistrar_##suite_name { \
        SetupRegistrar_##suite_name() { \
            suite_##suite_name->setup(setup_##suite_name); \
        } \
    }; \
    static SetupRegistrar_##suite_name setup_registrar_##suite_name; \
    void setup_##suite_name()

#define AION_TEARDOWN(suite_name) \
    void teardown_##suite_name(); \
    struct TeardownRegistrar_##suite_name { \
        TeardownRegistrar_##suite_name() { \
            suite_##suite_name->teardown(teardown_##suite_name); \
        } \
    }; \
    static TeardownRegistrar_##suite_name teardown_registrar_##suite_name; \
    void teardown_##suite_name()

#define AION_BEFORE_EACH(suite_name) \
    void before_each_##suite_name(); \
    struct BeforeEachRegistrar_##suite_name { \
        BeforeEachRegistrar_##suite_name() { \
            suite_##suite_name->before_each(before_each_##suite_name); \
        } \
    }; \
    static BeforeEachRegistrar_##suite_name before_each_registrar_##suite_name; \
    void before_each_##suite_name()

#define AION_AFTER_EACH(suite_name) \
    void after_each_##suite_name(); \
    struct AfterEachRegistrar_##suite_name { \
        AfterEachRegistrar_##suite_name() { \
            suite_##suite_name->after_each(after_each_##suite_name); \
        } \
    }; \
    static AfterEachRegistrar_##suite_name after_each_registrar_##suite_name; \
    void after_each_##suite_name()

// Basic Assertions
#define AION_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            throw aion::test::AssertionFailure( \
                "Assertion failed: " #condition, __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_TRUE(condition) AION_ASSERT(condition)
#define AION_ASSERT_FALSE(condition) AION_ASSERT(!(condition))

// For comparing enum types or other non-streamable types
#define AION_ASSERT_ENUM_EQ(actual, expected) \
    do { \
        auto actual_val = (actual); \
        auto expected_val = (expected); \
        if (actual_val != expected_val) { \
            std::ostringstream oss; \
            oss << "Enum mismatch:\n  Actual:   " << static_cast<int>(actual_val) \
                << "\n  Expected: " << static_cast<int>(expected_val); \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_EQ(actual, expected) \
    do { \
        if (!((actual) == (expected))) { \
            std::ostringstream oss; \
            oss << "Expected equality:\n  Actual:   " << (actual) \
                << "\n  Expected: " << (expected); \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_NE(actual, expected) \
    do { \
        if ((actual) == (expected)) { \
            std::ostringstream oss; \
            oss << "Expected inequality:\n  Value: " << (actual); \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_LT(a, b) \
    do { \
        if (!((a) < (b))) { \
            std::ostringstream oss; \
            oss << "Expected " << (a) << " < " << (b); \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_LE(a, b) \
    do { \
        if (!((a) <= (b))) { \
            std::ostringstream oss; \
            oss << "Expected " << (a) << " <= " << (b); \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_GT(a, b) \
    do { \
        if (!((a) > (b))) { \
            std::ostringstream oss; \
            oss << "Expected " << (a) << " > " << (b); \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_GE(a, b) \
    do { \
        if (!((a) >= (b))) { \
            std::ostringstream oss; \
            oss << "Expected " << (a) << " >= " << (b); \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_NEAR(actual, expected, epsilon) \
    do { \
        auto diff = std::abs((actual) - (expected)); \
        if (diff > (epsilon)) { \
            std::ostringstream oss; \
            oss << "Values not near:\n  Actual:   " << (actual) \
                << "\n  Expected: " << (expected) \
                << "\n  Epsilon:  " << (epsilon) \
                << "\n  Diff:     " << diff; \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != nullptr) { \
            throw aion::test::AssertionFailure( \
                "Expected null pointer", __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            throw aion::test::AssertionFailure( \
                "Expected non-null pointer", __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_THROWS(statement, exception_type) \
    do { \
        bool caught = false; \
        try { \
            statement; \
        } catch (const exception_type&) { \
            caught = true; \
        } catch (...) {} \
        if (!caught) { \
            throw aion::test::AssertionFailure( \
                "Expected exception of type " #exception_type " to be thrown", \
                __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_NO_THROW(statement) \
    do { \
        try { \
            statement; \
        } catch (...) { \
            throw aion::test::AssertionFailure( \
                "Expected no exception to be thrown", __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_IN_RANGE(val, min, max) \
    do { \
        auto v = (val); \
        auto low = (min); \
        auto high = (max); \
        if (v < low || v > high) { \
            std::ostringstream oss; \
            oss << "Value " << v << " out of range [" << low << ", " << high << "]"; \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_MATCH(str, pattern) \
    do { \
        std::string s = (str); \
        std::regex re(pattern); \
        if (!std::regex_search(s, re)) { \
            std::ostringstream oss; \
            oss << "String \"" << s << "\" does not match pattern \"" << pattern << "\""; \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_FAIL(message) \
    throw aion::test::AssertionFailure(message, __FILE__, __LINE__)

// String Assertions
#define AION_ASSERT_STREQ(actual, expected) \
    do { \
        std::string a_str = (actual); \
        std::string e_str = (expected); \
        if (a_str != e_str) { \
            std::ostringstream oss; \
            oss << "String mismatch:\n  Actual:   \"" << a_str \
                << "\"\n  Expected: \"" << e_str << "\""; \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_STRNE(actual, expected) \
    do { \
        std::string a_str = (actual); \
        std::string e_str = (expected); \
        if (a_str == e_str) { \
            std::ostringstream oss; \
            oss << "Strings should not be equal: \"" << a_str << "\""; \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_CONTAINS(string_val, substring_val) \
    do { \
        std::string AION_STR_TEMP = (string_val); \
        std::string AION_SUBSTR_TEMP = (substring_val); \
        if (AION_STR_TEMP.find(AION_SUBSTR_TEMP) == std::string::npos) { \
            std::ostringstream AION_OSS_TEMP; \
            AION_OSS_TEMP << "String \"" << AION_STR_TEMP << "\" does not contain \"" << AION_SUBSTR_TEMP << "\""; \
            std::string AION_MSG_TEMP = AION_OSS_TEMP.str(); \
            throw aion::test::AssertionFailure(AION_MSG_TEMP, __FILE__, __LINE__); \
        } \
    } while(0)

// Container Assertions
#define AION_ASSERT_EMPTY(container) \
    do { \
        if (!(container).empty()) { \
            std::ostringstream oss; \
            oss << "Container should be empty, size: " << (container).size(); \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define AION_ASSERT_SIZE(container, expected_size) \
    do { \
        if ((container).size() != (expected_size)) { \
            std::ostringstream oss; \
            oss << "Container size mismatch:\n  Actual:   " << (container).size() \
                << "\n  Expected: " << (expected_size); \
            throw aion::test::AssertionFailure(oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

// Run all tests
#define AION_RUN_ALL_TESTS(verbose) \
    aion::test::TestRunner::instance().run_all(verbose)

} // namespace aion::test

#endif // AION_TEST_HPP