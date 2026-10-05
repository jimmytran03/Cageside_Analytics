#include "TestFramework.hpp"
#include "ufc/Backtest.hpp"
#include <cmath>
#include <cstdint>
#include <random>

using namespace ufc;

static Fight fight(const std::string& date, const std::string& w, const std::string& l,
                   FinishMethod m = FinishMethod::KO_TKO) {
    return Fight{date, w, l, m};
}

// Deterministic synthetic league with KNOWN hidden skills. Uses only raw
// mt19937 output (standardized across compilers), so results are reproducible.
static std::vector<Fight> syntheticLeague(int fighters, int fights) {
    std::mt19937 rng(12345);
    auto uniform = [&rng]() { return rng() / 4294967296.0; };

    std::vector<double> skill(fighters);
    for (double& s : skill) s = (uniform() - 0.5) * 600.0;   // +/- 300 Elo points

    std::vector<Fight> out;
    for (int i = 0; i < fights; ++i) {
        int a = static_cast<int>(uniform() * fighters);
        int b = static_cast<int>(uniform() * fighters);
        if (a == b) b = (b + 1) % fighters;
        const double pA = 1.0 / (1.0 + std::pow(10.0, (skill[b] - skill[a]) / 400.0));
        const bool aWins = uniform() < pA;

        std::string key = std::to_string(i);
        key.insert(0, 8 - key.size(), '0');  // zero-padded so it sorts chronologically
        out.push_back(fight(key,
                            "F" + std::to_string(aWins ? a : b),
                            "F" + std::to_string(aWins ? b : a),
                            FinishMethod::UnanimousDecision));
    }
    return out;
}

TEST(backtest_first_fight_is_a_coin_flip) {
    const auto r = runBacktest({fight("2024-01-01", "A", "B")});
    CHECK_EQ(r.evaluated, 1);
    CHECK_EQ(r.coinFlips, 1);
    CHECK_EQ(r.decided, 0);
    CHECK_NEAR(r.logLoss, std::log(2.0), 1e-9);
    CHECK_NEAR(r.brier, 0.25, 1e-9);
}

TEST(backtest_uses_only_past_information) {
    // A beats B twice. Fight 2 is predicted from fight 1's result, not its own.
    const std::vector<Fight> fights = {fight("2024-01-01", "A", "B"), fight("2024-02-01", "A", "B")};
    const auto r = runBacktest(fights);
    CHECK_EQ(r.evaluated, 2);
    CHECK_EQ(r.decided, 1);     // fight 1 was a coin flip
    CHECK_EQ(r.correct, 1);     // fight 2: model favored A, A won
    // Fight 2 probability: A=1524 vs B=1476
    const double p = 1.0 / (1.0 + std::pow(10.0, (1476.0 - 1524.0) / 400.0));
    CHECK_NEAR(r.logLoss, (std::log(2.0) - std::log(p)) / 2.0, 1e-9);
}

TEST(backtest_upset_counts_as_incorrect) {
    const std::vector<Fight> fights = {fight("2024-01-01", "A", "B"), fight("2024-02-01", "B", "A")};
    const auto r = runBacktest(fights);
    CHECK_EQ(r.decided, 1);
    CHECK_EQ(r.correct, 0);
    CHECK_NEAR(r.accuracy(), 0.0, 1e-12);
}

TEST(backtest_window_limits_scored_fights) {
    const std::vector<Fight> fights = {fight("2024-01-01", "A", "B"), fight("2024-02-01", "A", "B"),
                                       fight("2024-03-01", "A", "B")};
    BacktestOptions opts;
    opts.evalFrom = 1;
    CHECK_EQ(runBacktest(fights, {}, opts).evaluated, 2);
    opts.evalTo = 2;
    CHECK_EQ(runBacktest(fights, {}, opts).evaluated, 1);
}

TEST(backtest_min_prior_fights_filters_debutants) {
    const std::vector<Fight> fights = {fight("2024-01-01", "A", "B"), fight("2024-02-01", "A", "B"),
                                       fight("2024-03-01", "C", "A")};
    BacktestOptions opts;
    opts.minPriorFights = 1;
    CHECK_EQ(runBacktest(fights, {}, opts).evaluated, 1);   // only fight 2: both have 1 prior fight
}

TEST(backtest_empty_input_is_safe) {
    const auto r = runBacktest({});
    CHECK_EQ(r.evaluated, 0);
    CHECK_NEAR(r.accuracy(), 0.0, 1e-12);
    CHECK_NEAR(r.logLoss, 0.0, 1e-12);
}

TEST(backtest_calibration_counts_add_up) {
    const auto fights = syntheticLeague(20, 400);
    const auto r = runBacktest(fights);
    int total = 0;
    int wins = 0;
    for (const auto& b : r.calibration) { total += b.count; wins += b.favoriteWins; }
    CHECK_EQ(total, r.decided);
    CHECK_EQ(wins, r.correct);
    CHECK_EQ(r.evaluated, r.decided + r.coinFlips);
}

TEST(backtest_beats_chance_on_data_with_real_skill_differences) {
    // Hidden skills exist, so a working Elo implementation must beat a coin flip.
    const auto fights = syntheticLeague(20, 500);
    const auto r = runBacktest(fights);
    CHECK(r.accuracy() > 0.60);
    CHECK(r.logLoss < std::log(2.0));
    CHECK(r.brier < 0.25);
}

TEST(backtest_on_pure_noise_does_not_beat_chance_by_much) {
    // Sanity check against leakage: with random winners there is nothing to learn.
    std::mt19937 rng(777);
    std::vector<Fight> fights;
    for (int i = 0; i < 600; ++i) {
        const int a = static_cast<int>(rng() % 20);
        int b = static_cast<int>(rng() % 20);
        if (a == b) b = (b + 1) % 20;
        const bool aWins = (rng() & 1u) != 0;
        std::string key = std::to_string(i);
        key.insert(0, 8 - key.size(), '0');
        fights.push_back(fight(key, "F" + std::to_string(aWins ? a : b), "F" + std::to_string(aWins ? b : a)));
    }
    const auto r = runBacktest(fights);
    CHECK(r.accuracy() < 0.58);                 // no free lunch
    CHECK(r.logLoss > std::log(2.0) - 0.02);    // not meaningfully better than guessing
}

TEST(backtest_sweep_returns_one_row_per_candidate) {
    const auto fights = syntheticLeague(10, 100);
    const auto rows = sweepBaseK(fights, EloConfig{}, {16.0, 32.0, 64.0});
    CHECK_EQ(rows.size(), 3u);
    CHECK_NEAR(rows[0].baseK, 16.0, 1e-12);
    CHECK_NEAR(rows[2].baseK, 64.0, 1e-12);
    CHECK_EQ(rows[1].result.evaluated, 100);
}
