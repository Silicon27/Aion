//
// Created by David Yang on 2025-11-13.
//
// error.cpp - Udo Diagnostics Engine Implementation
//

#include <error/error.hpp>
#include <support/source_manager.hpp>

#include <sstream>
#include <iomanip>

namespace udo::diag {

// ============================================================================
// CharSourceRange Implementation
// ============================================================================

bool CharSourceRange::isValid() const {
    // A range is valid if both begin and end have valid file IDs
    return begin.isValid() || end.isValid();
}

// ============================================================================
// FixItHint Implementation
// ============================================================================

FixItHint FixItHint::CreateInsertion(Source_Location loc, const std::string& code,
                                     bool before_previous) {
    FixItHint hint;
    hint.remove_range = CharSourceRange::getCharRange(loc, loc);
    hint.code_to_insert = code;
    hint.before_previous_insertions = before_previous;
    return hint;
}

FixItHint FixItHint::CreateRemoval(CharSourceRange range) {
    FixItHint hint;
    hint.remove_range = range;
    return hint;
}

FixItHint FixItHint::CreateReplacement(CharSourceRange range, const std::string& code) {
    FixItHint hint;
    hint.remove_range = range;
    hint.code_to_insert = code;
    return hint;
}

// ============================================================================
// StoredDiagnostic Implementation
// ============================================================================

StoredDiagnostic::StoredDiagnostic(Severity sev, const Diagnostic& diag)
    : id_(diag.id)
    , severity_(sev)
    , message_(diag.message)
    , ranges_(diag.ranges)
    , fixits_(diag.fixits) {}

// ============================================================================
// TextDiagnosticPrinter Implementation
// ============================================================================

void TextDiagnosticPrinter::HandleDiagnostic(Severity severity, const Diagnostic& diag) {
    // Update counts
    if (severity == Severity::Error || severity == Severity::Fatal) {
        ++num_errors_;
    } else if (severity == Severity::Warning) {
        ++num_warnings_;
    }

    // Print location and severity
    printLocation(diag);
    printSeverity(severity);

    // Print message in bold
    if (show_colors_) {
        *os_ << "\033[1m";
    }
    *os_ << diag.message;
    if (show_colors_) {
        *os_ << "\033[0m";
    }
    *os_ << "\n";

    // Print source line if we have a source manager
    if (source_mgr_) {
        printSourceLine(diag);
    }

    // Print fix-it hints
    if (!diag.fixits.empty()) {
        printFixItHints(diag);
    }

    os_->flush();
}

void TextDiagnosticPrinter::printSeverity(Severity severity) {
    if (show_colors_) {
        switch (severity) {
            case Severity::Note:
                *os_ << "\033[1;36m";  // Bold cyan
                break;
            case Severity::Remark:
                *os_ << "\033[1;35m";  // Bold magenta
                break;
            case Severity::Warning:
                *os_ << "\033[1;33m";  // Bold yellow
                break;
            case Severity::Error:
            case Severity::Fatal:
                *os_ << "\033[1;31m";  // Bold red
                break;
            default:
                break;
        }
    }

    *os_ << getSeverityName(severity);

    if (show_colors_) {
        *os_ << "\033[0m";  // Reset
    }
    *os_ << ": ";
}

void TextDiagnosticPrinter::printLocation(const Diagnostic& diag) {
    if (!source_mgr_) {
        return;
    }

    // Get file and location info
    if (diag.location.isValid()) {
        std::string path = source_mgr_->getFilePath(diag.location);
        auto [line, col] = source_mgr_->getLineColumn(diag.location);

        if (show_colors_) {
            *os_ << "\033[1m";  // Bold
        }

        if (!path.empty()) {
            *os_ << path << ":";
        }
        *os_ << line << ":" << col << ": ";

        if (show_colors_) {
            *os_ << "\033[0m";
        }
    }
}

void TextDiagnosticPrinter::printSourceLine(const Diagnostic& diag) {
    if (!diag.location.isValid()) {
        return;
    }

    std::string line_text = source_mgr_->getLineText(diag.location);
    if (line_text.empty()) {
        return;
    }

    // Remove trailing newline
    if (!line_text.empty() && line_text.back() == '\n') {
        line_text.pop_back();
    }

    auto [line_no, col_no] = source_mgr_->getLineColumn(diag.location);

    // Determine line number width for alignment
    int line_num_width = std::to_string(line_no).length();
    if (line_num_width < 2) line_num_width = 2; // Minimum width
    std::string padding(line_num_width, ' ');

    // Color codes
    std::string color_blue = show_colors_ ? "\033[1;34m" : "";
    std::string color_reset = show_colors_ ? "\033[0m" : "";
    std::string color_green = show_colors_ ? "\033[1;32m" : "";

    // Header pipe
    *os_ << color_blue << padding << " |\n" << color_reset;

    // Source line with line number
    *os_ << color_blue << std::setw(line_num_width) << line_no << " | " << color_reset;
    *os_ << line_text << "\n";

    // Caret/Underline line
    *os_ << color_blue << padding << " | " << color_reset;

    // Create a buffer for carets and underlines
    std::string underlines(line_text.length() + 1, ' ');

    // Helper to apply highlights to the buffer
    auto apply_range = [&](const CharSourceRange& range) {
        if (!range.isValid()) return;
        auto [b_line, b_col] = source_mgr_->getLineColumn(range.begin);
        auto [e_line, e_col] = source_mgr_->getLineColumn(range.end);
        
        if (b_line == line_no) {
            size_t start = b_col - 1;
            size_t end = (e_line == line_no) ? (e_col - (range.isTokenRange() ? 0 : 1)) : line_text.length();
            if (start < end) {
                for (size_t i = start; i < end && i < underlines.length(); ++i) {
                    if (underlines[i] == ' ') underlines[i] = '~';
                }
            } else if (start == end) {
                if (start < underlines.length() && underlines[start] == ' ') {
                    underlines[start] = '^';
                }
            }
        }
    };

    // Apply all ranges
    for (const auto& range : diag.ranges) {
        apply_range(range);
    }

    // Apply primary caret
    if (col_no > 0 && col_no <= underlines.length()) {
        underlines[col_no - 1] = '^';
    }

    // Apply extra carets
    for (const auto& loc : diag.extra_locations) {
        auto [l_line, l_col] = source_mgr_->getLineColumn(loc);
        if (l_line == line_no && l_col > 0 && l_col <= underlines.length()) {
            underlines[l_col - 1] = '^';
        }
    }

    // Print the underlines with color
    *os_ << color_green << underlines << color_reset << "\n";
}

void TextDiagnosticPrinter::printFixItHints(const Diagnostic& diag) {
    for (const auto& fixit : diag.fixits) {
        if (fixit.isNull()) continue;

        std::string color_green = show_colors_ ? "\033[1;32m" : "";
        std::string color_reset = show_colors_ ? "\033[0m" : "";
        std::string color_blue = show_colors_ ? "\033[1;34m" : "";

        *os_ << color_green << "help: " << color_reset;
        if (!fixit.code_to_insert.empty() && fixit.remove_range.begin == fixit.remove_range.end) {
            *os_ << "insert \"" << fixit.code_to_insert << "\"\n";
        } else if (fixit.code_to_insert.empty()) {
            *os_ << "remove this\n";
        } else {
            *os_ << "replace with \"" << fixit.code_to_insert << "\"\n";
        }

        // Show the line with the fix applied (Rust-style)
        if (source_mgr_ && fixit.remove_range.begin.isValid()) {
             auto [line_no, col_no] = source_mgr_->getLineColumn(fixit.remove_range.begin);
             std::string line_text = source_mgr_->getLineText(fixit.remove_range.begin);
             if (!line_text.empty()) {
                 if (line_text.back() == '\n') line_text.pop_back();

                 int line_num_width = std::to_string(line_no).length();
                 if (line_num_width < 2) line_num_width = 2;
                 std::string padding(line_num_width, ' ');

                 *os_ << color_blue << padding << " |\n";
                 *os_ << std::setw(line_num_width) << line_no << " | " << color_reset;

                 auto [start_line, start_col] = source_mgr_->getLineColumn(fixit.remove_range.begin);
                 auto [end_line, end_col] = source_mgr_->getLineColumn(fixit.remove_range.end);

                 if (start_line == end_line) {
                     std::string prefix = line_text.substr(0, start_col - 1);
                     std::string suffix = (end_col - 1 < line_text.length()) ? line_text.substr(end_col - 1) : "";
                     
                     *os_ << prefix << color_green << fixit.code_to_insert << color_reset << suffix << "\n";
                     
                     // Show underline for the fix
                     *os_ << color_blue << padding << " | " << color_green;
                     for (size_t i = 0; i < prefix.length(); ++i) *os_ << " ";
                     for (size_t i = 0; i < fixit.code_to_insert.length(); ++i) *os_ << "~";
                     *os_ << color_reset << "\n";
                 }
             }
        }
    }
}

// ============================================================================
// DiagnosticBuilder Implementation
// ============================================================================

DiagnosticBuilder::DiagnosticBuilder(DiagnosticBuilder&& other) noexcept
    : engine_(other.engine_)
    , diag_id_(other.diag_id_)
    , is_active_(other.is_active_)
    , string_args_(std::move(other.string_args_))
    , int_args_(std::move(other.int_args_))
    , ranges_(std::move(other.ranges_))
    , fixits_(std::move(other.fixits_))
    , extra_locations_(std::move(other.extra_locations_)) {
    other.is_active_ = false;
}

DiagnosticBuilder& DiagnosticBuilder::operator=(DiagnosticBuilder&& other) noexcept {
    if (this != &other) {
        if (is_active_) {
            emit();
        }
        engine_ = other.engine_;
        diag_id_ = other.diag_id_;
        is_active_ = other.is_active_;
        string_args_ = std::move(other.string_args_);
        int_args_ = std::move(other.int_args_);
        ranges_ = std::move(other.ranges_);
        fixits_ = std::move(other.fixits_);
        extra_locations_ = std::move(other.extra_locations_);
        other.is_active_ = false;
    }
    return *this;
}

DiagnosticBuilder::~DiagnosticBuilder() {
    if (is_active_) {
        emit();
    }
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(const std::string& str) {
    string_args_.push_back(str);
    return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(const char* str) {
    string_args_.push_back(str ? str : "(null)");
    return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(int val) {
    int_args_.push_back(val);
    return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(unsigned val) {
    int_args_.push_back(static_cast<int64_t>(val));
    return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(int64_t val) {
    int_args_.push_back(val);
    return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(CharSourceRange range) {
    ranges_.push_back(range);
    return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(Source_Location loc) {
    extra_locations_.push_back(loc);
    return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(FixItHint hint) {
    fixits_.push_back(std::move(hint));
    return *this;
}

std::string DiagnosticBuilder::formatMessage(const char* format_str) const {
    if (!format_str) {
        return "";
    }

    std::string result;
    result.reserve(256);

    size_t str_idx = 0;
    size_t int_idx = 0;

    const char* p = format_str;
    while (*p) {
        if (*p == '%' && p[1] >= '0' && p[1] <= '9') {
            // Found a placeholder like %0, %1, etc.
            // Determine which argument to use
            // Simple approach: use string args first, then int args
            if (str_idx < string_args_.size()) {
                result += string_args_[str_idx++];
            } else if (int_idx < int_args_.size()) {
                result += std::to_string(int_args_[int_idx++]);
            } else {
                result += "%";
                result += p[1];
            }
            p += 2;
        } else {
            result += *p++;
        }
    }

    return result;
}

void DiagnosticBuilder::emit() {
    if (!is_active_ || !engine_) {
        return;
    }

    is_active_ = false;

    // Use formatting if possible, otherwise build a simple message from arguments
    std::string message;
    const char* format_str = engine_->getDiagnosticFormatString(diag_id_);
    if (format_str) {
        message = formatMessage(format_str);
    } else {
        for (const auto& arg : string_args_) {
            if (!message.empty()) message += " ";
            message += arg;
        }
        for (const auto& arg : int_args_) {
            if (!message.empty()) message += " ";
            message += std::to_string(arg);
        }
    }

    engine_->ProcessDiag(diag_id_, engine_->cur_diag_loc_, message, ranges_, fixits_, extra_locations_);
}

// ============================================================================
// DiagnosticsEngine Implementation
// ============================================================================

DiagnosticsEngine::DiagnosticsEngine() = default;

DiagnosticsEngine::DiagnosticsEngine(Source_Manager* sm, DiagnosticConsumer* consumer, bool owns)
    : source_mgr_(sm)
    , consumer_(consumer)
    , owns_consumer_(owns) {}

DiagnosticsEngine::~DiagnosticsEngine() {
    if (owns_consumer_) {
        delete consumer_;
    }
}

void DiagnosticsEngine::setClient(DiagnosticConsumer* client, bool owns) {
    if (owns_consumer_) {
        delete consumer_;
    }
    consumer_ = client;
    owns_consumer_ = owns;
}

void DiagnosticsEngine::setSeverity(DiagID id, Severity sev, bool is_pragma) {
    diag_mappings_[id] = DiagnosticMapping::make(sev, !is_pragma, is_pragma);
}

Severity DiagnosticsEngine::getSeverity(DiagID id) const {
    auto it = diag_mappings_.find(id);
    if (it != diag_mappings_.end()) {
        return it->second.getSeverity();
    }
    return getDefaultSeverity(id);
}

Severity DiagnosticsEngine::getDefaultSeverity(DiagID id) const {
    // Determine default severity based on diagnostic ID ranges
    // IDs starting with err_ are errors, warn_ are warnings, note_ are notes
    // For now, use a simple heuristic based on ranges

    if (id >= DIAG_START_COMMON && id < DIAG_START_LEXER) {
        // Common diagnostics - check specific ranges
        if (id >= common::warn_unused_variable) {
            return Severity::Warning;
        }
        return Severity::Error;
    }
    if (id >= DIAG_START_LEXER && id < DIAG_START_PARSER) {
        return Severity::Error;  // Lexer errors are usually errors
    }
    if (id >= DIAG_START_PARSER && id < DIAG_START_SEMA) {
        return Severity::Error;  // Parser errors are usually errors
    }
    if (id >= DIAG_START_SEMA && id < DIAG_START_CODEGEN) {
        return Severity::Error;  // Sema errors are usually errors
    }

    return Severity::Warning;
}

const char* DiagnosticsEngine::getDiagnosticFormatString(DiagID id) const {
    switch (id) {
        case common::err_unknown_identifier: return "unknown identifier '%0'";
        case common::warn_unused_variable: return "unused variable '%0'";
        case parse::err_expected_semicolon: return "expected ';'";
        case common::err_file_not_found: return "file not found: '%0'";
        default: return nullptr;
    }
}

bool DiagnosticsEngine::hasFatalErrorOccurred() const {
    // In a full implementation, track if any fatal errors occurred
    return false;
}

void DiagnosticsEngine::reset() {
    num_errors_ = 0;
    num_warnings_ = 0;
    cur_diag_id_ = 0;
    if (consumer_) {
        consumer_->clear();
    }
}

DiagnosticBuilder DiagnosticsEngine::Report(Source_Location loc, DiagID id) {
    cur_diag_loc_ = loc;
    cur_diag_id_ = id;
    return DiagnosticBuilder(this, id);
}

DiagnosticBuilder DiagnosticsEngine::Report(DiagID id) {
    cur_diag_id_ = id;
    return DiagnosticBuilder(this, id);
}

void DiagnosticsEngine::EmitDiagnostic(const Diagnostic& diag) {
    if (suppress_all_diagnostics_) {
        return;
    }

    Severity sev = diag.severity;

    // Apply severity mappings
    if (warnings_as_errors_ && sev == Severity::Warning) {
        sev = Severity::Error;
    }
    if (errors_as_fatal_ && sev == Severity::Error) {
        sev = Severity::Fatal;
    }

    // Update counts
    if (sev == Severity::Error || sev == Severity::Fatal) {
        ++num_errors_;
    } else if (sev == Severity::Warning) {
        ++num_warnings_;
    }

    // Check error limit
    if (error_limit_ > 0 && num_errors_ > error_limit_) {
        return;  // Suppress further diagnostics
    }

    // Forward to consumer
    if (consumer_) {
        consumer_->HandleDiagnostic(sev, diag);
    }
}

void DiagnosticsEngine::ProcessDiag(DiagID id, Source_Location loc,
                                    const std::string& message,
                                    const std::vector<CharSourceRange>& ranges,
                                    const std::vector<FixItHint>& fixits,
                                    const std::vector<Source_Location>& extra_locations) {
    Diagnostic diag;
    diag.id = id;
    diag.location = loc;
    diag.severity = getSeverity(id);
    diag.message = message;
    diag.ranges = ranges;
    diag.fixits = fixits;
    diag.extra_locations = extra_locations;

    EmitDiagnostic(diag);
}

// ============================================================================
// Convenience Functions
// ============================================================================

std::unique_ptr<DiagnosticsEngine> createDiagnosticsEngine(Source_Manager* sm) {
    auto consumer = new TextDiagnosticPrinter(std::cerr, sm);
    return std::make_unique<DiagnosticsEngine>(sm, consumer, true);
}

} // namespace udo::diag
