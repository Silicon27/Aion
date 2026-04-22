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

option(AION_BUILD_LLVM "Build LLVM from source" OFF)

if(NOT AION_BUILD_LLVM)
    # -------------------------------------------------------------------------
    # Windows: consume LLVM_PATH set by KyleMayes/install-llvm-action, or
    # fall back to whatever the user has in LLVM_DIR / CMAKE_PREFIX_PATH.
    # -------------------------------------------------------------------------
    if(WIN32 AND NOT DEFINED LLVM_DIR AND DEFINED ENV{LLVM_PATH})
        file(TO_CMAKE_PATH "$ENV{LLVM_PATH}" _LLVM_PATH)

        # KyleMayes action layout: <root>/lib/cmake/llvm/LLVMConfig.cmake
        set(_llvm_cmake_hint "${_LLVM_PATH}/lib/cmake/llvm")
        if(EXISTS "${_llvm_cmake_hint}/LLVMConfig.cmake")
            set(LLVM_DIR "${_llvm_cmake_hint}" CACHE PATH "Path to LLVM CMake modules" FORCE)
            message(STATUS "Windows: LLVM_DIR resolved from LLVM_PATH env: ${LLVM_DIR}")
        else()
            # Fallback: scan under LLVM_PATH for LLVMConfig.cmake
            file(GLOB_RECURSE _found_config
                 LIST_DIRECTORIES false
                 "${_LLVM_PATH}/**/LLVMConfig.cmake")
            if(_found_config)
                list(GET _found_config 0 _first)
                get_filename_component(_dir "${_first}" DIRECTORY)
                set(LLVM_DIR "${_dir}" CACHE PATH "Path to LLVM CMake modules" FORCE)
                message(STATUS "Windows: LLVM_DIR found via glob: ${LLVM_DIR}")
            endif()
        endif()
    endif()

    # -------------------------------------------------------------------------
    # Standard find — works on all platforms; LLVM_DIR hint used if set above.
    # -------------------------------------------------------------------------
    find_package(LLVM CONFIG)

    if(NOT LLVM_FOUND)
        find_program(LLVM_CONFIG_EXE NAMES llvm-config llvm-config.exe)
        if(LLVM_CONFIG_EXE)
            execute_process(
                COMMAND ${LLVM_CONFIG_EXE} --cmakedir
                OUTPUT_VARIABLE LLVM_DIR_INTERNAL
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            file(TO_CMAKE_PATH "${LLVM_DIR_INTERNAL}" LLVM_DIR_INTERNAL)
            set(LLVM_DIR "${LLVM_DIR_INTERNAL}" CACHE PATH "Path to LLVM CMake modules" FORCE)
            find_package(LLVM REQUIRED CONFIG)
        else()
            find_package(LLVM REQUIRED CONFIG)
        endif()
    endif()

    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")
else()
    message(STATUS "AION_BUILD_LLVM=ON, fetching/building LLVM from source...")

    include(ExternalProject)
    ExternalProject_Add(
            llvm_project
            GIT_REPOSITORY https://github.com/llvm/llvm-project.git
            GIT_TAG llvmorg-16.0.0
            PREFIX ${CMAKE_BINARY_DIR}/llvm
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/llvm/install
            -DCMAKE_BUILD_TYPE=Release
    )

    add_dependencies(aion llvm_project)
    set(LLVM_DIR ${CMAKE_BINARY_DIR}/llvm/install/lib/cmake/llvm)
endif()

if(LLVM_FOUND)
    llvm_map_components_to_libnames(llvm_libs
            core
            support
            mc
            mcjit
            mcparser
            option
            bitwriter
            passes
    )

    find_library(LLD_COMMON lldCommon HINTS ${LLVM_LIBRARY_DIRS})
    find_library(LLD_ELF    lldELF    HINTS ${LLVM_LIBRARY_DIRS})
    find_library(LLD_COFF   lldCOFF   HINTS ${LLVM_LIBRARY_DIRS})
    find_library(LLD_MACHO  lldMachO  HINTS ${LLVM_LIBRARY_DIRS})
    find_library(LLD_WASM   lldWasm   HINTS ${LLVM_LIBRARY_DIRS})

    set(lld_libs "")
    foreach(lib_var LLD_COMMON LLD_ELF LLD_COFF LLD_MACHO LLD_WASM)
        if(${lib_var})
            list(APPEND lld_libs ${${lib_var}})
        endif()
    endforeach()

    target_link_libraries(aion PRIVATE ${llvm_libs} ${lld_libs})
    target_include_directories(aion PRIVATE ${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
endif()

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
