cmake_minimum_required(VERSION 3.31)
project(aion_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Directory Setup
# ============================================================================

# Include the main source directory for access to core modules
include_directories(${CMAKE_SOURCE_DIR}/core/src)

# Include the test framework and test directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/suite)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# Test Sources - Mirror the src/core structure
# ============================================================================

# Lexer tests
set(LEXER_TEST_SOURCES
    lexer/lexer_test.cpp
)

# Parser tests
set(PARSER_TEST_SOURCES
    parser/parser_test.cpp
)

# AST tests
set(AST_TEST_SOURCES
    ast/ast_test.cpp
)

# Error tests
set(ERROR_TEST_SOURCES
    error/error_test.cpp
)

# Preprocessor tests
set(PREPROCESSOR_TEST_SOURCES
    preprocessor/preprocessor_test.cpp
)

# ============================================================================
# Core source files needed for testing
# ============================================================================
set(LEXER_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/core/src/lexer/lexer.cpp
)

set(PARSER_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/core/src/parser/parser.cpp
    ${CMAKE_SOURCE_DIR}/core/src/parser/parser_expression.cpp
    ${CMAKE_SOURCE_DIR}/core/src/lexer/lexer.cpp
    ${CMAKE_SOURCE_DIR}/core/src/error/error.cpp
    ${CMAKE_SOURCE_DIR}/core/src/support/source_manager.cpp
    ${CMAKE_SOURCE_DIR}/core/src/ast/ASTContext.cpp
)

set(AST_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/core/src/ast/ast.cpp
    ${CMAKE_SOURCE_DIR}/core/src/ast/ASTContext.cpp
)

set(ERROR_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/core/src/error/error.cpp
    ${CMAKE_SOURCE_DIR}/core/src/support/source_manager.cpp
)

set(PREPROCESSOR_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/core/src/preprocessor/preprocessor.cpp
)

set(ALL_CORE_SOURCES
    ${LEXER_CORE_SOURCES}
    ${CMAKE_SOURCE_DIR}/core/src/parser/parser.cpp
    ${CMAKE_SOURCE_DIR}/core/src/parser/parser_expression.cpp
    ${AST_CORE_SOURCES}
    ${ERROR_CORE_SOURCES}
    ${PREPROCESSOR_CORE_SOURCES}
    ${CMAKE_SOURCE_DIR}/core/src/support/source_manager.cpp
)

# ============================================================================
# Unified Test Executable (all tests in one binary)
# ============================================================================
add_executable(aion_tests
    main.cpp
    ${LEXER_TEST_SOURCES}
    ${PARSER_TEST_SOURCES}
    ${AST_TEST_SOURCES}
    ${ERROR_TEST_SOURCES}
    ${PREPROCESSOR_TEST_SOURCES}
    ${ALL_CORE_SOURCES}
)

target_include_directories(aion_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/core/src
    ${CMAKE_CURRENT_SOURCE_DIR}/suite
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================================
# Individual Test Executables (for running specific test suites)
# ============================================================================

# Lexer test executable
add_executable(lexer_tests
    ${LEXER_TEST_SOURCES}
    ${LEXER_CORE_SOURCES}
)
target_include_directories(lexer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/core/src
    ${CMAKE_CURRENT_SOURCE_DIR}/suite
)
target_compile_definitions(lexer_tests PRIVATE LEXER_TEST_STANDALONE)

# Parser test executable
add_executable(parser_tests
    ${PARSER_TEST_SOURCES}
    ${PARSER_CORE_SOURCES}
)
target_include_directories(parser_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/core/src
    ${CMAKE_CURRENT_SOURCE_DIR}/suite
)
target_compile_definitions(parser_tests PRIVATE PARSER_TEST_STANDALONE)

# AST test executable
add_executable(ast_tests
    ${AST_TEST_SOURCES}
    ${AST_CORE_SOURCES}
)
target_include_directories(ast_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/core/src
    ${CMAKE_CURRENT_SOURCE_DIR}/suite
)
target_compile_definitions(ast_tests PRIVATE AST_TEST_STANDALONE)

# Error test executable
add_executable(error_tests
    ${ERROR_TEST_SOURCES}
    ${ERROR_CORE_SOURCES}
)
target_include_directories(error_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/core/src
    ${CMAKE_CURRENT_SOURCE_DIR}/suite
)
target_compile_definitions(error_tests PRIVATE ERROR_TEST_STANDALONE)

# Preprocessor test executable
add_executable(preprocessor_tests
    ${PREPROCESSOR_TEST_SOURCES}
    ${PREPROCESSOR_CORE_SOURCES}
)
target_include_directories(preprocessor_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/core/src
    ${CMAKE_CURRENT_SOURCE_DIR}/suite
)
target_compile_definitions(preprocessor_tests PRIVATE PREPROCESSOR_TEST_STANDALONE)

# ============================================================================
# CTest Integration
# ============================================================================

add_test(NAME AllTests COMMAND aion_tests)
add_test(NAME LexerTests COMMAND lexer_tests)
add_test(NAME ParserTests COMMAND parser_tests)
add_test(NAME ASTTests COMMAND ast_tests)
add_test(NAME ErrorTests COMMAND error_tests)
add_test(NAME PreprocessorTests COMMAND preprocessor_tests)

# ============================================================================
# Custom target to run all tests with verbose output
# ============================================================================
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS aion_tests lexer_tests parser_tests ast_tests error_tests preprocessor_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)

# ============================================================================
# Custom target to run individual test suites
# ============================================================================
add_custom_target(run_lexer_tests
    COMMAND lexer_tests -v
    DEPENDS lexer_tests
    COMMENT "Running lexer tests..."
)

add_custom_target(run_parser_tests
    COMMAND parser_tests -v
    DEPENDS parser_tests
    COMMENT "Running parser tests..."
)

add_custom_target(run_ast_tests
    COMMAND ast_tests -v
    DEPENDS ast_tests
    COMMENT "Running AST tests..."
)

add_custom_target(run_error_tests
    COMMAND error_tests -v
    DEPENDS error_tests
    COMMENT "Running error tests..."
)

add_custom_target(run_preprocessor_tests
    COMMAND preprocessor_tests -v
    DEPENDS preprocessor_tests
    COMMENT "Running preprocessor tests..."
)
