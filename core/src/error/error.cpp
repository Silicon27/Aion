//
// Created by David Yang on 2025-11-13.
//
// error.cpp - Aion Diagnostics Engine Implementation
//

#include <error/error.hpp>
#include <support/source_manager.hpp>

#include <sstream>
#include <iomanip>
#include <set>

namespace aion::diag {

    // ============================================================================
    // CharSourceRange Implementation
    // ============================================================================

    bool CharSourceRange::is_valid() const {
        // A range is valid if both begin and end have valid file IDs
        return begin.is_valid() || end.is_valid();
    }

    // ============================================================================
    // FixItHint Implementation
    // ============================================================================

    FixItHint FixItHint::create_insertion(Source_Location loc, const std::string& code,
                                         bool before_previous) {
        FixItHint hint;
        hint.remove_range = CharSourceRange::get_char_range(loc, loc);
        hint.code_to_insert = code;
        hint.before_previous_insertions = before_previous;
        return hint;
    }

    FixItHint FixItHint::create_removal(CharSourceRange range) {
        FixItHint hint;
        hint.remove_range = range;
        return hint;
    }

    FixItHint FixItHint::create_replacement(CharSourceRange range, const std::string& code) {
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

    void TextDiagnosticPrinter::handle_diagnostic(Severity severity, const Diagnostic& diag) {
        // Update counts
        if (severity == Severity::error || severity == Severity::fatal) {
            ++num_errors_;
        } else if (severity == Severity::warning) {
            ++num_warnings_;
        }

        Diagnostic rendered = diag;
        if (rendered.message.empty()) {
            rendered.message = "diagnostic emitted with no message";
        }

        // Print location and severity on the headline.
        print_location(rendered);
        print_severity(severity);
        if (!source_mgr_ || !rendered.location.is_valid()) {
            if (show_colors_) {
                *os_ << "\033[1m";
            }
            *os_ << rendered.message;
            if (show_colors_) {
                *os_ << "\033[0m";
            }
            *os_ << "\n";
        } else {
            *os_ << "\n";
            print_source_line(rendered, severity);
        }

        // Print fix-it hints
        if (!rendered.fixits.empty()) {
            print_fixit_hints(rendered);
        }

        os_->flush();
    }

    void TextDiagnosticPrinter::print_severity(Severity severity) {
        if (show_colors_) {
            switch (severity) {
                case Severity::note:
                    *os_ << "\033[1;36m";  // Bold cyan
                    break;
                case Severity::remark:
                    *os_ << "\033[1;35m";  // Bold magenta
                    break;
                case Severity::warning:
                    *os_ << "\033[1;33m";  // Bold yellow
                    break;
                case Severity::error:
                case Severity::fatal:
                    *os_ << "\033[1;31m";  // Bold red
                    break;
                default:
                    break;
            }
        }

        *os_ << get_severity_name(severity);

        if (show_colors_) {
            *os_ << "\033[0m";  // Reset
        }
        *os_ << ": ";
    }

    void TextDiagnosticPrinter::print_location(const Diagnostic& diag) {
        if (!source_mgr_) {
            return;
        }

        // Get file and location info
        if (diag.location.is_valid()) {
            std::string path = source_mgr_->get_file_path(diag.location);
            auto [line, col] = source_mgr_->get_line_column(diag.location);

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

    void TextDiagnosticPrinter::print_source_line(const Diagnostic& diag, const Severity severity) {
        if (!diag.location.is_valid()) {
            return;
        }

        const FileID fid = diag.location.file;
        auto [primary_line, primary_col] = source_mgr_->get_line_column(diag.location);

        std::set<Line> lines_to_render;
        lines_to_render.insert(primary_line);
        for (const auto& loc : diag.extra_locations) {
            if (loc.file == fid) {
                lines_to_render.insert(source_mgr_->get_line_column(loc).first);
            }
        }
        for (const auto& range : diag.ranges) {
            if (!range.is_valid() || range.begin.file != fid || range.end.file != fid) {
                continue;
            }
            auto [b_line, _b_col] = source_mgr_->get_line_column(range.begin);
            auto [e_line, _e_col] = source_mgr_->get_line_column(range.end);
            if (b_line > e_line) {
                std::swap(b_line, e_line);
            }
            for (Line line = b_line; line <= e_line; ++line) {
                lines_to_render.insert(line);
            }
        }

        int line_num_width = std::to_string(*lines_to_render.rbegin()).length();
        if (line_num_width < 2) {
            line_num_width = 2;
        }
        const std::string padding(line_num_width, ' ');
        const std::string color_blue = show_colors_ ? "\033[1;34m" : "";
        const std::string color_reset = show_colors_ ? "\033[0m" : "";
        const std::string color_green = show_colors_ ? "\033[1;32m" : "";
        const std::string color_msg = show_colors_ ? "\033[1;31m" : "";

        *os_ << color_blue << padding << " |\n" << color_reset;

        for (Line line_no : lines_to_render) {
            SourceLocation line_loc = source_mgr_->get_location(fid, line_no, 1);
            std::string line_text = source_mgr_->get_line_text(line_loc);
            if (!line_text.empty() && line_text.back() == '\n') {
                line_text.pop_back();
            }

            *os_ << color_blue << std::setw(line_num_width) << line_no << " | " << color_reset << line_text << "\n";

            std::string marks(std::max<size_t>(line_text.size(), 1), ' ');
            auto mark_col = [&](Column col, char glyph) {
                if (col == 0) {
                    return;
                }
                size_t idx = static_cast<size_t>(col - 1);
                if (idx >= marks.size()) {
                    idx = marks.size() - 1;
                }
                marks[idx] = glyph;
            };

            for (const auto& range : diag.ranges) {
                if (!range.is_valid() || range.begin.file != fid || range.end.file != fid) {
                    continue;
                }
                auto [b_line, b_col] = source_mgr_->get_line_column(range.begin);
                auto [e_line, e_col] = source_mgr_->get_line_column(range.end);
                if (line_no < b_line || line_no > e_line) {
                    continue;
                }

                Column start_col = (line_no == b_line) ? b_col : 1;
                Column end_col = (line_no == e_line) ? e_col : static_cast<Column>(line_text.size() + 1);
                size_t start_idx = (start_col > 0) ? static_cast<size_t>(start_col - 1) : 0;
                size_t end_idx_exclusive = (end_col > 0) ? static_cast<size_t>(end_col - (range.is_token_range() ? 0 : 1)) : 0;
                if (end_idx_exclusive <= start_idx) {
                    end_idx_exclusive = start_idx + 1;
                }
                end_idx_exclusive = std::min(end_idx_exclusive, marks.size());
                for (size_t i = start_idx; i < end_idx_exclusive; ++i) {
                    if (marks[i] == ' ') {
                        marks[i] = '~';
                    }
                }
            }

            if (line_no == primary_line) {
                mark_col(primary_col, '^');
            }
            for (const auto& loc : diag.extra_locations) {
                if (loc.file != fid) {
                    continue;
                }
                auto [caret_line, caret_col] = source_mgr_->get_line_column(loc);
                if (caret_line == line_no) {
                    mark_col(caret_col, '^');
                }
            }

            const bool has_marks = marks.find_first_not_of(' ') != std::string::npos;
            if (has_marks) {
                *os_ << color_blue << padding << " | " << color_green << marks << color_reset << "\n";
                if (line_no == primary_line) {
                    size_t anchor = marks.find('^');
                    if (anchor == std::string::npos) {
                        anchor = marks.find('~');
                    }
                    if (anchor == std::string::npos) {
                        anchor = (primary_col > 0) ? static_cast<size_t>(primary_col - 1) : 0;
                    }
                    *os_ << color_blue << padding << " | " << color_reset
                         << std::string(anchor, ' ') << color_msg;
                    if (show_colors_) {
                        print_severity(severity);
                    } else {
                        *os_ << get_severity_name(severity) << ": ";
                    }
                    *os_ << diag.message << color_reset << "\n";
                }
            }
        }
    }

    void TextDiagnosticPrinter::print_fixit_hints(const Diagnostic& diag) {
        for (const auto& fixit : diag.fixits) {
            if (fixit.is_null()) continue;

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
            if (source_mgr_ && fixit.remove_range.begin.is_valid()) {
                 auto [line_no, col_no] = source_mgr_->get_line_column(fixit.remove_range.begin);
                 std::string line_text = source_mgr_->get_line_text(fixit.remove_range.begin);
                 if (!line_text.empty()) {
                     if (line_text.back() == '\n') line_text.pop_back();

                     int line_num_width = std::to_string(line_no).length();
                     if (line_num_width < 2) line_num_width = 2;
                     std::string padding(line_num_width, ' ');

                     *os_ << color_blue << padding << " |\n";
                     *os_ << std::setw(line_num_width) << line_no << " | " << color_reset;

                     auto [start_line, start_col] = source_mgr_->get_line_column(fixit.remove_range.begin);
                     auto [end_line, end_col] = source_mgr_->get_line_column(fixit.remove_range.end);

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
        string_args_.emplace_back(str ? str : "(null)");
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
        const char* format_str = engine_->get_diagnostic_format_string(diag_id_);
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

        engine_->process_diag(diag_id_, engine_->cur_diag_loc_, message, ranges_, fixits_, extra_locations_);
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

    void DiagnosticsEngine::set_client(DiagnosticConsumer* client, bool owns) {
        if (owns_consumer_) {
            delete consumer_;
        }
        consumer_ = client;
        owns_consumer_ = owns;
    }

    void DiagnosticsEngine::set_severity(DiagID id, Severity sev, bool is_pragma) {
        diag_mappings_[id] = DiagnosticMapping::make(sev, !is_pragma, is_pragma);
    }

    Severity DiagnosticsEngine::get_severity(DiagID id) const {
        auto it = diag_mappings_.find(id);
        if (it != diag_mappings_.end()) {
            return it->second.get_severity();
        }
        return get_default_severity(id);
    }

    Severity DiagnosticsEngine::get_default_severity(DiagID id) const {
        // Determine default severity based on diagnostic ID ranges
        // IDs starting with err_ are errors, warn_ are warnings, note_ are notes
        // For now, use a simple heuristic based on ranges

        if (id >= DIAG_START_COMMON && id < DIAG_START_LEXER) {
            // Common diagnostics - check specific ranges
            if (id >= common::warn_unused_variable) {
                return Severity::warning;
            }
            return Severity::error;
        }
        if (id >= DIAG_START_LEXER && id < DIAG_START_PARSER) {
            return Severity::error;  // Lexer errors are usually errors
        }
        if (id >= DIAG_START_PARSER && id < DIAG_START_SEMA) {
            return Severity::error;  // Parser errors are usually errors
        }
        if (id >= DIAG_START_SEMA && id < DIAG_START_CODEGEN) {
            return Severity::error;  // Sema errors are usually errors
        }

        return Severity::warning;
    }

    const char* DiagnosticsEngine::get_diagnostic_format_string(DiagID id) const {
        switch (id) {
            case common::err_unknown_identifier: return "unknown identifier '%0'";
            case common::warn_unused_variable: return "unused variable '%0'";
            case parse::err_expected_semicolon: return "expected ';'";
            case parse::err_expected_expression: return "expected expression";
            case parse::err_unrecognized_identifier: return "unrecognized identifier";
            case parse::err_unknown_operator: return "unexpected token in expression";
            case common::err_file_not_found: return "file not found: '%0'";
            default: return nullptr;
        }
    }

    bool DiagnosticsEngine::has_fatal_error_occurred() const {
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

    DiagnosticBuilder DiagnosticsEngine::report(Source_Location loc, DiagID id) {
        cur_diag_loc_ = loc;
        cur_diag_id_ = id;
        return DiagnosticBuilder(this, id);
    }

    DiagnosticBuilder DiagnosticsEngine::report(DiagID id) {
        cur_diag_id_ = id;
        return DiagnosticBuilder(this, id);
    }

    void DiagnosticsEngine::emit_diagnostic(const Diagnostic& diag) {
        if (suppress_all_diagnostics_) {
            return;
        }

        Severity sev = diag.severity;

        // Apply severity mappings
        if (warnings_as_errors_ && sev == Severity::warning) {
            sev = Severity::error;
        }
        if (errors_as_fatal_ && sev == Severity::error) {
            sev = Severity::fatal;
        }

        // Update counts
        if (sev == Severity::error || sev == Severity::fatal) {
            ++num_errors_;
        } else if (sev == Severity::warning) {
            ++num_warnings_;
        }

        // Check error limit
        if (error_limit_ > 0 && num_errors_ > error_limit_) {
            return;  // Suppress further diagnostics
        }

        // Forward to consumer
        if (consumer_) {
            consumer_->handle_diagnostic(sev, diag);
        }
    }

    void DiagnosticsEngine::process_diag(DiagID id, Source_Location loc,
                                        const std::string& message,
                                        const std::vector<CharSourceRange>& ranges,
                                        const std::vector<FixItHint>& fixits,
                                        const std::vector<Source_Location>& extra_locations) {
        Diagnostic diag;
        diag.id = id;
        diag.location = loc;
        diag.severity = get_severity(id);
        diag.message = message;
        diag.ranges = ranges;
        diag.fixits = fixits;
        diag.extra_locations = extra_locations;

        emit_diagnostic(diag);
    }

    SourceLocation DiagnosticsEngine::get_token_location(const FileID fid, const lexer::Token &token) const {
        return get_source_manager()->get_location(fid, token);
    }

    // ============================================================================
    // Convenience Functions
    // ============================================================================

    std::unique_ptr<DiagnosticsEngine> create_diagnostics_engine(Source_Manager* sm) {
        auto consumer = new TextDiagnosticPrinter(std::cerr, sm);
        return std::make_unique<DiagnosticsEngine>(sm, consumer, true);
    }
} // namespace aion::diag
