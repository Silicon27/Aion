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
    Ignored = 1,    ///< Do not present this diagnostic, ignore it.

    Note = 2,       ///< Informational note, no detected defiance of compiler rules.

    Remark = 3,     ///< Present this diagnostic as a remark (for optimization reports, etc.)

    Warning = 4,    ///< Detected uses that defy rules, still compilable.
                    ///< Can be promoted to Error with -Werror.

    Error = 5,      ///< Compile time: cannot be handled and indicates an unrecoverable state
                    ///< (syntax errors, unknown symbol errors, etc.)
                    ///< Runtime: Unless handled, will terminate the program.

    Fatal = 6       ///< Immediately terminates compilation, cannot be recovered from.
                    ///< Indicates an unrecoverable state (e.g., too many errors).
};

/// Flavors of diagnostics we can emit. Used to filter for a particular
/// kind of diagnostic (for instance, for -W/-R flags).
enum class Flavor : uint8_t {
    WarningOrError, ///< A diagnostic that indicates a problem or potential problem.
                    ///< Can be made fatal by -Werror.
    Remark          ///< A diagnostic that indicates normal progress through compilation.
};

/// Returns true if the severity represents an error or fatal condition.
inline bool isErrorOrFatal(Severity S) {
    return S == Severity::Error || S == Severity::Fatal;
}

/// Returns a string representation of the severity level.
inline const char* getSeverityName(Severity S) {
    switch (S) {
        case Severity::Ignored: return "ignored";
        case Severity::Note:    return "note";
        case Severity::Remark:  return "remark";
        case Severity::Warning: return "warning";
        case Severity::Error:   return "error";
        case Severity::Fatal:   return "fatal error";
    }
    return "unknown";
}

} // namespace aion::diag

#endif // DIAGNOSTIC_SEVERITY_HPP
