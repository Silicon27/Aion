//
// Created by David Yang on 2026-02-03.
//

#ifndef COMPILER_CONFIG_HPP
#define COMPILER_CONFIG_HPP

#include <string>

namespace aion::compiler_config {

    enum class Opt_Level {
        O0, // compiler attempts 1:1 of source code, minimal change in output
        O1, // refer to https://llvm.org/doxygen/classllvm_1_1OptimizationLevel.html for all the below.
        O2,
        O3,
        Os,
        Oz
    };

    enum class Output_Format {
        /// Native object file (ELF / Mach-O / COFF depending on target)
        Object,

        /// Assembly (.s)
        Assembly,

        /// LLVM IR (textual .ll)
        LLVM_IR,

        /// LLVM Bitcode (.bc)
        LLVM_Bitcode,

        /// wasm object file (WebAssembly)
        Wasm_Object,

        /// wasm text format (.wat)
        Wasm_Text,

        /// Null output (used for benchmarking / analysis)
        Null,

        // Native executable, requires lld
        Executable
    };

    struct Flags {
        // frontend flags
        bool verbose = false;
        int max_error_count = 20;
        bool show_fixits = false;
        bool ast = false;

        // backend flags
        Opt_Level     level        = Opt_Level::O1;
        Output_Format output_format = Output_Format::Executable;
        std::string   output_file;        // raw -o argument, if any

        // pipeline control
        bool          link = true;       // whether to invoke the linker / produce final executable
    };

} // namespace aion::compiler_config

#endif //COMPILER_CONFIG_HPP
