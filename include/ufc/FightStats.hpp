#pragma once
#include <optional>
#include <string>

namespace ufc {

// One fighter's numbers for one fight.
struct FighterStats {
    int knockdowns = 0;
    int sigLanded = 0;
    int sigAttempted = 0;
    int totalLanded = 0;
    int totalAttempted = 0;
    int takedownsLanded = 0;
    int takedownsAttempted = 0;
    int submissionAttempts = 0;
    int controlSeconds = 0;
};

// Returns an error message, or nullopt if the stats are consistent.
std::optional<std::string> validateStats(const FighterStats& stats);

// Accepts "m:ss" (e.g. "3:45") or plain seconds (e.g. "225").
std::optional<int> parseControlTime(const std::string& text);

// 225 -> "3:45"
std::string formatControlTime(int seconds);

} // namespace ufc
