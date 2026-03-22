//
// Created by David Yang on 2025-12-09.
//
// DiagnosticSeverity.hpp - Severity enum for diagnostics
// This file is separated to avoid circular dependencies between
// error.hpp, diagid.hpp, and source_manager.hpp
//

#ifndef DIAGNOSTIC_SEVERITY_HPP
#define DIAGNOSTIC_SEVERITY_HPP

#include <cstdint>

namespace aion::diag {

/// Enum values that allow the client to map diagnostics to different
/// severity levels. Diagnostics may be promoted or demoted during
/// parsing/sema even after initialization.
///
/// Based on clang::diag::Severity from LLVM/Clang.
enum class Severity : uint8_t {
    // NOTE: 0 means "uncomputed" internally
    ignored = 1,    ///< Do not present this diagnostic, ignore it.

    note = 2,       ///< Informational note, no detected defiance of compiler rules.

    remark = 3,     ///< Present this diagnostic as a remark (for optimization reports, etc.)

    warning = 4,    ///< Detected uses that defy rules, still compilable.
                    ///< Can be promoted to Error with -Werror.

    error = 5,      ///< Compile time: cannot be handled and indicates an unrecoverable state
                    ///< (syntax errors, unknown symbol errors, etc.)
                    ///< Runtime: Unless handled, will terminate the program.

    fatal = 6       ///< Immediately terminates compilation, cannot be recovered from.
                    ///< Indicates an unrecoverable state (e.g., too many errors).
};

/// Flavors of diagnostics we can emit. Used to filter for a particular
/// kind of diagnostic (for instance, for -W/-R flags).
enum class Flavor : uint8_t {
    warning_or_error, ///< A diagnostic that indicates a problem or potential problem.
                    ///< Can be made fatal by -Werror.
    remark          ///< A diagnostic that indicates normal progress through compilation.
};

/// Returns true if the severity represents an error or fatal condition.
inline bool is_error_or_fatal(Severity S) {
    return S == Severity::error || S == Severity::fatal;
}

/// Returns a string representation of the severity level.
inline const char* get_severity_name(Severity S) {
    switch (S) {
        case Severity::ignored: return "ignored";
        case Severity::note:    return "note";
        case Severity::remark:  return "remark";
        case Severity::warning: return "warning";
        case Severity::error:   return "error";
        case Severity::fatal:   return "fatal error";
    }
    return "unknown";
}

} // namespace aion::diag

#endif // DIAGNOSTIC_SEVERITY_HPP
