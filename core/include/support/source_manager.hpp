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

namespace udo::diag {
    struct Diagnostic;
    class DiagnosticsEngine;
}

namespace udo {

    using Column = std::size_t;
    using Line = std::size_t;

    // forward types
    using FileID = uint32_t;
    using Offset = uint64_t;

    struct Source_Location {
        FileID file = 0;
        Offset offset = 0;

        Source_Location() = default;
        Source_Location(FileID f, Offset o) : file(f), offset(o) {}

        bool isValid() const { return file != 0 || offset != 0; }
        bool isInvalid() const { return !isValid(); }

        bool operator==(const Source_Location& other) const {
            return file == other.file && offset == other.offset;
        }
        bool operator!=(const Source_Location& other) const {
            return !(*this == other);
        }
    };

    struct Source_Range {
        Source_Location begin;
        Source_Location end;

        Source_Range() = default;
        Source_Range(Source_Location b, Source_Location e) : begin(b), end(e) {}

        bool isValid() const { return begin.isValid() && end.isValid(); }
    };

    inline Source_Location make_source_loc(FileID file, Offset offset) {
        return Source_Location(file, offset);
    }

    inline FileID loc_to_FileID(Source_Location loc) {
        return loc.file;
    }

    inline Offset loc_to_Offset(Source_Location loc) {
        return loc.offset;
    }

    struct Buffer {
        std::string data;                           // owned contents
        std::string path;                           // path to the original file
        std::vector<std::size_t> line_starts;       // offsets for start of each line (0-based)
        bool computed = false;                      // line starts computed

        Buffer() = default;
        Buffer(const std::string &data, const std::string &path);

        void compute_line_starts();

        std::pair<Line, Column> get_line_column(Offset offset);
        std::string get_line_text(Line line_no);
    };

    class Source_Manager {
        std::unordered_map<FileID, Buffer> buffers;
        FileID next_file_id_ = 1;

    public:
        Source_Manager() = default;

        /// add a file from a string (in-memory / virtual file). Returns a FileID
        FileID add_buffer(std::string content, std::string path="");

        /// add a buffer from disk
        FileID add_file_from_disk(const std::string &path, udo::diag::DiagnosticsEngine &diag);

        /// Get the buffer for a file ID
        Buffer* getBuffer(FileID id);
        const Buffer* getBuffer(FileID id) const;

        /// Get line and column for a source location
        std::pair<Line, Column> getLineColumn(Source_Location loc) const;

        /// Get the text of a line
        std::string getLineText(Source_Location loc) const;

        /// Get the file path for a location
        std::string getFilePath(Source_Location loc) const;
    };

}

#endif //SOURCE_MANAGER_HPP
