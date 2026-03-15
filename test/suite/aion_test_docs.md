# AION Test Framework Documentation

## Introduction

The AION Test Framework is a modern C++20 testing library designed to provide a flexible, production-ready environment for verifying codebase integrity. It supports hierarchical test suites, rich assertions, configurable logging levels, and a clean, tree-structured output.

## Core Concepts

### Test Suites
A `TestSuite` is a collection of related tests. Each suite can have its own setup and teardown logic. Suites allow you to group tests by functionality (e.g., Lexer, Parser, AST).

### Test Runner
The `TestRunner` is a singleton that manages all registered test suites. It handles the execution flow, filtering, and summary reporting.

### Logging System
The framework features a hierarchical logger that supports multiple levels:
- `ERROR`: Critical failures.
- `WARN`: Issues that don't necessarily stop execution but should be noted (like expected values in failed assertions).
- `INFO`: General progress information.
- `SUCCESS`: Successful test completion.
- `DEBUG`: Detailed information for troubleshooting.
- `TRACE`: Low-level details (like file and line numbers for failures).

The output is displayed in a tree structure, making it easy to see which tests belong to which suite.

## Usage

### Defining a Test Suite
Use the `AION_TEST_SUITE(name)` macro to define a new suite. This macro also handles registration with the global `TestRunner`.

```cpp
AION_TEST_SUITE(MyFeature)
```

### Adding Tests
Use the `AION_TEST(suite_name, test_name)` macro to add a test case to a suite.

```cpp
AION_TEST(MyFeature, BasicBehavior) {
    int value = 42;
    AION_ASSERT_EQ(value, 42);
}
```

### Setup and Teardown
The framework provides macros for lifecycle management:
- `AION_SETUP(suite)`: Runs once before any tests in the suite.
- `AION_TEARDOWN(suite)`: Runs once after all tests in the suite have finished.
- `AION_BEFORE_EACH(suite)`: Runs before every individual test in the suite.
- `AION_AFTER_EACH(suite)`: Runs after every individual test in the suite.

### Logging within Tests
You can log custom messages during test execution using `AION_INFO(msg)` or `AION_DEBUG(msg)`. These messages will be properly indented under the current test in the output.

## Assertions

The framework provides a wide range of assertions to verify different conditions:

- `AION_ASSERT(condition)`: Basic boolean check.
- `AION_ASSERT_TRUE(condition)` / `AION_ASSERT_FALSE(condition)`: Semantic boolean checks.
- `AION_ASSERT_EQ(actual, expected)`: Checks for equality using `==`.
- `AION_ASSERT_NE(actual, expected)`: Checks for inequality using `!=`.
- `AION_ASSERT_LT`, `AION_ASSERT_LE`, `AION_ASSERT_GT`, `AION_ASSERT_GE`: Numeric comparisons.
- `AION_ASSERT_NEAR(actual, expected, epsilon)`: Floating point comparison within a tolerance.
- `AION_ASSERT_NULL(ptr)` / `AION_ASSERT_NOT_NULL(ptr)`: Pointer checks.
- `AION_ASSERT_STREQ(actual, expected)`: String equality.
- `AION_ASSERT_CONTAINS(string, substring)`: Verifies that a string contains a specific substring.
- `AION_ASSERT_MATCH(string, pattern)`: Verifies that a string matches a regex pattern.
- `AION_ASSERT_IN_RANGE(value, min, max)`: Verifies that a value is within the inclusive range [min, max].
- `AION_ASSERT_THROWS(statement, exception_type)`: Verifies that a statement throws a specific exception.
- `AION_ASSERT_NO_THROW(statement)`: Verifies that a statement does not throw any exceptions.

## Configuration Options

The `TestOptions` struct allows you to customize the runner's behavior:

- `verbose`: Enables more detailed logging (defaults to `LogLevel::DEBUG`).
- `stop_on_fail`: Immediately stops execution after the first failing test.
- `use_color`: Enables ANSI color highlighting in the terminal.
- `log_level`: Sets the threshold for what messages are displayed.
- `suite_filter` / `test_filter`: Only run suites or tests whose names contain the given pattern.

## Command Line Interface

The standard `main.cpp` provided with the project supports several command-line flags:
- `-v`, `--verbose`: Enable verbose output.
- `--stop-on-fail`: Stop after the first failure.
- `--no-color`: Disable colors.
- `--filter-suite=PATTERN`: Filter suites.
- `--filter-test=PATTERN`: Filter tests.
- `-l`, `--list`: List all available tests.

## Production Viability

The AION Test Framework is designed for production use by:
1. **Thread Safety**: Using localized contexts for logging (can be extended to full thread-safety with thread-local storage).
2. **Robustness**: Handling exceptions within tests and lifecycle functions to prevent the runner from crashing.
3. **Clarity**: Providing a clean, aligned ASCII summary box and hierarchical logs that help identify issues quickly.
4. **Flexibility**: Offering both macro-based and programmatic ways to define and run tests.
