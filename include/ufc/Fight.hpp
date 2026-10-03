#pragma once
#include <optional>
#include <string>

namespace ufc {

enum class FinishMethod {
    KO_TKO,
    Submission,
    UnanimousDecision,
    SplitDecision
};

// Parses CSV-style text ("KO/TKO", "SUB", "UD", "SD", ...), case-insensitive.
std::optional<FinishMethod> parseFinishMethod(const std::string& text);
std::string toString(FinishMethod method);

// One historical fight. Draws are not modeled: every fight has a winner.
struct Fight {
    std::string date;    // ISO "YYYY-MM-DD" so lexicographic order == chronological
    std::string winner;
    std::string loser;
    FinishMethod method;
};

} 
