#pragma once
#include "ufc/Fighter.hpp"
#include <string>
#include <vector>

namespace ufc {

// All export functions create missing parent folders, overwrite an existing
// file, and throw std::runtime_error (or std::filesystem::filesystem_error)
// if the file cannot be written.

// Quotes a CSV field if it contains a comma, quote or newline.
std::string csvEscape(const std::string& field);

// "Jon 'Bones' Jones" -> "jon_bones_jones" (safe to use in a file name).
std::string sanitizeFilename(const std::string& name);

// Columns: rank,fighter,elo,wins,losses,fights,win_rate_pct
void exportLeaderboardCsv(const std::string& path, const std::vector<Fighter>& ranked);

// Columns: fight_number,date,opponent,result,method,rating_before,rating_after,change
void exportFighterHistoryCsv(const std::string& path, const Fighter& fighter);

void writeTextFile(const std::string& path, const std::string& content);

} // namespace ufc
