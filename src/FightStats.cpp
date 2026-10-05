#include "ufc/FightStats.hpp"
#include <cctype>

namespace ufc {

static bool allDigits(const std::string& s) {
    if (s.empty() || s.size() > 6) return false;
    for (unsigned char c : s) {
        if (!std::isdigit(c)) return false;
    }
    return true;
}

std::optional<std::string> validateStats(const FighterStats& s) {
    if (s.sigLanded > s.sigAttempted)
        return "Significant strikes landed cannot exceed attempted.";
    if (s.totalLanded > s.totalAttempted)
        return "Total strikes landed cannot exceed attempted.";
    if (s.takedownsLanded > s.takedownsAttempted)
        return "Takedowns landed cannot exceed attempted.";
    if (s.sigLanded > s.totalLanded)
        return "Significant strikes landed cannot exceed total strikes landed.";
    if (s.sigAttempted > s.totalAttempted)
        return "Significant strikes attempted cannot exceed total strikes attempted.";
    if (s.controlSeconds > 25 * 60)
        return "Control time cannot exceed 25:00 (five 5-minute rounds).";
    return std::nullopt;
}

std::optional<int> parseControlTime(const std::string& text) {
    const auto colon = text.find(':');
    if (colon == std::string::npos) {
        if (!allDigits(text)) return std::nullopt;
        return std::stoi(text);
    }
    const std::string minutes = text.substr(0, colon);
    const std::string seconds = text.substr(colon + 1);
    if (!allDigits(minutes) || !allDigits(seconds)) return std::nullopt;
    const int sec = std::stoi(seconds);
    if (sec >= 60) return std::nullopt;
    return std::stoi(minutes) * 60 + sec;
}

std::string formatControlTime(int seconds) {
    const int m = seconds / 60;
    const int s = seconds % 60;
    return std::to_string(m) + ":" + (s < 10 ? "0" : "") + std::to_string(s);
}

} // namespace ufc
