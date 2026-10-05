#pragma once
#include "ufc/EloCalculator.hpp"
#include "ufc/Fight.hpp"
#include <cstddef>
#include <iosfwd>
#include <limits>
#include <string>
#include <vector>

namespace ufc {

// Controls which fights are SCORED. Every fight is still processed in order,
// so ratings keep evolving; the window only decides what counts in the metrics.
struct BacktestOptions {
    std::size_t evalFrom = 0;                                       // first scored index
    std::size_t evalTo = std::numeric_limits<std::size_t>::max();   // one past the last
    int minPriorFights = 0;  // both fighters need at least this many earlier fights
};

// Calibration: when the model says "favorite wins 70%", do favorites win ~70%?
struct CalibrationBucket {
    double low = 0.0;
    double high = 0.0;
    int count = 0;
    int favoriteWins = 0;
    double sumConfidence = 0.0;

    double avgConfidence() const;
    double actualRate() const;
};

struct BacktestResult {
    int evaluated = 0;   // fights scored
    int decided = 0;     // evaluated minus exact 50/50 coin flips
    int correct = 0;     // decided fights where the favorite won
    int coinFlips = 0;   // e.g. two debutants: the model has no opinion
    double logLoss = 0.0;  // mean over evaluated fights (coin-flip baseline: 0.693)
    double brier = 0.0;    // mean over evaluated fights (coin-flip baseline: 0.250)
    std::vector<CalibrationBucket> calibration;

    double accuracy() const {
        return decided > 0 ? static_cast<double>(correct) / decided : 0.0;
    }
};

// Walk-forward evaluation: each fight is predicted using ONLY ratings built from
// earlier fights, then the fight is applied. Fights must be chronological.
BacktestResult runBacktest(const std::vector<Fight>& fights,
                           const EloConfig& config = {},
                           const BacktestOptions& options = {});

struct SweepRow {
    double baseK;
    BacktestResult result;
};

// Runs one backtest per candidate base K (all other settings from `base`).
std::vector<SweepRow> sweepBaseK(const std::vector<Fight>& fights,
                                 const EloConfig& base,
                                 const std::vector<double>& candidates,
                                 const BacktestOptions& options = {});

void printBacktestReport(std::ostream& out, const std::string& title,
                         const BacktestResult& result, bool showCalibration = true);

// Full report used by the CLI: overall, established fighters, and a
// walk-forward tuning experiment (tune on first 70%, test on last 30%).
void runBacktestReport(std::ostream& out, const std::vector<Fight>& fights,
                       const EloConfig& config);

} // namespace ufc
