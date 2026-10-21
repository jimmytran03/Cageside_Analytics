#include "ufc/Backtest.hpp"
#include "ufc/RatingEngine.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace ufc
{
    namespace
    {
        constexpr int kBuckets{ 5 };  // 50-60, 60-70, 70-80, 80-90, 90-100
    }

    double CalibrationBucket::avgConfidence() const
    {
        return (count > 0) ? sumConfidence / count : 0.0;
    }

    double CalibrationBucket::actualRate() const
    {
        return (count > 0) ? static_cast<double>(favoriteWins) / count : 0.0;
    }

    BacktestResult runBacktest(const std::vector<Fight>& fights, const EloConfig& config,
                               const BacktestOptions& options)
    {
        BacktestResult result{};
        result.calibration.resize(kBuckets);
        for (int i{ 0 }; i < kBuckets; ++i)
        {
            result.calibration[i].low = 0.5 + 0.1 * i;
            result.calibration[i].high = 0.6 + 0.1 * i;
        }

        RatingEngine engine{ EloCalculator(config) };
        double logLossSum{ 0.0 };
        double brierSum{ 0.0 };

        for (std::size_t i{ 0 }; i < fights.size(); ++i)
        {
            const Fight& fight{ fights[i] };

            // 1) Predict using only what was known BEFORE this fight.
            const Fighter* w{ engine.find(fight.winner) };
            const Fighter* l{ engine.find(fight.loser) };
            const double ratingW{ w ? w->rating : config.startRating };
            const double ratingL{ l ? l->rating : config.startRating };
            const int priorW{ w ? w->wins + w->losses : 0 };
            const int priorL{ l ? l->wins + l->losses : 0 };

            const bool inWindow{ i >= options.evalFrom && i < options.evalTo };
            if (inWindow && priorW >= options.minPriorFights && priorL >= options.minPriorFights)
            {
                const double p{ EloCalculator::expectedScore(ratingW, ratingL) };  // P(actual winner wins)
                ++result.evaluated;
                logLossSum += -std::log(std::clamp(p, 1e-12, 1.0));
                brierSum += (1.0 - p) * (1.0 - p);

                if (std::fabs(p - 0.5) < 1e-12)
                {
                    ++result.coinFlips;
                }
                else
                {
                    ++result.decided;
                    const bool favoriteWon{ p > 0.5 };
                    if (favoriteWon)
                    {
                        ++result.correct;
                    }

                    const double confidence{ std::max(p, 1.0 - p) };
                    const int idx{ std::min(static_cast<int>((confidence - 0.5) * 10.0), kBuckets - 1) };
                    CalibrationBucket& b{ result.calibration[idx] };
                    ++b.count;
                    b.sumConfidence += confidence;
                    if (favoriteWon)
                    {
                        ++b.favoriteWins;
                    }
                }
            }

            // 2) Only now reveal the result to the model.
            engine.processFight(fight);
        }

        if (result.evaluated > 0)
        {
            result.logLoss = logLossSum / result.evaluated;
            result.brier = brierSum / result.evaluated;
        }

        return result;
    }

    std::vector<SweepRow> sweepBaseK(const std::vector<Fight>& fights, const EloConfig& base,
                                     const std::vector<double>& candidates,
                                     const BacktestOptions& options)
    {
        std::vector<SweepRow> rows{};
        rows.reserve(candidates.size());
        for (double k : candidates)
        {
            EloConfig config{ base };
            config.baseK = k;
            rows.push_back({ k, runBacktest(fights, config, options) });
        }

        return rows;
    }

    // ---------- reporting ----------

    namespace
    {
        std::string pct(double fraction)
        {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << fraction * 100.0 << "%";

            return ss.str();
        }

        void tableHeader(std::ostream& out, const std::string& firstColumn)
        {
            out << "  " << std::left << std::setw(34) << firstColumn << std::right
                << std::setw(8) << "Fights" << std::setw(10) << "Accuracy"
                << std::setw(10) << "LogLoss" << std::setw(8) << "Brier" << '\n';
        }

        void tableRow(std::ostream& out, const std::string& label, const BacktestResult& r)
        {
            out << "  " << std::left << std::setw(34) << label << std::right
                << std::setw(8) << r.evaluated << std::setw(10) << pct(r.accuracy())
                << std::setw(10) << std::fixed << std::setprecision(3) << r.logLoss
                << std::setw(8) << r.brier << '\n';
        }

    } // namespace

    void printBacktestReport(std::ostream& out, const std::string& title,
                             const BacktestResult& r, bool showCalibration)
    {
        const std::string rule(64, '-');
        out << '\n' << rule << '\n' << title << '\n' << rule << '\n';

        if (r.evaluated == 0)
        {
            out << "No fights matched the evaluation window.\n";

            return;
        }

        out << std::fixed << std::setprecision(3)
            << "Fights scored : " << r.evaluated << "  (coin flips, no opinion: " << r.coinFlips << ")\n"
            << "Accuracy      : " << pct(r.accuracy()) << "  (" << r.correct << " of " << r.decided << ")\n"
            << "Log loss      : " << r.logLoss << "   (guessing 50/50 scores 0.693; lower is better)\n"
            << "Brier score   : " << r.brier << "   (guessing 50/50 scores 0.250; lower is better)\n";

        if (showCalibration)
        {
            out << "\nCalibration (favorite's predicted chance vs how often favorites won)\n"
                << "  " << std::left << std::setw(12) << "Confidence" << std::right
                << std::setw(8) << "Fights" << std::setw(12) << "Predicted" << std::setw(10) << "Actual" << '\n';
            for (const CalibrationBucket& b : r.calibration)
            {
                if (b.count == 0)
                {
                    continue;
                }

                std::ostringstream range;
                range << static_cast<int>(b.low * 100 + 0.5) << "-" << static_cast<int>(b.high * 100 + 0.5) << "%";
                out << "  " << std::left << std::setw(12) << range.str() << std::right
                    << std::setw(8) << b.count << std::setw(12) << pct(b.avgConfidence())
                    << std::setw(10) << pct(b.actualRate()) << '\n';
            }
        }
    }

    void runBacktestReport(std::ostream& out, const std::vector<Fight>& fights,
                           const EloConfig& config)
    {
        out << "\nBACKTEST: every fight is predicted using only ratings built from EARLIER fights.\n";
        if (fights.empty())
        {
            out << "No fights loaded.\n";

            return;
        }

        if (fights.size() < 100)
        {
            out << "Note: only " << fights.size()
                << " fights loaded. Results on a sample this small are not statistically meaningful.\n";
        }

        printBacktestReport(out, "All fights (K = " + std::to_string(static_cast<int>(config.baseK)) + ")",
                            runBacktest(fights, config));

        BacktestOptions established{};
        established.minPriorFights = 3;
        printBacktestReport(out, "Established fighters only (3+ earlier fights each)",
                            runBacktest(fights, config, established), false);

        // Walk-forward tuning: choose K on the first 70%, judge it on the last 30%.
        const std::size_t split{ fights.size() * 7 / 10 };
        const std::size_t testCount{ fights.size() - split };
        if (split < 20 || testCount < 10)
        {
            out << "\nSkipping tuning experiment: it needs roughly 30+ fights.\n";

            return;
        }

        const std::vector<double> candidates{ 8, 16, 24, 32, 40, 48, 64 };
        BacktestOptions train{};
        train.evalTo = split;
        const auto rows{ sweepBaseK(fights, config, candidates, train) };

        const auto best{ std::min_element(rows.begin(), rows.end(),
            [](const SweepRow& a, const SweepRow& b)
            {
                return a.result.logLoss < b.result.logLoss;
            }) };

        out << "\n" << std::string(64, '-') << "\nTUNING (walk-forward): choose K on the first 70% of fights ("
            << split << "), then test on the unseen last 30% (" << testCount << ")\n"
            << std::string(64, '-') << '\n';
        tableHeader(out, "Training window");
        for (const SweepRow& row : rows)
        {
            std::string label{ "K = " + std::to_string(static_cast<int>(row.baseK)) };
            if (&row == &*best)
            {
                label += "  <- best";
            }

            tableRow(out, label, row.result);
        }

        BacktestOptions test{};
        test.evalFrom = split;

        EloConfig tuned{ config };
        tuned.baseK = best->baseK;

        EloConfig flat{ config };  // same K, but every finish counts the same
        flat.splitMultiplier = 1.0;
        flat.decisionMultiplier = 1.0;
        flat.finishMultiplier = 1.0;

        out << "\nUnseen test window (nothing here was used to pick K):\n";
        tableHeader(out, "Setting");
        tableRow(out, "Default K = " + std::to_string(static_cast<int>(config.baseK)), runBacktest(fights, config, test));
        tableRow(out, "Tuned K = " + std::to_string(static_cast<int>(best->baseK)), runBacktest(fights, tuned, test));
        tableRow(out, "Default K, finish weighting OFF", runBacktest(fights, flat, test));
        out << "\nTip: if finish weighting OFF scores as well or better, the method\n"
               "multipliers aren't adding predictive value on this data.\n";
    }

} // namespace ufc