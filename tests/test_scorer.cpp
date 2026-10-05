#include "TestFramework.hpp"
#include "ufc/FightScorer.hpp"

using namespace ufc;

static FighterStats make(int kd, int sigL, int sigA, int totL, int totA,
                         int tdL, int tdA, int sub, int control) {
    FighterStats s;
    s.knockdowns = kd;
    s.sigLanded = sigL;  s.sigAttempted = sigA;
    s.totalLanded = totL; s.totalAttempted = totA;
    s.takedownsLanded = tdL; s.takedownsAttempted = tdA;
    s.submissionAttempts = sub;
    s.controlSeconds = control;
    return s;
}

TEST(scorer_dominant_fighter_wins_unanimously) {
    const FighterStats a = make(1, 40, 80, 50, 90, 2, 3, 1, 240);
    const FighterStats b = make(0, 15, 50, 20, 60, 0, 1, 0, 20);
    const FightVerdict v = FightScorer{}.judge(a, b);
    CHECK(v.winner == Winner::FighterA);
    CHECK(v.method == FinishMethod::UnanimousDecision);
    CHECK(v.margin > 0.10);
}

TEST(scorer_is_symmetric_when_fighters_are_swapped) {
    const FighterStats a = make(1, 40, 80, 50, 90, 2, 3, 1, 240);
    const FighterStats b = make(0, 15, 50, 20, 60, 0, 1, 0, 20);
    const FightScorer scorer;
    CHECK(scorer.judge(a, b).winner == Winner::FighterA);
    CHECK(scorer.judge(b, a).winner == Winner::FighterB);
    CHECK_NEAR(scorer.judge(a, b).margin, scorer.judge(b, a).margin, 1e-12);
}

TEST(scorer_close_fight_is_scored_as_split_decision) {
    const FighterStats a = make(0, 30, 60, 35, 70, 0, 0, 0, 0);
    const FighterStats b = make(0, 29, 60, 35, 70, 0, 0, 0, 0);
    const FightVerdict v = FightScorer{}.judge(a, b);
    CHECK(v.winner == Winner::FighterA);
    CHECK(v.method == FinishMethod::SplitDecision);
    CHECK(v.margin < 0.10);
}

TEST(scorer_identical_stats_are_a_draw) {
    const FighterStats a = make(0, 20, 40, 30, 60, 1, 2, 0, 100);
    const FightVerdict v = FightScorer{}.judge(a, a);
    CHECK(v.winner == Winner::Draw);
    CHECK_NEAR(v.margin, 0.0, 1e-12);
}

TEST(scorer_ties_are_broken_by_knockdowns) {
    // A: 1 knockdown = 10 pts.  B: 20 sig strikes = 10 pts.  Equal totals.
    const FighterStats a = make(1, 0, 0, 0, 0, 0, 0, 0, 0);
    const FighterStats b = make(0, 20, 20, 20, 20, 0, 0, 0, 0);
    const FightScorer scorer;
    CHECK_NEAR(scorer.score(a).total(), scorer.score(b).total(), 1e-9);
    CHECK(scorer.judge(a, b).winner == Winner::FighterA);
}

TEST(scorer_breakdown_adds_up) {
    const FighterStats a = make(2, 10, 20, 15, 30, 1, 2, 1, 120);
    const ScoreBreakdown s = FightScorer{}.score(a);
    CHECK_NEAR(s.knockdowns, 20.0, 1e-9);                 // 2 * 10
    CHECK_NEAR(s.striking, 10 * 0.5 + 5 * 0.1, 1e-9);     // sig + other strikes
    CHECK_NEAR(s.grappling, 1 * 4.0 + 1 * 2.0, 1e-9);     // takedown + sub attempt
    CHECK_NEAR(s.control, 2.0 * 1.5, 1e-9);               // 2 minutes
    CHECK_NEAR(s.total(), s.knockdowns + s.striking + s.grappling + s.control, 1e-12);
}

TEST(scorer_more_of_any_stat_never_hurts) {
    const FighterStats base = make(0, 10, 20, 15, 30, 1, 2, 0, 60);
    FighterStats more = base;
    more.knockdowns += 1;
    more.sigLanded += 5; more.sigAttempted += 5; more.totalLanded += 5; more.totalAttempted += 5;
    more.takedownsLanded += 1; more.takedownsAttempted += 1;
    more.submissionAttempts += 1;
    more.controlSeconds += 30;
    const FightScorer scorer;
    CHECK(scorer.score(more).total() > scorer.score(base).total());
}
