#include "ufc/CsvLoader.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ufc {

static std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

static std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> cols;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ',')) cols.push_back(trim(cell));
    return cols;
}

std::vector<Fight> loadFightsFromCsv(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Could not open file: " + path);

    std::vector<Fight> fights;
    std::string line;
    int lineNo = 0;
    bool headerSkipped = false;

    while (std::getline(in, line)) {
        ++lineNo;
        if (trim(line).empty()) continue;
        if (!headerSkipped) { headerSkipped = true; continue; }

        const auto cols = splitCsvLine(line);
        if (cols.size() != 4) {
            throw std::runtime_error("Line " + std::to_string(lineNo) +
                                     ": expected 4 columns (date,winner,loser,method)");
        }
        const auto method = parseFinishMethod(cols[3]);
        if (!method) {
            throw std::runtime_error("Line " + std::to_string(lineNo) +
                                     ": unknown finish method '" + cols[3] + "'");
        }
        if (cols[1].empty() || cols[2].empty()) {
            throw std::runtime_error("Line " + std::to_string(lineNo) + ": empty fighter name");
        }
        fights.push_back(Fight{cols[0], cols[1], cols[2], *method});
    }

    std::stable_sort(fights.begin(), fights.end(),
                     [](const Fight& a, const Fight& b) { return a.date < b.date; });
    return fights;
}

void appendFightToCsv(const std::string& path, const Fight& fight) {
    // If the file doesn't end in a newline, add one so rows don't merge.
    bool needsNewline = false;
    {
        std::ifstream in(path, std::ios::binary);
        if (in) {
            in.seekg(0, std::ios::end);
            if (in.tellg() > 0) {
                in.seekg(-1, std::ios::end);
                char last = 0;
                in.get(last);
                needsNewline = (last != '\n');
            }
        }
    }

    std::ofstream out(path, std::ios::app);
    if (!out) throw std::runtime_error("Could not write to file: " + path);
    if (needsNewline) out << '\n';
    out << fight.date << ',' << fight.winner << ',' << fight.loser << ','
        << toString(fight.method) << '\n';
}

} // namespace ufc
