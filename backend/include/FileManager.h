// FileManager.h
// -----------------------------------------------------------------------------
// Purpose : The ONLY class in the project that touches fstream.
//
// Everything above this layer deals in vector<Entity>; nothing above it knows
// that data lives in text files. That is the abstraction the brief asks for
// ("All file operations should be handled by appropriate C++ classes").
//
// Responsibilities
//   1. Create the data directory and any missing .txt file automatically.
//   2. Load records, skipping corrupt/short lines instead of crashing.
//   3. Save with an ATOMIC rewrite: write to "<file>.tmp", then rename() over
//      the real file, so an interrupted save cannot destroy existing rows.
//   4. Escape '|', '\' and newlines so descriptions may contain anything.
//
// It is a template because the same code stores every record type. That is
// generic programming, not inheritance - and it removes four copies of the
// same load/save loops.
//
// File format
//   line 1 : #CC1            (format marker + version)
//   line n : field|field|... (escaped, see escape())
// -----------------------------------------------------------------------------

#ifndef CC_FILE_MANAGER_H
#define CC_FILE_MANAGER_H

#include <cstdio>      // std::rename, std::remove
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Entity.h"
#include "Errors.h"

namespace cc {

class FileManager {
public:
    static constexpr const char* kFormatHeader = "#CC1";

    explicit FileManager(std::string dataDir) : dataDir_(std::move(dataDir)) {
        ensureDataDir();
    }

    const std::string& dataDir() const { return dataDir_; }

    std::string pathOf(const std::string& fileName) const {
        return (std::filesystem::path(dataDir_) / fileName).string();
    }

    // Create <dataDir> if it is missing. Safe to call repeatedly.
    void ensureDataDir() const {
        std::error_code ec;
        if (std::filesystem::exists(dataDir_, ec)) return;

        std::filesystem::create_directories(dataDir_, ec);
        if (ec) {
            throw StorageError("Cannot create data directory '" + dataDir_ +
                               "': " + ec.message());
        }
    }

    // -----------------------------------------------------------------------
    // Load
    // -----------------------------------------------------------------------
    // Missing file  -> empty vector (the file is created on the first save).
    // Corrupt line  -> warning on stderr, line skipped, everything else loads.
    template <typename T>
    std::vector<T> loadAll(const std::string& fileName) const {
        std::vector<T> items;
        const std::string path = pathOf(fileName);

        std::ifstream input(path);
        if (!input.is_open()) {
            return items; // first run: no file yet, not an error
        }

        std::string line;
        std::size_t lineNumber = 0;
        std::size_t skipped = 0;

        while (std::getline(input, line)) {
            ++lineNumber;

            // Strip Windows CRLF so files edited on another machine still load.
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            if (line[0] == '#') continue; // header / comment

            try {
                const Record fields = splitRecord(line);
                T item;
                item.fromRecord(fields);

                if (item.getId() <= 0) {
                    throw ValidationError("record has an invalid id");
                }
                items.push_back(item);
            } catch (const std::exception& error) {
                ++skipped;
                std::cerr << "[FileManager] " << fileName << ":" << lineNumber
                          << " skipped (" << error.what() << ")\n";
            }
        }

        if (skipped > 0) {
            std::cerr << "[FileManager] Loaded " << items.size() << " records from "
                      << fileName << ", skipped " << skipped << " corrupt line(s).\n";
        }

        return items;
    }

    // -----------------------------------------------------------------------
    // Save (atomic)
    // -----------------------------------------------------------------------
    template <typename T>
    void saveAll(const std::string& fileName, const std::vector<T>& items) const {
        ensureDataDir();

        const std::string finalPath = pathOf(fileName);
        const std::string tempPath  = finalPath + ".tmp";

        {
            std::ofstream output(tempPath, std::ios::trunc);
            if (!output.is_open()) {
                throw StorageError("Cannot open '" + tempPath + "' for writing.");
            }

            output << kFormatHeader << '\n';

            for (const T& item : items) {
                output << item.toRecord() << '\n';
            }

            output.flush();
            if (!output.good()) {
                output.close();
                std::remove(tempPath.c_str());
                throw StorageError("Failed while writing '" + tempPath + "'.");
            }
        } // stream closed here, before the rename

        // rename() is atomic on the same filesystem: either the old file is
        // still intact, or the new one is fully in place. Never a half file.
        if (std::rename(tempPath.c_str(), finalPath.c_str()) != 0) {
            std::remove(tempPath.c_str());
            throw StorageError("Cannot replace '" + finalPath + "'.");
        }
    }

    // True if the file exists and holds at least one record line.
    bool hasRecords(const std::string& fileName) const {
        std::ifstream input(pathOf(fileName));
        if (!input.is_open()) return false;

        std::string line;
        while (std::getline(input, line)) {
            if (!line.empty() && line[0] != '#') return true;
        }
        return false;
    }

    // -----------------------------------------------------------------------
    // Escaping: '|' separates fields, so it must be escaped inside a field.
    // -----------------------------------------------------------------------
    static std::string escape(const std::string& value) {
        std::string out;
        out.reserve(value.size() + 8);

        for (char c : value) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '|':  out += "\\p";  break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                default:   out.push_back(c); break;
            }
        }
        return out;
    }

    static std::string unescape(const std::string& value) {
        std::string out;
        out.reserve(value.size());

        for (std::size_t i = 0; i < value.size(); ++i) {
            if (value[i] != '\\' || i + 1 >= value.size()) {
                out.push_back(value[i]);
                continue;
            }

            switch (value[++i]) {
                case '\\': out.push_back('\\'); break;
                case 'p':  out.push_back('|');  break;
                case 'n':  out.push_back('\n'); break;
                case 'r':  out.push_back('\r'); break;
                default:   out.push_back(value[i]); break; // unknown escape: keep it
            }
        }
        return out;
    }

private:
    // Split one escaped line into unescaped fields.
    static Record splitRecord(const std::string& line) {
        Record fields;
        std::string current;
        bool escaped = false;

        for (char c : line) {
            if (escaped) {
                // Keep the backslash so unescape() can interpret the pair.
                current.push_back('\\');
                current.push_back(c);
                escaped = false;
                continue;
            }

            if (c == '\\') {
                escaped = true;
                continue;
            }

            if (c == '|') {
                fields.push_back(unescape(current));
                current.clear();
                continue;
            }

            current.push_back(c);
        }

        // Trailing lone backslash: keep it as literal text.
        if (escaped) current.push_back('\\');

        fields.push_back(unescape(current));
        return fields;
    }

    std::string dataDir_;
};

} // namespace cc

#endif // CC_FILE_MANAGER_H
