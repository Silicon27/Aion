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

# Prefer LLVM_PATH (set by KyleMayes/install-llvm-action) then fall back to
# whatever llvm-config is on PATH.
if(DEFINED ENV{LLVM_PATH})
    if(WIN32)
        set(LLVM_CONFIG_EXE "$ENV{LLVM_PATH}/bin/llvm-config.exe")
    else()
        set(LLVM_CONFIG_EXE "$ENV{LLVM_PATH}/bin/llvm-config")
    endif()
else()
    find_program(LLVM_CONFIG_EXE NAMES llvm-config llvm-config.exe REQUIRED)
endif()

message(STATUS "Using llvm-config: ${LLVM_CONFIG_EXE}")

set(LLVM_COMPONENTS core support mc mcjit mcparser option bitwriter passes)

execute_process(
    COMMAND ${LLVM_CONFIG_EXE} --cxxflags
    OUTPUT_VARIABLE LLVM_CXX_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
    COMMAND ${LLVM_CONFIG_EXE} --ldflags
    OUTPUT_VARIABLE LLVM_LD_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
    COMMAND ${LLVM_CONFIG_EXE} --libs ${LLVM_COMPONENTS}
    OUTPUT_VARIABLE LLVM_LIB_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
    COMMAND ${LLVM_CONFIG_EXE} --includedir
    OUTPUT_VARIABLE LLVM_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
    COMMAND ${LLVM_CONFIG_EXE} --libdir
    OUTPUT_VARIABLE LLVM_LIB_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)

message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIR}")
message(STATUS "LLVM lib dir:     ${LLVM_LIB_DIR}")

separate_arguments(LLVM_CXX_FLAGS  NATIVE_COMMAND "${LLVM_CXX_FLAGS}")
separate_arguments(LLVM_LD_FLAGS   NATIVE_COMMAND "${LLVM_LD_FLAGS}")
separate_arguments(LLVM_LIB_FLAGS  NATIVE_COMMAND "${LLVM_LIB_FLAGS}")

target_compile_options(aion PRIVATE ${LLVM_CXX_FLAGS})
target_include_directories(aion PRIVATE ${LLVM_INCLUDE_DIR})
target_link_options(aion PRIVATE ${LLVM_LD_FLAGS})
target_link_libraries(aion PRIVATE ${LLVM_LIB_FLAGS})

# LLD (optional — link directly from libdir if present)
foreach(lld_lib lldCommon lldELF lldCOFF lldMachO lldWasm)
    find_library(LLD_${lld_lib} NAMES ${lld_lib} HINTS ${LLVM_LIB_DIR})
    if(LLD_${lld_lib})
        target_link_libraries(aion PRIVATE ${LLD_${lld_lib}})
    endif()
endforeach()

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
