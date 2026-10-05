#pragma once
#include "ufc/Fight.hpp"
#include <string>
#include <vector>

namespace ufc {

// One entry in a fighter's career log, captured when the fight is processed.
struct FightRecord {
    std::string date;
    std::string opponent;
    FinishMethod method;
    bool won;
    double ratingBefore;
    double ratingAfter;
};

struct Fighter {
    std::string name;
    double rating = 1500.0;
    int wins = 0;
    int losses = 0;
    std::vector<FightRecord> history;  // chronological
};

} // namespace ufc
