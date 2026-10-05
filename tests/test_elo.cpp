#include "TestFramework.hpp"
#include "ufc/EloCalculator.hpp"

using namespace ufc;

TEST(elo_equal_ratings_give_fifty_percent) {
    CHECK_NEAR(EloCalculator::expectedScore(1500, 1500), 0.5, 1e-12);
}

TEST(elo_1800_vs_1500_is_about_85_percent) {
    CHECK_NEAR(EloCalculator::expectedScore(1800, 1500), 0.849, 0.001);
}

TEST(elo_expected_scores_sum_to_one) {
    const double a = EloCalculator::expectedScore(1731, 1422);
    const double b = EloCalculator::expectedScore(1422, 1731);
    CHECK_NEAR(a + b, 1.0, 1e-12);
}

TEST(elo_higher_rating_means_higher_win_probability) {
    CHECK(EloCalculator::expectedScore(1600, 1500) > EloCalculator::expectedScore(1550, 1500));
}

TEST(elo_multipliers_match_spec) {
    const EloCalculator calc;
    CHECK_NEAR(calc.multiplierFor(FinishMethod::KO_TKO), 1.5, 1e-12);
    CHECK_NEAR(calc.multiplierFor(FinishMethod::Submission), 1.5, 1e-12);
    CHECK_NEAR(calc.multiplierFor(FinishMethod::UnanimousDecision), 1.0, 1e-12);
    CHECK_NEAR(calc.multiplierFor(FinishMethod::SplitDecision), 0.75, 1e-12);
}

TEST(elo_delta_for_equal_fighters_is_half_of_adjusted_k) {
    const EloCalculator calc;  // base K = 32
    CHECK_NEAR(calc.ratingDelta(1500, 1500, FinishMethod::KO_TKO), 24.0, 1e-9);
    CHECK_NEAR(calc.ratingDelta(1500, 1500, FinishMethod::UnanimousDecision), 16.0, 1e-9);
    CHECK_NEAR(calc.ratingDelta(1500, 1500, FinishMethod::SplitDecision), 12.0, 1e-9);
}

TEST(elo_finish_moves_ratings_more_than_decision) {
    const EloCalculator calc;
    const double ko = calc.ratingDelta(1500, 1500, FinishMethod::KO_TKO);
    const double ud = calc.ratingDelta(1500, 1500, FinishMethod::UnanimousDecision);
    const double sd = calc.ratingDelta(1500, 1500, FinishMethod::SplitDecision);
    CHECK(ko > ud);
    CHECK(ud > sd);
}

TEST(elo_upset_pays_far_more_than_expected_win) {
    const EloCalculator calc;
    const double upset = calc.ratingDelta(1500, 1800, FinishMethod::KO_TKO);
    const double expected = calc.ratingDelta(1800, 1500, FinishMethod::SplitDecision);
    CHECK_NEAR(upset, 40.75, 0.01);     // underdog KOs favorite
    CHECK_NEAR(expected, 3.62, 0.01);   // favorite wins narrowly: barely moves
    CHECK(upset > 10.0 * expected);
}

TEST(elo_delta_is_always_positive) {
    const EloCalculator calc;
    CHECK(calc.ratingDelta(2400, 1200, FinishMethod::SplitDecision) > 0.0);
    CHECK(calc.ratingDelta(1200, 2400, FinishMethod::KO_TKO) > 0.0);
}

TEST(elo_respects_custom_config) {
    EloConfig config;
    config.baseK = 20.0;
    config.finishMultiplier = 2.0;
    const EloCalculator calc(config);
    CHECK_NEAR(calc.ratingDelta(1500, 1500, FinishMethod::KO_TKO), 20.0, 1e-9);  // 20*2*0.5
}
