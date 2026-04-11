//
// Created by David Yang on 2025-11-13.
//
// error.cpp - Aion Diagnostics Engine Implementation
//

#include <error/error.hpp>
#include <support/source_manager.hpp>
#include <support/global_constants.hpp>
#include <sstream>
#include <iomanip>
#include <set>
#include <cctype>

#define ANSI_RESET          "\033[0m"
#define ANSI_BOLD           "\033[1m"
#define ANSI_RED            "\033[31m"
#define ANSI_GREEN          "\033[32m"
#define ANSI_YELLOW         "\033[33m"
#define ANSI_BLUE           "\033[34m"
#define ANSI_MAGENTA        "\033[35m"
#define ANSI_CYAN           "\033[36m"
#define ANSI_WHITE          "\033[37m"
#define ANSI_BOLD_RED       "\033[1;31m"
#define ANSI_BOLD_GREEN     "\033[1;32m"
#define ANSI_BOLD_YELLOW    "\033[1;33m"
#define ANSI_BOLD_BLUE      "\033[1;34m"
#define ANSI_BOLD_MAGENTA   "\033[1;35m"
#define ANSI_BOLD_CYAN      "\033[1;36m"
#define ANSI_BOLD_WHITE     "\033[1;37m"
#define ANSI_GRAY           "\033[90m"

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
        , notes_(diag.notes)
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

        // Header: severity: location: message
        print_severity(severity);
        
        if (source_mgr_ && rendered.location.is_valid()) {
            std::string path = source_mgr_->get_file_path(rendered.location);
            auto [line, col] = source_mgr_->get_line_column(rendered.location);
            
            if (show_colors_) *os_ << ANSI_BOLD_CYAN;
            if (!path.empty()) *os_ << path << ":";
            *os_ << line << ":" << col << ": ";
            if (show_colors_) *os_ << ANSI_RESET;
        }

        if (show_colors_) *os_ << ANSI_BOLD_WHITE;
        *os_ << rendered.message << (show_colors_ ? ANSI_RESET : "") << "\n";

        // Source snippet
        if (source_mgr_ && rendered.location.is_valid()) {
            print_source_line(rendered, severity, false);
        }

        for (const auto& note_entry : rendered.notes) {
            const bool has_note_loc = note_entry.location.is_valid();
            const bool has_note_range = note_entry.range.is_valid();

            // Note Header
            print_severity(Severity::note);
            
            if (has_note_loc) {
                std::string path = source_mgr_->get_file_path(note_entry.location);
                auto [line, col] = source_mgr_->get_line_column(note_entry.location);
                if (show_colors_) *os_ << ANSI_BOLD_CYAN;
                if (!path.empty()) *os_ << path << ":";
                *os_ << line << ":" << col << ": ";
                if (show_colors_) *os_ << ANSI_RESET;
            }

            if (show_colors_) *os_ << ANSI_BOLD_WHITE;
            *os_ << (note_entry.text.empty() ? "related location" : note_entry.text) << (show_colors_ ? ANSI_RESET : "") << "\n";

            if (!source_mgr_ || (!has_note_loc && !has_note_range)) {
                continue;
            }

            Diagnostic note_diag;
            note_diag.id = rendered.id;
            note_diag.severity = Severity::note;
            note_diag.location = has_note_loc ? note_entry.location : note_entry.range.begin;
            if (has_note_range) {
                note_diag.ranges.push_back(note_entry.range);
            }

            print_source_line(note_diag, Severity::note, false);
        }

        os_->flush();
    }

    void TextDiagnosticPrinter::print_severity(Severity severity) {
        if (show_colors_) {
            switch (severity) {
                case Severity::note:
                    *os_ << ANSI_BOLD_CYAN;
                    break;
                case Severity::remark:
                    *os_ << ANSI_BOLD_MAGENTA;
                    break;
                case Severity::warning:
                    *os_ << ANSI_BOLD_YELLOW;
                    break;
                case Severity::error:
                case Severity::fatal:
                    *os_ << ANSI_BOLD_RED;
                    break;
                default:
                    break;
            }
        }

        std::string name = get_severity_name(severity);
        
        *os_ << name << (show_colors_ ? ANSI_RESET : "") << ": ";
    }

    void TextDiagnosticPrinter::print_location(const Diagnostic& diag, bool is_primary) {
        if (!source_mgr_) {
            return;
        }

        // Get file and location info
        if (diag.location.is_valid()) {
            std::string path = source_mgr_->get_file_path(diag.location);
            auto [line, col] = source_mgr_->get_line_column(diag.location);
            
            if (show_colors_) {
                *os_ << ANSI_BOLD_CYAN;
            }

            *os_ << (is_primary ? "--> " : "::: ");

            if (show_colors_) {
                *os_ << ANSI_RESET << ANSI_GRAY;
            }

            if (!path.empty()) {
                *os_ << path << ":";
            }
            *os_ << line << ":" << col;

            if (show_colors_) {
                *os_ << ANSI_RESET;
            }
            *os_ << "\n";
        }
    }

    void TextDiagnosticPrinter::print_source_line(const Diagnostic& diag, const Severity severity, bool show_message) {
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
        for (const auto& fixit : diag.fixits) {
            if (fixit.is_null()) continue;
            if (fixit.remove_range.begin.file == fid) {
                auto [f_line, _] = source_mgr_->get_line_column(fixit.remove_range.begin);
                lines_to_render.insert(f_line);
            }
        }

        int line_num_width = std::to_string(*lines_to_render.rbegin()).length();
        if (line_num_width < 2) {
            line_num_width = 2;
        }
        const std::string padding(line_num_width, ' ');
        const std::string color_blue = show_colors_ ? ANSI_BOLD_CYAN : "";
        const std::string color_reset = show_colors_ ? ANSI_RESET : "";
        const std::string color_green = show_colors_ ? ANSI_BOLD_GREEN : "";

        std::string color_severity = "";
        if (show_colors_) {
            switch (severity) {
                case Severity::error:
                case Severity::fatal: color_severity = ANSI_BOLD_RED; break;
                case Severity::warning: color_severity = ANSI_BOLD_YELLOW; break;
                case Severity::note: color_severity = ANSI_BOLD_CYAN; break;
                default: color_severity = ANSI_BOLD_WHITE; break;
            }
        }

        *os_ << color_blue << padding << " |\n" << color_reset;

        for (Line line_no : lines_to_render) {
            SourceLocation line_loc = source_mgr_->get_location(fid, line_no, 1);
            std::string line_text = source_mgr_->get_line_text(line_loc);
            if (!line_text.empty() && line_text.back() == '\n') {
                line_text.pop_back();
            }

            *os_ << color_blue << std::setw(line_num_width) << line_no << " | " << color_reset << highlight_line(line_text) << "\n";

            std::string marks(std::max<size_t>(line_text.size() + 1, 1), ' ');
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

                if (!range.show_underline()) {
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

            for (const auto& range : diag.ranges) {
                if (!range.is_valid() || !range.has_caret_location()) {
                    continue;
                }
                if (range.caret_location.file != fid) {
                    continue;
                }
                auto [caret_line, caret_col] = source_mgr_->get_line_column(range.caret_location);
                if (caret_line == line_no) {
                    mark_col(caret_col, '^');
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
            std::string trimmed_marks = "";
            bool primary_marks_printed = false;
            
            if (has_marks) {
                trimmed_marks = marks;
                size_t last = trimmed_marks.find_last_not_of(' ');
                if (last != std::string::npos) {
                    trimmed_marks.erase(last + 1);
                }
                *os_ << color_blue << padding << " | " << color_severity << trimmed_marks;
                primary_marks_printed = true;
                if (show_message && line_no == primary_line) {
                    *os_ << " " << color_severity << diag.message << color_reset;
                }
            }

            // Print fix-its on this line
            std::vector<const FixItHint*> line_fixits;
            for (const auto& fixit : diag.fixits) {
                if (fixit.is_null()) continue;
                if (fixit.remove_range.begin.file == fid) {
                    auto [f_line, _] = source_mgr_->get_line_column(fixit.remove_range.begin);
                    if (f_line == line_no) {
                        line_fixits.push_back(&fixit);
                    }
                }
            }

            for (size_t i = 0; i < line_fixits.size(); ++i) {
                const auto& fixit = *line_fixits[i];
                auto [start_line, start_col] = source_mgr_->get_line_column(fixit.remove_range.begin);
                auto [end_line, end_col] = source_mgr_->get_line_column(fixit.remove_range.end);
                
                std::string f_marks(std::max<size_t>(line_text.size() + 1, 1), ' ');
                if (start_col == end_col) {
                    if (start_col > 0 && start_col <= f_marks.size() + 1) f_marks[start_col - 1] = '^';
                } else {
                    for (int j = start_col - 1; j < end_col - 1 && (size_t)j < f_marks.size(); ++j) f_marks[j] = '~';
                }
                
                size_t last_f_mark = f_marks.find_last_not_of(' ');
                if (last_f_mark != std::string::npos) {
                    f_marks = f_marks.substr(0, last_f_mark + 1);
                }

                std::string help_text;
                if (!fixit.help_message.empty()) help_text = fixit.help_message;
                else if (!fixit.code_to_insert.empty() && fixit.remove_range.begin == fixit.remove_range.end) help_text = "insert `" + fixit.code_to_insert + "`";
                else if (fixit.code_to_insert.empty()) help_text = "remove this";
                else help_text = "replace with `" + fixit.code_to_insert + "`";

                if (i == 0 && primary_marks_printed && f_marks == trimmed_marks && !show_message) {
                    // Merge with primary marks
                    *os_ << color_green << " help: " << help_text << color_reset;
                } else {
                    if (primary_marks_printed || i > 0) *os_ << "\n";
                    *os_ << color_blue << padding << " | " << color_green << f_marks << " help: " << help_text << color_reset;
                    primary_marks_printed = true; // For next fixit
                }
            }

            if (primary_marks_printed) {
                *os_ << "\n";
            }
        }
    }

    std::string TextDiagnosticPrinter::highlight_line(const std::string& line) const {
        if (!show_colors_ || line.empty()) return line;

        std::string result;
        result.reserve(line.size() * 10); // Generous space for ANSI codes

        // Start with white for non-highlighted parts
        result += ANSI_WHITE;

        auto is_type = [](const std::string& word) {
            static const std::set<std::string> types = {
                "i4", "i8", "i16", "i32", "i64", "i128",
                "f4", "f8", "f16", "f32", "f64", "f128",
                "bool", "char", "str", "String", "void", "unit"
            };
            return types.find(word) != types.end();
        };

        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];

            if (std::isspace(static_cast<unsigned char>(c))) {
                result += c;
                continue;
            }

            // Line comment
            if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') {
                result += ANSI_GRAY;
                result += line.substr(i);
                result += ANSI_RESET;
                return result; // Comments are the end of the line
            }

            // String literal
            if (c == '"') {
                result += ANSI_BOLD_GREEN;
                result += c;
                for (++i; i < line.size(); ++i) {
                    result += line[i];
                    if (line[i] == '"' && (i == 0 || line[i - 1] != '\\')) break;
                }
                result += ANSI_RESET;
                result += ANSI_WHITE;
                continue;
            }

            // Char literal
            if (c == '\'') {
                result += ANSI_BOLD_GREEN;
                result += c;
                for (++i; i < line.size(); ++i) {
                    result += line[i];
                    if (line[i] == '\'' && (i == 0 || line[i - 1] != '\\')) break;
                }
                result += ANSI_RESET;
                result += ANSI_WHITE;
                continue;
            }

            // Numbers
            if (std::isdigit(static_cast<unsigned char>(c))) {
                result += ANSI_BOLD_CYAN;
                while (i < line.size() && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '.')) {
                    result += line[i++];
                }
                result += ANSI_RESET;
                result += ANSI_WHITE;
                --i;
                continue;
            }

            // Identifiers / Keywords / Types
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                size_t start = i;
                while (i < line.size() && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_')) {
                    ++i;
                }
                std::string word = line.substr(start, i - start);
                --i;

                if (aion::lexer::is_keyword(word)) {
                    result += ANSI_BOLD_MAGENTA;
                    result += word;
                    result += ANSI_RESET;
                    result += ANSI_WHITE;
                } else if (is_type(word)) {
                    result += ANSI_BOLD_YELLOW;
                    result += word;
                    result += ANSI_RESET;
                    result += ANSI_WHITE;
                } else {
                    result += word;
                }
                continue;
            }

            // Punctuation / Operators
            result += ANSI_WHITE;
            result += c;
        }

        result += ANSI_RESET;
        return result;
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
        , extra_locations_(std::move(other.extra_locations_))
        , notes_(std::move(other.notes_)) {
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
            notes_ = std::move(other.notes_);
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
        if (range.has_caret_location() && !range_contains_location(range, range.caret_location)) {
            range.has_caret_location_ = false;
            range.caret_location = Source_Location{};
        }
        ranges_.push_back(range);
        return *this;
    }

    DiagnosticBuilder& DiagnosticBuilder::operator<<(const RangeDisplay& styled_range) {
        CharSourceRange range = styled_range.range;
        if (range.has_caret_location() && !range_contains_location(range, range.caret_location)) {
            range.has_caret_location_ = false;
            range.caret_location = Source_Location{};
        }
        ranges_.push_back(range);
        return *this;
    }

    DiagnosticBuilder& DiagnosticBuilder::operator<<(const lexer::Token& token) {
        if (!is_active_ || !engine_ || !engine_->get_source_manager()) return *this;
    
        // Use the engine's current diagnostic location's file ID
        FileID fid = engine_->cur_diag_loc_.file;
        Source_Location loc = engine_->get_source_manager()->get_location(fid, token);
    
        // Create a char range covering the token lexeme
        Source_Location end_loc = loc;
        end_loc.offset += token.lexeme.length();
    
        ranges_.push_back(CharSourceRange::get_char_range(loc, end_loc));
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

    DiagnosticBuilder& DiagnosticBuilder::operator<<(const FixItMessage& message) {
        if (!fixits_.empty()) {
            fixits_.back().help_message = message.text;
        }
        return *this;
    }

    DiagnosticBuilder& DiagnosticBuilder::operator<<(const DiagnosticNote& note_text) {
        notes_.push_back(note_text);
        return *this;
    }

    DiagnosticBuilder& DiagnosticBuilder::operator<<(const NoteLocation& note_loc) {
        if (notes_.empty()) {
            notes_.push_back(note("related location"));
        }
        if (!notes_.back().range.is_valid() || range_contains_location(notes_.back().range, note_loc.location)) {
            notes_.back().location = note_loc.location;
        }
        return *this;
    }

    DiagnosticBuilder& DiagnosticBuilder::operator<<(const NoteRange& note_range) {
        if (notes_.empty()) {
            notes_.push_back(note("related range"));
        }
        notes_.back().range = note_range.range;
        if (note_range.caret_location.is_valid() && range_contains_location(note_range.range, note_range.caret_location)) {
            notes_.back().location = note_range.caret_location;
        }
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
                size_t idx = p[1] - '0';
                if (idx < string_args_.size()) {
                    result += string_args_[idx];
                    // We don't mark it as used yet, because placeholders might be out of order
                } else if (idx < string_args_.size() + int_args_.size()) {
                    result += std::to_string(int_args_[idx - string_args_.size()]);
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

        engine_->process_diag(diag_id_, engine_->cur_diag_loc_, message, notes_, ranges_, fixits_, extra_locations_);
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
            case common::err_expected_token: return "expected token";
            case common::err_file_not_found: return "file not found: '%0'";
            case common::err_invalid_character: return "invalid character";
            case common::err_matched_no_tokens: return "matched no tokens";
            case common::warn_unused_parameter: return "unused parameter '%0'";
            case common::note_previous_definition: return "previous definition is here";
            case common::note_declared_here: return "declared here";

            case parse::err_expected_expression: return "expected expression";
            case parse::err_expected_one_of: return "expected one of %0";
            case parse::err_expected_statement: return "expected statement";
            case parse::err_expected_type: return "expected type";
            case parse::err_expected_identifier: return "expected identifier";
            case parse::err_expected_semicolon: return "expected ';'";
            case parse::err_expected_lparen: return "expected '('";
            case parse::err_expected_rparen: return "expected ')'";
            case parse::err_expected_lbrace: return "expected '{'";
            case parse::err_expected_rbrace: return "expected '}'";
            case parse::err_expected_lbracket: return "expected '['";
            case parse::err_expected_rbracket: return "expected ']'";
            case parse::err_unexpected_token: return "unexpected token";
            case parse::err_mismatched_brackets: return "mismatched brackets";
            case parse::err_untyped_uninitialized_variable_declaration: return "%0"; // Use passed message
            case parse::err_expected_initialization: return "expected initialization";
            case parse::err_unrecognized_identifier: return "unrecognized identifier";
            case parse::err_unknown_operator: return "unexpected token in expression";
            case parse::warn_empty_statement: return "empty statement";

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
                                        const std::vector<DiagnosticNote>& notes,
                                        const std::vector<CharSourceRange>& ranges,
                                        const std::vector<FixItHint>& fixits,
                                        const std::vector<Source_Location>& extra_locations) {
        Diagnostic diag;
        diag.id = id;
        diag.location = loc;
        diag.severity = get_severity(id);
        diag.message = message;
        diag.notes = notes;
        diag.ranges = ranges;
        diag.fixits = fixits;
        diag.extra_locations = extra_locations;

        emit_diagnostic(diag);
    }

    CharSourceRange DiagnosticsEngine::token_range(const lexer::Token &token) const {
        FileID fid = cur_diag_loc_.file;
        SourceLocation loc = get_token_location(fid, token);
        SourceLocation end_loc = loc;
        end_loc.offset += token.lexeme.length();
        return CharSourceRange::get_char_range(loc, end_loc);
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
