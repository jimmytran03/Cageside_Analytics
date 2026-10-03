#pragma once
#include "ufc/Fight.hpp"
#include <string>
#include <vector>

namespace ufc {
// Expected CSV header: date,winner,loser,method
// Throws std::runtime_error (with line number) on I/O or format errors.
// Returned fights are sorted chronologically (stable for same-day fights).
std::vector<Fight> loadFightsFromCsv(const std::string& path);

} 