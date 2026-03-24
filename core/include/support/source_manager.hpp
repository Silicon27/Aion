//
// Created by David Yang on 2025-11-30.
//

#ifndef SOURCE_MANAGER_HPP
#define SOURCE_MANAGER_HPP
#include <string>
#include <ostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <cstdint>

#define GET_COLUMN_FOR_BUFFER (data.empty() ? 1 : (data.size() - line_starts.back() + 1))

namespace aion::diag {
    struct Diagnostic;
    class DiagnosticsEngine;
}

namespace aion::lexer {
    struct Token;
}

namespace aion {

    using Column = std::size_t;
    using Line = std::size_t;

    // forward types
    using FileId = uint32_t;
    using Offset = uint64_t;

    struct SourceLocation {
        FileId file = 0;
        Offset offset = 0;

        SourceLocation() = default;
        SourceLocation(FileId f, Offset o) : file(f), offset(o) {}

        bool is_valid() const { return file != 0 || offset != 0; }
        bool is_invalid() const { return !is_valid(); }

        bool operator==(const SourceLocation& other) const {
            return file == other.file && offset == other.offset;
        }
        bool operator!=(const SourceLocation& other) const {
            return !(*this == other);
        }
    };

    struct SourceRange {
        SourceLocation begin;
        SourceLocation end;

        SourceRange() = default;
        SourceRange(SourceLocation b, SourceLocation e) : begin(b), end(e) {}

        bool is_valid() const { return begin.is_valid() && end.is_valid(); }
    };

    inline SourceLocation make_source_loc(FileId file, Offset offset) {
        return SourceLocation(file, offset);
    }

    inline FileId loc_to_file_id(SourceLocation loc) {
        return loc.file;
    }

    inline Offset loc_to_offset(SourceLocation loc) {
        return loc.offset;
    }

    // Backward-compatible aliases for legacy call sites.
    using FileID = FileId;
    using Source_Location = SourceLocation;
    using Source_Range = SourceRange;
    inline FileID loc_to_FileID(const Source_Location loc) { return loc_to_file_id(loc); }
    inline Offset loc_to_Offset(const Source_Location loc) { return loc_to_offset(loc); }

    struct Buffer {
        std::string data;                           // owned contents
        std::string path;                           // path to the original file
        std::vector<std::size_t> line_starts;       // offsets for start of each line (0-based)
        bool computed = false;                      // line starts computed

        Buffer() = default;
        Buffer(const std::string &data, const std::string &path);

        void compute_line_starts();

        Offset get_offset(Line line, Column column);
        std::pair<Line, Column> get_line_column(Offset offset);
        std::string get_line_text(Line line_no);
    };

    class SourceManager {
        std::unordered_map<FileId, Buffer> buffers;
        FileId next_file_id_ = 1;

    public:
        SourceManager() = default;

        /// add a file from a string (in-memory / virtual file). Returns a FileID
        FileId add_buffer(std::string content, std::string path="");

        /// add a buffer from disk
        FileId add_file_from_disk(const std::string &path, aion::diag::DiagnosticsEngine &diag);

        /// Get the buffer for a file ID
        Buffer* get_buffer(FileId id);
        const Buffer* get_buffer(FileId id) const;

        /// Get line and column for a source location
        std::pair<Line, Column> get_line_column(SourceLocation loc) const;

        /// Get a source location from file, line, and column
        SourceLocation get_location(FileId file, Line line, Column column) const;

        /// Get a source location from a token
        SourceLocation get_location(FileId file, const lexer::Token& token) const;

        /// Get the text of a line
        std::string get_line_text(SourceLocation loc) const;

        /// Get the file path for a location
        std::string get_file_path(SourceLocation loc) const;

        // Backward-compatible wrappers for legacy method names.
        Buffer* getBuffer(FileID id) { return get_buffer(id); }
        const Buffer* getBuffer(FileID id) const { return get_buffer(id); }
        std::pair<Line, Column> getLineColumn(Source_Location loc) const { return get_line_column(loc); }
        std::string getLineText(Source_Location loc) const { return get_line_text(loc); }
        std::string getFilePath(Source_Location loc) const { return get_file_path(loc); }
    };

    using Source_Manager = SourceManager;

}

#endif //SOURCE_MANAGER_HPP
