#ifndef COMPILER_INVOCATION_HPP
#define COMPILER_INVOCATION_HPP

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <istream>

#include "compiler_config.hpp"
#include "../error/error.hpp"
#include "../preprocessor/preprocessor.hpp"
#include "../lexer/lexer.hpp"
#include "../ast/ASTContext.hpp"

// Forward declaration to avoid circular dependency
namespace aion::parse {
    class Parser;
}

namespace aion::compiler_config {

    using namespace aion;
    using namespace aion::ast;
    using namespace aion::lexer;

    /// Necessary arguments
    struct CompilerConfig {
        std::vector<std::string> sources; // sources to compile from
        Flags flags;
        // Resolved output artifact (final exe or single file output).
        // May be std::nullopt for per-source outputs (e.g. -c with multiple files).
        std::optional<std::string> output;
        // Reference to the shared diagnostics engine
        diag::DiagnosticsEngine* diag = nullptr;
    };

    // takes argv and returns a fully-populated Compiler_Config
    CompilerConfig parse(int argc, char *argv[]);

    struct PreprocessorInvoke {
        struct Param {
            std::string input_file;
            diag::DiagnosticsEngine& diag;
        };

        Param param;
        explicit PreprocessorInvoke(Param param);

        /// Invoke the preprocessor and return its result.
        /// For now this just returns a Preprocessor instance.
        Preprocessor invoke() const;
    };

    /// @brief Individual invocation for the Lexer and it's objects
    struct LexerInvoke {
        /// @brief calling parameters for invocation of the Lexer object
        struct Param {
            std::istream &input_Stream;
            diag::DiagnosticsEngine& diag;
        };

        /// Mutable because Param.inputStream may be altered by Lexer constructor
        mutable Param param;

        explicit LexerInvoke(const Param &param);

        /// @brief Initialize a Lexer object with Lexer constructor params
        /// @returns Lexer object
        std::unique_ptr<Lexer> invoke() const;
    };

    /// @brief Individual invocation of Parser and it's objects
    struct ParserInvoke {
        struct Param {
            FileId file_id;
            diag::DiagnosticsEngine& diag;
            ASTContext& context;
            std::vector<Token> tokens;
            Flags flags;
        };

        mutable Param param;

        explicit ParserInvoke(const Param& param);

        std::unique_ptr<parse::Parser> invoke() const;
    };

    struct SemaInvoke {
        struct Param {
            ASTContext& context;
            int allowed_errors;
            diag::DiagnosticsEngine& diag;
        };

        mutable Param param;

        explicit SemaInvoke(Param param);

        /// Run semantic analysis, mutate the AST in-place.
        /// For now this is a no-op stub.
        void invoke() const;
    };

    struct LinkerInvoke {
        struct Param {
            CompilerConfig &config;
            std::vector<std::string> object_files; // input object files from all sources
            diag::DiagnosticsEngine& diag;
        };

        mutable Param param;

        explicit LinkerInvoke(Param param);

        /// Invoke the linker. For now this is a stub.
        void invoke() const;
    };

} // namespace aion::compiler_config


class CompilerInvocation {
    aion::compiler_config::CompilerConfig config;
    aion::diag::DiagnosticsEngine& diag_;

public:
    explicit CompilerInvocation(aion::compiler_config::CompilerConfig config,
                                 aion::diag::DiagnosticsEngine& diag);

    /// Run the entire pipeline (preprocess, lex, parse, sema, codegen, link).
    /// Currently, codegen is not wired here; focus is on CC as orchestration.
    int run();

    /// Get the diagnostics engine
    aion::diag::DiagnosticsEngine& getDiagnostics() { return diag_; }
};

#endif //COMPILER_INVOCATION_HPP