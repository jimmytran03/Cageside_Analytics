#pragma once
#include "ufc/Fighter.hpp"
#include "ufc/RatingEngine.hpp"
#include <cstddef>
#include <iosfwd>
#include <vector>

namespace ufc {

// Copies fighters out of the engine's map and sorts by rating (descending).
// Ties are broken by name so output is deterministic.
std::vector<Fighter> buildLeaderboard(const RatingEngine& engine);

// Prints the top `limit` rows (0 = all) as a formatted table.
void printLeaderboard(std::ostream& out, const std::vector<Fighter>& ranked,
                      std::size_t limit = 0);

} 
