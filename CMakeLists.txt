cmake_minimum_required(VERSION 3.31)
project(aion)

set(CMAKE_CXX_STANDARD 20)

add_executable(aion
        core/src/cli/main.cpp
        core/src/cli/CompilerInvocation.cpp
        core/src/lexer/lexer.cpp
        core/src/preprocessor/preprocessor.cpp
        core/src/parser/parser.cpp
        core/src/parser/parser_decl.cpp
        core/src/parser/parser_expression.cpp
        core/src/ast/ast.cpp
        core/src/ast/decl.cpp
        core/src/ast/stmt.cpp
        core/src/ast/expr.cpp
        core/src/ast/type.cpp
        core/src/ast/ASTContext.cpp
        core/src/ast/printer.cpp
        core/src/ast/RecursiveAstVisitor.cpp
        core/src/error/error.cpp
        core/src/support/source_manager.cpp
        test/suite/aion_test.hpp
)

# ============================================================================
# LLVM
# ============================================================================

execute_process(
    COMMAND "$ENV{LLVM_PATH}/bin/llvm-config.exe" --cxxflags --ldflags --libs core support mc mcjit mcparser option bitwriter passes
    OUTPUT_VARIABLE LLVM_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

separate_arguments(LLVM_FLAGS NATIVE_COMMAND "${LLVM_FLAGS}")

target_compile_options(aion PRIVATE ${LLVM_FLAGS})
target_link_options(aion PRIVATE ${LLVM_FLAGS})

include_directories(
        SYSTEM
        core/include
        core/include/support/si/include
        core/include/support
)

# ============================================================================
# Testing
# ============================================================================
enable_testing()
option(BUILD_TESTS "Build the test suite" ON)
if(BUILD_TESTS)
    add_subdirectory(test)
endif()
