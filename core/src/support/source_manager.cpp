//
// Created by David Yang on 2025-11-30.
//

#include <support/source_manager.hpp>
#include <error/error.hpp>
#include <support/global_constants.hpp>

#include <algorithm>

namespace udo {

    Buffer::Buffer(const std::string &data, const std::string &path)
        : data(data), path(path) {}

    void Buffer::compute_line_starts() {
        line_starts.clear();
        line_starts.reserve(data.size());
        line_starts.push_back(0);

        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] == '\n') {
                line_starts.push_back(i + 1);
            }
        }
        computed = true;
    }

    std::pair<Line, Column> Buffer::get_line_column(const Offset offset) {
        if (!computed) compute_line_starts();
        if (offset >= data.size()) return {line_starts.size(), (data.empty() ? 1 : (data.size() - line_starts.back() + 1))};

        auto it = std::upper_bound(line_starts.begin(), line_starts.end(), offset);
        if (it == line_starts.begin()) return {0, offset};
        --it;
        return {(std::distance(line_starts.begin(), it) + 1), (offset - *it + 1)}; // add one because it's 1-based
    }

    std::string Buffer::get_line_text(const Line line_no) {
        if (!computed) compute_line_starts();
        if (line_no == 0 || line_no > line_starts.size()) return "";
        std::size_t start = line_starts[line_no - 1];
        std::size_t end;
        if (line_no < line_starts.size()) end = line_starts[line_no] - 1;   // can't access the last element
        else end = data.size() - 1;                                         // actual logic for last line
        return data.substr(start, end - start + 1);
    }

    FileID Source_Manager::add_buffer(std::string content, std::string path) {
        Buffer b(std::move(content), std::move(path));
        b.compute_line_starts();
        FileID id = next_file_id_++;
        buffers[id] = std::move(b);
        return id;
    }

    FileID Source_Manager::add_file_from_disk(const std::string &path, udo::diag::DiagnosticsEngine &diag) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            diag.Report(Source_Location{}, diag::common::err_file_not_found)
                << path;
            return SOURCE_MANAGER_INVALID_FILE_ID;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();

        return add_buffer(std::move(content), path);
    }

    Buffer* Source_Manager::getBuffer(FileID id) {
        auto it = buffers.find(id);
        if (it != buffers.end()) {
            return &it->second;
        }
        return nullptr;
    }

    const Buffer* Source_Manager::getBuffer(FileID id) const {
        auto it = buffers.find(id);
        if (it != buffers.end()) {
            return &it->second;
        }
        return nullptr;
    }

    std::pair<Line, Column> Source_Manager::getLineColumn(Source_Location loc) const {
        const Buffer* buf = getBuffer(loc.file);
        if (!buf) {
            return {0, 0};
        }
        // Need a non-const version for get_line_column
        return const_cast<Buffer*>(buf)->get_line_column(loc.offset);
    }

    std::string Source_Manager::getLineText(Source_Location loc) const {
        const Buffer* buf = getBuffer(loc.file);
        if (!buf) {
            return "";
        }
        auto [line, col] = const_cast<Buffer*>(buf)->get_line_column(loc.offset);
        return const_cast<Buffer*>(buf)->get_line_text(line);
    }

    std::string Source_Manager::getFilePath(Source_Location loc) const {
        const Buffer* buf = getBuffer(loc.file);
        if (!buf) {
            return "";
        }
        return buf->path;
    }

}