#include "TestFramework.hpp"
#include "ufc/Leaderboard.hpp"
#include "ufc/RatingEngine.hpp"

using namespace ufc;

static Fight fight(const std::string& date, const std::string& w, const std::string& l,
                   FinishMethod m = FinishMethod::UnanimousDecision) {
    return Fight{date, w, l, m};
}

TEST(engine_new_fighters_start_at_baseline) {
    RatingEngine engine;
    engine.processFight(fight("2024-01-01", "Alpha", "Bravo"));
    CHECK(engine.find("Alpha") != nullptr);
    CHECK_EQ(engine.fighterCount(), 2u);
}

TEST(engine_updates_are_zero_sum) {
    RatingEngine engine;
    const std::vector<Fight> fights = {
        fight("2024-01-01", "A", "B", FinishMethod::KO_TKO),
        fight("2024-02-01", "C", "A", FinishMethod::SplitDecision),
        fight("2024-03-01", "B", "C", FinishMethod::Submission),
        fight("2024-04-01", "A", "C"),
        fight("2024-05-01", "D", "A", FinishMethod::KO_TKO),
    };
    engine.processAll(fights);

    double total = 0.0;
    for (const auto& entry : engine.fighters()) total += entry.second.rating;
    CHECK_NEAR(total, 4 * 1500.0, 1e-9);
}

TEST(engine_tracks_win_loss_records) {
    RatingEngine engine;
    engine.processFight(fight("2024-01-01", "A", "B"));
    engine.processFight(fight("2024-02-01", "A", "B"));
    engine.processFight(fight("2024-03-01", "B", "A"));
    CHECK_EQ(engine.find("A")->wins, 2);
    CHECK_EQ(engine.find("A")->losses, 1);
    CHECK_EQ(engine.find("B")->wins, 1);
    CHECK_EQ(engine.find("B")->losses, 2);
}

TEST(engine_winner_gains_and_loser_loses) {
    RatingEngine engine;
    engine.processFight(fight("2024-01-01", "A", "B"));
    CHECK(engine.find("A")->rating > 1500.0);
    CHECK(engine.find("B")->rating < 1500.0);
}

TEST(engine_records_history_for_both_fighters) {
    RatingEngine engine;
    engine.processFight(fight("2024-01-01", "A", "B", FinishMethod::KO_TKO));
    engine.processFight(fight("2024-02-01", "B", "A"));

    const Fighter* a = engine.find("A");
    CHECK_EQ(a->history.size(), 2u);
    CHECK_EQ(a->history[0].opponent, std::string("B"));
    CHECK(a->history[0].won);
    CHECK(!a->history[1].won);
    CHECK_NEAR(a->history[0].ratingBefore, 1500.0, 1e-12);
    CHECK_NEAR(a->history[0].ratingAfter, a->history[1].ratingBefore, 1e-12);
    CHECK_NEAR(a->history.back().ratingAfter, a->rating, 1e-12);
}

TEST(engine_lookup_is_case_insensitive_and_merges_names) {
    RatingEngine engine;
    engine.processFight(fight("2024-01-01", "Marcus Vane", "Other"));
    engine.processFight(fight("2024-02-01", "marcus vane", "Other"));
    CHECK_EQ(engine.fighterCount(), 2u);
    CHECK(engine.find("MARCUS VANE") != nullptr);
    CHECK_EQ(engine.find("MARCUS VANE")->wins, 2);
    CHECK(engine.find("Nobody") == nullptr);
}

TEST(engine_search_substring_exact_and_empty) {
    RatingEngine engine;
    engine.processFight(fight("2024-01-01", "Marcus Vane", "Marcus Cole"));
    engine.processFight(fight("2024-02-01", "Ivan Petrov", "Marcus Cole"));

    const auto marcus = engine.search("marcus");
    CHECK_EQ(marcus.size(), 2u);
    CHECK_EQ(marcus[0]->name, std::string("Marcus Cole"));  // sorted by name
    CHECK_EQ(marcus[1]->name, std::string("Marcus Vane"));

    CHECK_EQ(engine.search("marcus vane").size(), 1u);  // exact match wins
    CHECK_EQ(engine.search("ivan p").size(), 1u);
    CHECK(engine.search("").empty());
    CHECK(engine.search("zzz").empty());
}

TEST(leaderboard_is_sorted_descending_with_name_tiebreak) {
    RatingEngine engine;
    engine.processFight(fight("2024-01-01", "A", "B"));
    engine.processFight(fight("2024-02-01", "A", "C"));
    engine.processFight(fight("2024-03-01", "B", "C"));

    const auto ranked = buildLeaderboard(engine);
    CHECK_EQ(ranked.size(), 3u);
    for (std::size_t i = 1; i < ranked.size(); ++i) {
        CHECK(ranked[i - 1].rating >= ranked[i].rating);
    }
    CHECK_EQ(ranked.front().name, std::string("A"));
}
