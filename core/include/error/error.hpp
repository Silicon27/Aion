//
// Created by David Yang on 2025-11-13.
//
// error.hpp - Udo Diagnostics Engine
// Based on clang::DiagnosticsEngine from LLVM/Clang.
//

#ifndef ERROR_HPP
#define ERROR_HPP

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

#include "error/DiagnosticSeverity.hpp"
#include "error/diagid.hpp"
#include "support/source_manager.hpp"

namespace udo::diag {

// Forward declarations
class DiagnosticsEngine;
class DiagnosticBuilder;
class DiagnosticConsumer;

// ============================================================================
// Source Range (for highlighting)
// ============================================================================

struct CharSourceRange {
    Source_Location begin;
    Source_Location end;
    bool is_token_range = true;  // true = token range, false = char range

    CharSourceRange() = default;
    CharSourceRange(Source_Location b, Source_Location e, bool token = true)
        : begin(b), end(e), is_token_range(token) {}

    bool isValid() const;
    bool isTokenRange() const { return is_token_range; }
    bool isCharRange() const { return !is_token_range; }

    static CharSourceRange getTokenRange(Source_Location b, Source_Location e) {
        return CharSourceRange(b, e, true);
    }

    static CharSourceRange getCharRange(Source_Location b, Source_Location e) {
        return CharSourceRange(b, e, false);
    }
};

// ============================================================================
// FixItHint
// ============================================================================
// Annotates a diagnostic with code that should be inserted, removed, or
// replaced to fix the problem.

class FixItHint {
public:
    /// Code that should be replaced to correct the error.
    /// Empty for an insertion hint.
    CharSourceRange remove_range;

    /// Code in a specific range that should be inserted.
    CharSourceRange insert_from_range;

    /// The actual code to insert at the insertion location.
    std::string code_to_insert;

    bool before_previous_insertions = false;

    FixItHint() = default;

    bool isNull() const { return !remove_range.isValid(); }

    /// Create a code modification hint that inserts the given code at a location.
    static FixItHint CreateInsertion(Source_Location loc, const std::string& code,
                                     bool before_previous = false);

    /// Create a code modification hint that removes the given source range.
    static FixItHint CreateRemoval(CharSourceRange range);

    /// Create a code modification hint that replaces the given source range.
    static FixItHint CreateReplacement(CharSourceRange range, const std::string& code);
};

// ============================================================================
// Diagnostic
// ============================================================================
// Represents a single diagnostic that has been emitted.

struct Diagnostic {
    DiagID id = 0;                          ///< The diagnostic ID
    Source_Location location;               ///< Primary location
    std::vector<Source_Location> extra_locations; ///< Additional carets
    Severity severity = Severity::Warning;  ///< Severity level
    std::string message;                    ///< Formatted message
    std::vector<CharSourceRange> ranges;    ///< Source ranges to highlight
    std::vector<FixItHint> fixits;          ///< Fix-it hints

    Diagnostic() = default;
    Diagnostic(DiagID id, Source_Location loc, Severity sev, std::string msg)
        : id(id), location(loc), severity(sev), message(std::move(msg)) {}
};

// ============================================================================
// StoredDiagnostic
// ============================================================================
// A diagnostic that has been stored for later retrieval.

class StoredDiagnostic {
    DiagID id_ = 0;
    Severity severity_ = Severity::Warning;
    std::string message_;
    // Full location info would be stored here
    std::vector<CharSourceRange> ranges_;
    std::vector<FixItHint> fixits_;

public:
    StoredDiagnostic() = default;
    StoredDiagnostic(Severity sev, const Diagnostic& diag);

    DiagID getID() const { return id_; }
    Severity getSeverity() const { return severity_; }
    const std::string& getMessage() const { return message_; }

    using range_iterator = std::vector<CharSourceRange>::const_iterator;
    range_iterator range_begin() const { return ranges_.begin(); }
    range_iterator range_end() const { return ranges_.end(); }
    unsigned range_size() const { return ranges_.size(); }

