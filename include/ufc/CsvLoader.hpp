#pragma once
#include "ufc/Fight.hpp"
#include <string>
#include <vector>

namespace ufc {

// Expected CSV header: date,winner,loser,method
// Throws std::runtime_error (with line number) on I/O or format errors.
// Returned fights are sorted chronologically (stable for same-day fights).
std::vector<Fight> loadFightsFromCsv(const std::string& path);

// Appends one fight to an existing CSV so it persists between runs.
// Throws std::runtime_error if the file can't be written.
void appendFightToCsv(const std::string& path, const Fight& fight);

} // namespace ufc
