cmake_minimum_required(VERSION 3.31)
project(aion)

set(CMAKE_CXX_STANDARD 20)

add_executable(aion
        core/src/cli/main.cpp
        core/src/cli/compiler_invocation.cpp
        core/src/lexer/lexer.cpp
        core/src/preprocessor/preprocessor.cpp
        core/src/parser/parser.cpp
        core/src/ast/ast.cpp
        core/src/ast/decl.cpp
        core/src/ast/stmt.cpp
        core/src/ast/type.cpp
        core/src/ast/ASTContext.cpp
        core/src/error/error.cpp
        core/src/support/source_manager.cpp
        test/suite/aion_test.hpp
)

# Try to find system LLVM first
option(AION_BUILD_LLVM "Build LLVM from source" OFF)

if(NOT AION_BUILD_LLVM)
    find_package(LLVM CONFIG)
    if(NOT LLVM_FOUND)
        find_program(LLVM_CONFIG_EXE NAMES llvm-config llvm-config.exe)
        if(LLVM_CONFIG_EXE)
            execute_process(COMMAND ${LLVM_CONFIG_EXE} --cmakedir
                            OUTPUT_VARIABLE LLVM_DIR_INTERNAL
                            OUTPUT_STRIP_TRAILING_WHITESPACE)
            file(TO_CMAKE_PATH "${LLVM_DIR_INTERNAL}" LLVM_DIR_INTERNAL)
            set(LLVM_DIR "${LLVM_DIR_INTERNAL}" CACHE PATH "Path to LLVM CMake modules" FORCE)
            find_package(LLVM REQUIRED CONFIG)
        else()
            find_package(LLVM REQUIRED CONFIG) # Re-run to get standard error message
        endif()
    endif()
    message(STATUS "Found system LLVM: ${LLVM_DIR}")
else()
    message(STATUS "LLVM not found or AION_BUILD_LLVM=ON, fetching/building...")

    include(ExternalProject)

    ExternalProject_Add(
            llvm_project
            GIT_REPOSITORY https://github.com/llvm/llvm-project.git
            GIT_TAG llvmorg-16.0.0  # replace with the LLVM version you want
            PREFIX ${CMAKE_BINARY_DIR}/llvm
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/llvm/install
            -DCMAKE_BUILD_TYPE=Release
    )

    # Make your main target depend on this build
    add_dependencies(aion llvm_project)

    # After build, set LLVM_DIR to the install path for includes/libs
    set(LLVM_DIR ${CMAKE_BINARY_DIR}/llvm/install/lib/cmake/llvm)
    # find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_DIR})
endif()

# Map components and link
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
    find_library(LLD_ELF lldELF HINTS ${LLVM_LIBRARY_DIRS})
    find_library(LLD_COFF lldCOFF HINTS ${LLVM_LIBRARY_DIRS})
    find_library(LLD_MACHO lldMachO HINTS ${LLVM_LIBRARY_DIRS})
    find_library(LLD_WASM lldWasm HINTS ${LLVM_LIBRARY_DIRS})

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
