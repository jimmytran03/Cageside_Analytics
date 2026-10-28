#include "ufc/EloCalculator.hpp"
#include <cmath>

namespace ufc
{
    double EloCalculator::expectedScore(double ratingA, double ratingB)
    {
        return 1.0 / (1.0 + std::pow(10.0, (ratingB - ratingA) / 400.0));
    }

    double EloCalculator::multiplierFor(FinishMethod method) const
    {
        switch (method)
        {
        case FinishMethod::KO_TKO:
        case FinishMethod::Submission:
            return config_.finishMultiplier;
        case FinishMethod::UnanimousDecision:
            return config_.decisionMultiplier;
        case FinishMethod::SplitDecision:
            return config_.splitMultiplier;
        }

        return config_.decisionMultiplier;
    }

    double EloCalculator::ratingDelta(double winnerRating, double loserRating,
                                      FinishMethod method) const
    {
        const double expectedWinner{ expectedScore(winnerRating, loserRating) };
        const double kAdjusted{ config_.baseK * multiplierFor(method) };

        // The winner's actual score is always 1.
        return kAdjusted * (1.0 - expectedWinner);
    }

} 