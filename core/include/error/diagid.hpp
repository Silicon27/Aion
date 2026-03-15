//
// Created by David Yang on 2025-12-09.
//
// DiagnosticIDs.hpp - Diagnostic ID definitions
// Based on clang::DiagnosticIDs from LLVM/Clang.
//

#ifndef DIAGID_HPP
#define DIAGID_HPP

#include <string>
#include <cstdint>

#include "error/DiagnosticSeverity.hpp"

namespace aion::diag {

// ============================================================================
// Diagnostic Category Sizes
// ============================================================================
// Size of each diagnostic category. These define the maximum number of
// diagnostics that can be defined in each category.

enum DiagnosticSizes : unsigned {
    DIAG_SIZE_COMMON    = 300,
    DIAG_SIZE_LEXER     = 200,
    DIAG_SIZE_PARSER    = 500,
    DIAG_SIZE_SEMA      = 1000,
    DIAG_SIZE_CODEGEN   = 200,
};

// ============================================================================
// Diagnostic Category Start Positions
// ============================================================================
// Start position for diagnostics in each category.

enum DiagnosticStart : unsigned {
    DIAG_START_COMMON   = 0,
    DIAG_START_LEXER    = DIAG_START_COMMON  + DIAG_SIZE_COMMON,
    DIAG_START_PARSER   = DIAG_START_LEXER   + DIAG_SIZE_LEXER,
    DIAG_START_SEMA     = DIAG_START_PARSER  + DIAG_SIZE_PARSER,
    DIAG_START_CODEGEN  = DIAG_START_SEMA    + DIAG_SIZE_SEMA,
    DIAG_UPPER_LIMIT    = DIAG_START_CODEGEN + DIAG_SIZE_CODEGEN,
};

// ============================================================================
// Diagnostic ID Type
// ============================================================================

/// The type used to represent a diagnostic ID.
using DiagID = unsigned;

/// Check if a diagnostic ID is a custom (user-defined) diagnostic.
inline bool isCustomDiag(DiagID ID) {
    return ID >= DIAG_UPPER_LIMIT;
}

// ============================================================================
// Diagnostic Classes
// ============================================================================
// The class of a diagnostic determines its default behavior.

enum class DiagClass : uint8_t {
    Invalid   = 0x00,
    Note      = 0x01,
    Remark    = 0x02,
    Warning   = 0x03,
    Extension = 0x04,
    Error     = 0x05,
};

// ============================================================================
// Built-in Diagnostic IDs
// ============================================================================
// Common diagnostics used across the compiler.

namespace common {
enum : DiagID {
    err_expected_token = DIAG_START_COMMON,
    err_unknown_identifier,
    err_file_not_found,
    err_invalid_character,
    err_matched_no_tokens,
    warn_unused_variable,
    warn_unused_parameter,
    note_previous_definition,
    note_declared_here,
};
} // namespace common

namespace lex {
enum : DiagID {
    err_unterminated_string = DIAG_START_LEXER,
    err_unterminated_char,
    err_unterminated_block_comment,
    err_invalid_numeric_literal,
    err_invalid_escape_sequence,
    err_empty_character_literal,
    warn_multi_char_constant,
};
} // namespace lex

namespace parse {
enum : DiagID {
    err_expected_expression = DIAG_START_PARSER,
    err_expected_one_of,
    err_expected_statement,
    err_expected_type,
    err_expected_identifier,
    err_expected_semicolon,
    err_expected_lparen,
    err_expected_rparen,
    err_expected_lbrace,
    err_expected_rbrace,
    err_expected_lbracket,
    err_expected_rbracket,
    err_unexpected_token,
    err_mismatched_brackets,
    warn_empty_statement,
};
} // namespace parse

namespace sema {
enum : DiagID {
    err_undeclared_identifier = DIAG_START_SEMA,
    err_redefinition,
    err_type_mismatch,
    err_invalid_operands,
    err_cannot_convert,
    err_ambiguous_call,
    err_no_matching_function,
    err_too_many_arguments,
    err_too_few_arguments,
    warn_implicit_conversion,
    warn_shadowed_variable,
    note_previous_declaration,
};
} // namespace sema

// ============================================================================
// Diagnostic Info Structure
// ============================================================================
// Stores static information about a diagnostic.

struct DiagnosticInfo {
    const char* message_template;   ///< Format string with %0, %1, etc. placeholders
    Severity default_severity;      ///< Default severity level
    DiagClass diag_class;           ///< The class of the diagnostic
    const char* category;           ///< Category name (e.g., "Lexer", "Parser")

    DiagnosticInfo(const char* msg, Severity sev, DiagClass cls, const char* cat = nullptr)
        : message_template(msg), default_severity(sev), diag_class(cls), category(cat) {}
};

// ============================================================================
// Diagnostic Mapping
// ============================================================================
// Used to track how a diagnostic's severity has been modified.

class DiagnosticMapping {
    Severity severity_ : 4;
    bool is_user_ : 1;          // Set by user (command line)
    bool is_pragma_ : 1;        // Set by pragma
    bool no_warning_as_error_ : 1;
    bool no_error_as_fatal_ : 1;

public:
    static DiagnosticMapping make(Severity sev, bool is_user = false, bool is_pragma = false) {
        DiagnosticMapping result;
        result.severity_ = sev;
        result.is_user_ = is_user;
        result.is_pragma_ = is_pragma;
        result.no_warning_as_error_ = false;
        result.no_error_as_fatal_ = false;
        return result;
    }

    Severity getSeverity() const { return severity_; }
    void setSeverity(Severity sev) { severity_ = sev; }

    bool isUser() const { return is_user_; }
    bool isPragma() const { return is_pragma_; }

    bool hasNoWarningAsError() const { return no_warning_as_error_; }
    void setNoWarningAsError(bool val) { no_warning_as_error_ = val; }

    bool hasNoErrorAsFatal() const { return no_error_as_fatal_; }
    void setNoErrorAsFatal(bool val) { no_error_as_fatal_ = val; }
};

} // namespace aion::diag

namespace diag {
    struct SimpleErrorMessagePack {

    };
}

#endif // DIAGID_HPP
