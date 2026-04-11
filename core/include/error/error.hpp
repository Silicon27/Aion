//
// Created by David Yang on 2025-11-13.
//
// error.hpp - Aion Diagnostics Engine
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
#include <utility>

#include "error/DiagnosticSeverity.hpp"
#include "error/diagid.hpp"
#include "support/source_manager.hpp"
#include "lexer/lexer.hpp"

namespace aion::diag {

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
    bool is_token_range_ = true;  // true = token range, false = char range
    bool show_underline_ = true;
    Source_Location caret_location;
    bool has_caret_location_ = false;

    CharSourceRange() = default;
    CharSourceRange(Source_Location b, Source_Location e, bool token = true)
        : begin(b), end(e), is_token_range_(token) {}

    bool is_valid() const;
    bool is_token_range() const { return is_token_range_; }
    bool is_char_range() const { return !is_token_range_; }
    bool show_underline() const { return show_underline_; }
    bool has_caret_location() const { return has_caret_location_; }

    static CharSourceRange get_token_range(Source_Location b, Source_Location e) {
        return CharSourceRange(b, e, true);
    }

    static CharSourceRange get_char_range(Source_Location b, Source_Location e) {
        return CharSourceRange(b, e, false);
    }
};

inline CharSourceRange token_range(Source_Location b, Source_Location e) {
    return CharSourceRange::get_token_range(b, e);
}

inline CharSourceRange char_range(Source_Location b, Source_Location e) {
    return CharSourceRange::get_char_range(b, e);
}

inline bool range_contains_location(const CharSourceRange& range, Source_Location loc) {
    if (!range.is_valid() || !loc.is_valid()) {
        return false;
    }
    if (range.begin.file != loc.file || range.end.file != loc.file) {
        return false;
    }

    Offset begin = range.begin.offset;
    Offset end = range.end.offset;
    if (begin > end) {
        std::swap(begin, end);
    }
    return loc.offset >= begin && loc.offset <= end;
}

struct DiagnosticNote {
    std::string text;
    Source_Location location;
    CharSourceRange range;
};

inline DiagnosticNote note(const std::string& text) {
    return DiagnosticNote{text, Source_Location{}, CharSourceRange{}};
}

inline DiagnosticNote note(const std::string& text, Source_Location loc) {
    return DiagnosticNote{text, loc, CharSourceRange{}};
}

inline DiagnosticNote note(const std::string& text, CharSourceRange range) {
    return DiagnosticNote{text, Source_Location{}, range};
}

struct NoteLocation {
    Source_Location location;
};

inline NoteLocation at(Source_Location loc) {
    return NoteLocation{loc};
}

struct NoteRange {
    CharSourceRange range;
    Source_Location caret_location;
};

inline NoteRange with_range(CharSourceRange range) {
    return NoteRange{range, Source_Location{}};
}

inline NoteRange with_range(CharSourceRange range, NoteLocation loc) {
    return NoteRange{range, range_contains_location(range, loc.location) ? loc.location : Source_Location{}};
}

struct RangeDisplay {
    CharSourceRange range;
};

inline RangeDisplay range_display(CharSourceRange range, bool underline = true) {
    range.show_underline_ = underline;
    return RangeDisplay{range};
}

inline RangeDisplay range_display(CharSourceRange range, bool underline, Source_Location caret_location) {
    range.show_underline_ = underline;
    range.has_caret_location_ = range_contains_location(range, caret_location);
    range.caret_location = range.has_caret_location_ ? caret_location : Source_Location{};
    return RangeDisplay{range};
}

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

    /// Optional custom help text shown with this fix-it.
    std::string help_message;

    bool before_previous_insertions = false;

    FixItHint() = default;

    bool is_null() const { return !remove_range.is_valid(); }

    /// Create a code modification hint that inserts the given code at a location.
    static FixItHint create_insertion(Source_Location loc, const std::string& code,
                                     bool before_previous = false);

    /// Create a code modification hint that removes the given source range.
    static FixItHint create_removal(CharSourceRange range);

    /// Create a code modification hint that replaces the given source range.
    static FixItHint create_replacement(CharSourceRange range, const std::string& code);
};

/// Stream token that attaches a help message to the most recent fix-it.
struct FixItMessage {
    std::string text;
};

inline FixItMessage fixit_message(const std::string& text) {
    return FixItMessage{text};
}

// ============================================================================
// Diagnostic
// ============================================================================
// Represents a single diagnostic that has been emitted.

struct Diagnostic {
    DiagID id = 0;                          ///< The diagnostic ID
    Source_Location location;               ///< Primary location
    std::vector<Source_Location> extra_locations; ///< Additional carets
    Severity severity = Severity::warning;  ///< Severity level
    std::string message;                    ///< Formatted message
    std::vector<DiagnosticNote> notes;      ///< Additional note lines and optional source anchors
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
    Severity severity_ = Severity::warning;
    std::string message_;
    std::vector<DiagnosticNote> notes_;
    // Full location info would be stored here
    std::vector<CharSourceRange> ranges_;
    std::vector<FixItHint> fixits_;

public:
    StoredDiagnostic() = default;
    StoredDiagnostic(Severity sev, const Diagnostic& diag);

    DiagID get_id() const { return id_; }
    Severity get_severity() const { return severity_; }
    const std::string& get_message() const { return message_; }
    const std::vector<DiagnosticNote>& get_notes() const { return notes_; }

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
    virtual void begin_source_file() {}

    /// Called at the end of processing a source file.
    virtual void end_source_file() {}

