#pragma once
#include "ufc/Fight.hpp"
#include "ufc/FightStats.hpp"
#include <iosfwd>
#include <string>

namespace ufc {

// Point values used to turn raw stats into a score. Tweak freely.
struct ScoringWeights {
    double knockdown         = 10.0;  // per knockdown
    double sigStrike         = 0.5;   // per significant strike landed
    double otherStrike       = 0.1;   // per non-significant strike landed
    double takedown          = 4.0;   // per takedown landed
    double submissionAttempt = 2.0;   // per submission attempt
    double controlPerMinute  = 1.5;   // per minute of control time
    double closeMargin       = 0.10;  // score margin below this = "split decision"
};

struct ScoreBreakdown {
    double knockdowns = 0.0;
    double striking   = 0.0;
    double grappling  = 0.0;
    double control    = 0.0;
    double total() const { return knockdowns + striking + grappling + control; }
};

enum class Winner { FighterA, FighterB, Draw };

struct FightVerdict {
    ScoreBreakdown a;
    ScoreBreakdown b;
    Winner winner = Winner::Draw;
    double margin = 0.0;  // |scoreA - scoreB| / (scoreA + scoreB), 0..1
    FinishMethod method = FinishMethod::UnanimousDecision;  // how to record it
};

class FightScorer {
public:
    explicit FightScorer(ScoringWeights weights = {}) : weights_(weights) {}

    ScoreBreakdown score(const FighterStats& stats) const;
    FightVerdict judge(const FighterStats& a, const FighterStats& b) const;

private:
    ScoringWeights weights_;
};

void printVerdict(std::ostream& out,
                  const std::string& nameA, const FighterStats& a,
                  const std::string& nameB, const FighterStats& b,
                  const FightVerdict& verdict);

} // namespace ufc
