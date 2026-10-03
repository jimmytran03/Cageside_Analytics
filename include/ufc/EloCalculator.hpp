#pragma once
#include "ufc/Fight.hpp"

namespace ufc 
{

struct EloConfig 
{
    double startRating        = 1500.0;
    double baseK              = 32.0;  
    double finishMultiplier   = 1.5;   // KO/TKO and Submission
    double decisionMultiplier = 1.0;   // Unanimous Decision
    double splitMultiplier    = 0.75;  // Split Decision
};

class EloCalculator {
public:
    explicit EloCalculator(EloConfig config = {}) : config_(config) {}

    // E_A = 1 / (1 + 10^((R_B - R_A) / 400))
    static double expectedScore(double ratingA, double ratingB);

    double multiplierFor(FinishMethod method) const;

    // Points the winner gains (and the loser loses). Always > 0.
    // delta = K * multiplier * (1 - E_winner)
    double ratingDelta(double winnerRating, double loserRating, FinishMethod method) const;

    const EloConfig& config() const { return config_; }

private:
    EloConfig config_;
};

} 
