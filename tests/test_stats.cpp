#include "TestFramework.hpp"
#include "ufc/FightStats.hpp"

using namespace ufc;

static FighterStats valid() {
    FighterStats s;
    s.sigLanded = 20;  s.sigAttempted = 40;
    s.totalLanded = 30; s.totalAttempted = 60;
    s.takedownsLanded = 1; s.takedownsAttempted = 2;
    s.controlSeconds = 120;
    return s;
}

TEST(stats_valid_stats_pass) {
    CHECK(!validateStats(valid()).has_value());
}

TEST(stats_landed_cannot_exceed_attempted) {
    FighterStats s = valid(); s.sigLanded = 41;
    CHECK(validateStats(s).has_value());
    s = valid(); s.totalLanded = 61;
    CHECK(validateStats(s).has_value());
    s = valid(); s.takedownsLanded = 3;
    CHECK(validateStats(s).has_value());
}

TEST(stats_sig_strikes_are_subset_of_total) {
    FighterStats s = valid(); s.sigLanded = 35;  // more than total landed (30)
    CHECK(validateStats(s).has_value());
    s = valid(); s.sigAttempted = 70;            // more than total attempted (60)
    CHECK(validateStats(s).has_value());
}

TEST(stats_control_time_capped_at_25_minutes) {
    FighterStats s = valid(); s.controlSeconds = 1500;
    CHECK(!validateStats(s).has_value());
    s.controlSeconds = 1501;
    CHECK(validateStats(s).has_value());
}

TEST(stats_parse_control_time) {
    CHECK_EQ(parseControlTime("3:45").value(), 225);
    CHECK_EQ(parseControlTime("0:05").value(), 5);
    CHECK_EQ(parseControlTime("225").value(), 225);
    CHECK_EQ(parseControlTime("0").value(), 0);
}

TEST(stats_parse_control_time_rejects_garbage) {
    CHECK(!parseControlTime("").has_value());
    CHECK(!parseControlTime("abc").has_value());
    CHECK(!parseControlTime("3:75").has_value());   // seconds must be < 60
    CHECK(!parseControlTime("3:").has_value());
    CHECK(!parseControlTime(":30").has_value());
    CHECK(!parseControlTime("-5").has_value());
    CHECK(!parseControlTime("1:2:3").has_value());
}

TEST(stats_format_control_time) {
    CHECK_EQ(formatControlTime(225), std::string("3:45"));
    CHECK_EQ(formatControlTime(5), std::string("0:05"));
    CHECK_EQ(formatControlTime(0), std::string("0:00"));
}