    /// Callback for when a diagnostic is emitted.
    virtual void handle_diagnostic(Severity severity, const Diagnostic& diag) = 0;

    /// Returns the number of errors emitted.
    unsigned get_num_errors() const { return num_errors_; }

    /// Returns the number of warnings emitted.
    unsigned get_num_warnings() const { return num_warnings_; }

    /// Reset the error/warning counts.
    virtual void clear() { num_errors_ = 0; num_warnings_ = 0; }

    /// Print a summary of errors and warnings.
    virtual void print_summary() {}

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
    bool show_fixits_line_ = false;

public:
    TextDiagnosticPrinter(std::ostream& os, Source_Manager* sm = nullptr, bool colors = true)
        : os_(&os), source_mgr_(sm), show_colors_(colors) {}

    void handle_diagnostic(Severity severity, const Diagnostic& diag) override;

    void print_summary() override;

    void set_show_fixits_line(bool show) { show_fixits_line_ = show; }

private:
    void print_severity(Severity severity);
    void print_source_line(const Diagnostic& diag, Severity severity, bool show_message = false);
    void print_fixit_hints(const Diagnostic& diag);
    std::string highlight_line(const std::string& line) const;
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
    std::vector<DiagnosticNote> notes_;

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

    /// Add a source range with explicit display configuration.
    DiagnosticBuilder& operator<<(const RangeDisplay& styled_range);

    /// Add a token range to highlight.
    DiagnosticBuilder& operator<<(const lexer::Token& token);

    /// Add an extra location for a caret.
    DiagnosticBuilder& operator<<(Source_Location loc);

    /// Add a fix-it hint.
    DiagnosticBuilder& operator<<(FixItHint hint);

    /// Add a note line to this diagnostic.
    DiagnosticBuilder& operator<<(const DiagnosticNote& note_text);

    /// Attach a source location to the most recent note.
    DiagnosticBuilder& operator<<(const NoteLocation& note_loc);

    /// Attach a source range to the most recent note.
    DiagnosticBuilder& operator<<(const NoteRange& note_range);

    /// Attach text to the most recently added fix-it hint.
    DiagnosticBuilder& operator<<(const FixItMessage& message);

    /// Emit the diagnostic.
    void emit();

    /// Check if this builder is still active.
    bool is_active() const { return is_active_; }

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
        ignored = static_cast<int>(Severity::ignored),
        note = static_cast<int>(Severity::note),
        remark = static_cast<int>(Severity::remark),
        warning = static_cast<int>(Severity::warning),
        error = static_cast<int>(Severity::error),
        fatal = static_cast<int>(Severity::fatal)
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

    void set_source_manager(Source_Manager* sm) { source_mgr_ = sm; }
    Source_Manager* get_source_manager() const { return source_mgr_; }

    void set_client(DiagnosticConsumer* client, bool owns = false);
    DiagnosticConsumer* get_client() const { return consumer_; }

    void set_warnings_as_errors(bool val) { warnings_as_errors_ = val; }
    bool get_warnings_as_errors() const { return warnings_as_errors_; }

    void set_errors_as_fatal(bool val) { errors_as_fatal_ = val; }
    bool get_errors_as_fatal() const { return errors_as_fatal_; }

    void set_suppress_all_diagnostics(bool val) { suppress_all_diagnostics_ = val; }
    bool get_suppress_all_diagnostics() const { return suppress_all_diagnostics_; }

    void set_show_colors(bool val) { show_colors_ = val; }
    bool get_show_colors() const { return show_colors_; }

    void set_error_limit(unsigned limit) { error_limit_ = limit; }
    unsigned get_error_limit() const { return error_limit_; }

    // ---- Diagnostic Mapping ----

    /// Set the severity for a specific diagnostic.
    void set_severity(DiagID id, Severity sev, bool is_pragma = false);

    /// Get the current severity for a diagnostic.
    Severity get_severity(DiagID id) const;

    // ---- Diagnostic Counts ----

    unsigned get_num_errors() const { return num_errors_; }
    unsigned get_num_warnings() const { return num_warnings_; }

    bool has_error_occurred() const { return num_errors_ > 0; }
    bool has_fatal_error_occurred() const;

    void reset();

    // ---- Diagnostic Emission ----

    /// Report a diagnostic at the given location.
    DiagnosticBuilder report(Source_Location loc, DiagID id);

    /// Report a diagnostic at the current location.
    DiagnosticBuilder report(DiagID id);

    /// Emit a fully-formed diagnostic.
    void emit_diagnostic(const Diagnostic& diag);

    /// Get a character range for a token.
    CharSourceRange token_range(const lexer::Token& token) const;

    SourceLocation get_token_location(FileID fid, const lexer::Token& token) const;

private:
    friend class DiagnosticBuilder;

    /// Process and emit the current diagnostic.
    void process_diag(DiagID id, Source_Location loc,
                     const std::string& message,
                     const std::vector<DiagnosticNote>& notes,
                     const std::vector<CharSourceRange>& ranges,
                     const std::vector<FixItHint>& fixits,
                     const std::vector<Source_Location>& extra_locations);

    /// Get the default severity for a diagnostic ID.
    Severity get_default_severity(DiagID id) const;

    /// Get the format string for a diagnostic ID.
    const char* get_diagnostic_format_string(DiagID id) const;
};

// ============================================================================
// Convenience Functions
// ============================================================================

/// Create a diagnostic engine with a text printer to stderr.
std::unique_ptr<DiagnosticsEngine> create_diagnostics_engine(Source_Manager* sm = nullptr);

} // namespace aion::diag

#endif // ERROR_HPP