    using fixit_iterator = std::vector<FixItHint>::const_iterator;
    fixit_iterator fixit_begin() const { return fixits_.begin(); }
    fixit_iterator fixit_end() const { return fixits_.end(); }
    unsigned fixit_size() const { return fixits_.size(); }
};

// ============================================================================
// DiagnosticConsumer
// ============================================================================
// Abstract interface for diagnostic consumers.

class DiagnosticConsumer {
public:
    virtual ~DiagnosticConsumer() = default;

    /// Called at the beginning of processing a source file.
    virtual void BeginSourceFile() {}

    /// Called at the end of processing a source file.
    virtual void EndSourceFile() {}

    /// Callback for when a diagnostic is emitted.
    virtual void HandleDiagnostic(Severity severity, const Diagnostic& diag) = 0;

    /// Returns the number of errors emitted.
    unsigned getNumErrors() const { return num_errors_; }

    /// Returns the number of warnings emitted.
    unsigned getNumWarnings() const { return num_warnings_; }

    /// Reset the error/warning counts.
    virtual void clear() { num_errors_ = 0; num_warnings_ = 0; }

protected:
    unsigned num_errors_ = 0;
    unsigned num_warnings_ = 0;
};

// ============================================================================
// TextDiagnosticPrinter
// ============================================================================
// A diagnostic consumer that prints diagnostics to a stream.

class TextDiagnosticPrinter : public DiagnosticConsumer {
    std::ostream* os_;
    Source_Manager* source_mgr_;
    bool show_colors_;

public:
    TextDiagnosticPrinter(std::ostream& os, Source_Manager* sm = nullptr, bool colors = true)
        : os_(&os), source_mgr_(sm), show_colors_(colors) {}

    void HandleDiagnostic(Severity severity, const Diagnostic& diag) override;

private:
    void printSeverity(Severity severity);
    void printLocation(const Diagnostic& diag);
    void printSourceLine(const Diagnostic& diag);
    void printFixItHints(const Diagnostic& diag);
};

// ============================================================================
// DiagnosticBuilder
// ============================================================================
// Helper class for building diagnostics with a fluent interface.

class DiagnosticBuilder {
    DiagnosticsEngine* engine_;
    DiagID diag_id_;
    bool is_active_ = true;

    // Diagnostic arguments for formatting
    std::vector<std::string> string_args_;
    std::vector<int64_t> int_args_;

    // Source ranges, fixits and extra locations
    std::vector<CharSourceRange> ranges_;
    std::vector<FixItHint> fixits_;
    std::vector<Source_Location> extra_locations_;

public:
    DiagnosticBuilder(DiagnosticsEngine* engine, DiagID id)
        : engine_(engine), diag_id_(id) {}

    DiagnosticBuilder(const DiagnosticBuilder&) = delete;
    DiagnosticBuilder& operator=(const DiagnosticBuilder&) = delete;

    DiagnosticBuilder(DiagnosticBuilder&& other) noexcept;
    DiagnosticBuilder& operator=(DiagnosticBuilder&& other) noexcept;

    ~DiagnosticBuilder();

    /// Add a string argument.
    DiagnosticBuilder& operator<<(const std::string& str);
    DiagnosticBuilder& operator<<(const char* str);

    /// Add an integer argument.
    DiagnosticBuilder& operator<<(int val);
    DiagnosticBuilder& operator<<(unsigned val);
    DiagnosticBuilder& operator<<(int64_t val);

    /// Add a source range to highlight.
    DiagnosticBuilder& operator<<(CharSourceRange range);

    /// Add an extra location for a caret.
    DiagnosticBuilder& operator<<(Source_Location loc);

    /// Add a fix-it hint.
    DiagnosticBuilder& operator<<(FixItHint hint);

    /// Emit the diagnostic.
    void emit();

    /// Check if this builder is still active.
    bool isActive() const { return is_active_; }

    /// Abandon this diagnostic without emitting.
    void clear() { is_active_ = false; }

private:
    friend class DiagnosticsEngine;
    std::string formatMessage(const char* format_str) const;
};

// ============================================================================
// DiagnosticsEngine
// ============================================================================
// Main class for emitting diagnostics.

class DiagnosticsEngine {
public:
    /// The level of a diagnostic after mapping.
    enum Level {
        Ignored = static_cast<int>(Severity::Ignored),
        Note = static_cast<int>(Severity::Note),
        Remark = static_cast<int>(Severity::Remark),
        Warning = static_cast<int>(Severity::Warning),
        Error = static_cast<int>(Severity::Error),
        Fatal = static_cast<int>(Severity::Fatal)
    };

private:
    Source_Manager* source_mgr_ = nullptr;
    DiagnosticConsumer* consumer_ = nullptr;
    bool owns_consumer_ = false;

