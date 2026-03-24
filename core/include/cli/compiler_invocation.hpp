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
    struct Compiler_Config {
        std::vector<std::string> sources; // sources to compile from
        Flags flags;
        // Resolved output artifact (final exe or single file output).
        // May be std::nullopt for per-source outputs (e.g. -c with multiple files).
        std::optional<std::string> output;
        // Reference to the shared diagnostics engine
        diag::DiagnosticsEngine* diag = nullptr;
    };

    // takes argv and returns a fully-populated Compiler_Config
    Compiler_Config parse(int argc, char *argv[]);

    struct Preprocessor_Invoke {
        struct Param {
            std::string input_file;
            diag::DiagnosticsEngine& diag;
        };

        Param param;
        explicit Preprocessor_Invoke(Param param);

        /// Invoke the preprocessor and return its result.
        /// For now this just returns a Preprocessor instance.
        Preprocessor invoke() const;
    };

    /// @brief Individual invocation for the Lexer and it's objects
    struct Lexer_Invoke {
        /// @brief calling parameters for invocation of the Lexer object
        struct Param {
            std::istream &input_Stream;
            diag::DiagnosticsEngine& diag;
        };

        /// Mutable because Param.inputStream may be altered by Lexer constructor
        mutable Param param;

        explicit Lexer_Invoke(const Param &param);

        /// @brief Initialize a Lexer object with Lexer constructor params
        /// @returns Lexer object
        std::unique_ptr<Lexer> invoke() const;
    };

    /// @brief Individual invocation of Parser and it's objects
    struct Parser_Invoke {
        struct Param {
            FileId file_id;
            diag::DiagnosticsEngine& diag;
            ASTContext& context;
            std::vector<Token> tokens;
            Flags flags;
        };

        mutable Param param;

        explicit Parser_Invoke(const Param& param);

        std::unique_ptr<parse::Parser> invoke() const;
    };

    struct Sema_Invoke {
        struct Param {
            ASTContext& context;
            int allowed_errors;
            diag::DiagnosticsEngine& diag;
        };

        mutable Param param;

        explicit Sema_Invoke(Param param);

        /// Run semantic analysis, mutate the AST in-place.
        /// For now this is a no-op stub.
        void invoke() const;
    };

    struct Linker_Invoke {
        struct Param {
            Compiler_Config &config;
            std::vector<std::string> object_files; // input object files from all sources
            diag::DiagnosticsEngine& diag;
        };

        mutable Param param;

        explicit Linker_Invoke(Param param);

        /// Invoke the linker. For now this is a stub.
        void invoke() const;
    };

} // namespace aion::compiler_config


class Compiler_Invocation {
    aion::compiler_config::Compiler_Config config;
    aion::diag::DiagnosticsEngine& diag_;
    aion::ast::ASTContext context_;

public:
    explicit Compiler_Invocation(const aion::compiler_config::Compiler_Config& config,
                                 aion::diag::DiagnosticsEngine& diag);

    /// Run the entire pipeline (preprocess, lex, parse, sema, codegen, link).
    /// Currently, codegen is not wired here; focus is on CC as orchestration.
    int run();

    /// Get the diagnostics engine
    aion::diag::DiagnosticsEngine& getDiagnostics() { return diag_; }

    /// Get the AST context
    aion::ast::ASTContext& getASTContext() { return context_; }
};

#endif //COMPILER_INVOCATION_HPP