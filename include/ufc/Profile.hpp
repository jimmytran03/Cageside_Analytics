#pragma once
#include "ufc/Fighter.hpp"
#include <cstddef>
#include <iosfwd>

namespace ufc {

// Prints a fighter's summary and their rating after every fight.
// `rank` is 1-based; `startRating` is the baseline used for peak/low stats.
void printProfile(std::ostream& out, const Fighter& fighter,
                  std::size_t rank, std::size_t totalFighters, double startRating);

} // namespace ufc