    // Current diagnostic state
    Source_Location cur_diag_loc_;
    DiagID cur_diag_id_ = 0;

    // Diagnostic counts
    unsigned num_errors_ = 0;
    unsigned num_warnings_ = 0;

    // Configuration
    bool warnings_as_errors_ = false;
    bool errors_as_fatal_ = false;
    bool suppress_all_diagnostics_ = false;
    bool show_colors_ = true;
    unsigned error_limit_ = 0;  // 0 = no limit

    // Custom diagnostic mappings
    std::unordered_map<DiagID, DiagnosticMapping> diag_mappings_;

public:
    DiagnosticsEngine();
    DiagnosticsEngine(Source_Manager* sm, DiagnosticConsumer* consumer, bool owns = false);
    ~DiagnosticsEngine();

    DiagnosticsEngine(const DiagnosticsEngine&) = delete;
    DiagnosticsEngine& operator=(const DiagnosticsEngine&) = delete;

    // ---- Configuration ----

    void setSourceManager(Source_Manager* sm) { source_mgr_ = sm; }
    Source_Manager* getSourceManager() const { return source_mgr_; }

    void setClient(DiagnosticConsumer* client, bool owns = false);
    DiagnosticConsumer* getClient() const { return consumer_; }

    void setWarningsAsErrors(bool val) { warnings_as_errors_ = val; }
    bool getWarningsAsErrors() const { return warnings_as_errors_; }

    void setErrorsAsFatal(bool val) { errors_as_fatal_ = val; }
    bool getErrorsAsFatal() const { return errors_as_fatal_; }

    void setSuppressAllDiagnostics(bool val) { suppress_all_diagnostics_ = val; }
    bool getSuppressAllDiagnostics() const { return suppress_all_diagnostics_; }

    void setShowColors(bool val) { show_colors_ = val; }
    bool getShowColors() const { return show_colors_; }

    void setErrorLimit(unsigned limit) { error_limit_ = limit; }
    unsigned getErrorLimit() const { return error_limit_; }

    // ---- Diagnostic Mapping ----

    /// Set the severity for a specific diagnostic.
    void setSeverity(DiagID id, Severity sev, bool is_pragma = false);

    /// Get the current severity for a diagnostic.
    Severity getSeverity(DiagID id) const;

    // ---- Diagnostic Counts ----

    unsigned getNumErrors() const { return num_errors_; }
    unsigned getNumWarnings() const { return num_warnings_; }

    bool hasErrorOccurred() const { return num_errors_ > 0; }
    bool hasFatalErrorOccurred() const;

    void reset();

    // ---- Diagnostic Emission ----

    /// Report a diagnostic at the given location.
    DiagnosticBuilder Report(Source_Location loc, DiagID id);

    /// Report a diagnostic at the current location.
    DiagnosticBuilder Report(DiagID id);

    /// Emit a fully-formed diagnostic.
    void EmitDiagnostic(const Diagnostic& diag);

private:
    friend class DiagnosticBuilder;

    /// Process and emit the current diagnostic.
    void ProcessDiag(DiagID id, Source_Location loc,
                     const std::string& message,
                     const std::vector<CharSourceRange>& ranges,
                     const std::vector<FixItHint>& fixits,
                     const std::vector<Source_Location>& extra_locations);

    /// Get the default severity for a diagnostic ID.
    Severity getDefaultSeverity(DiagID id) const;

    /// Get the format string for a diagnostic ID.
    const char* getDiagnosticFormatString(DiagID id) const;
};

// ============================================================================
// Convenience Functions
// ============================================================================

/// Create a diagnostic engine with a text printer to stderr.
std::unique_ptr<DiagnosticsEngine> createDiagnosticsEngine(Source_Manager* sm = nullptr);

} // namespace udo::diag

#endif // ERROR_HPP
