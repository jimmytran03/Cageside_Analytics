#include "TestFramework.hpp"
#include "ufc/CsvLoader.hpp"
#include <cstdio>
#include <fstream>

using namespace ufc;

static const char* kPath = "test_tmp_fights.csv";

static void writeFile(const std::string& content) {
    std::ofstream out(kPath, std::ios::binary);
    out << content;
}

TEST(csv_loads_and_sorts_by_date) {
    writeFile("date,winner,loser,method\n2024-02-01,B,C,UD\n2023-01-01,A,B,KO/TKO\n");
    const auto fights = loadFightsFromCsv(kPath);
    CHECK_EQ(fights.size(), 2u);
    CHECK_EQ(fights[0].date, std::string("2023-01-01"));
    CHECK_EQ(fights[0].winner, std::string("A"));
    CHECK(fights[0].method == FinishMethod::KO_TKO);
    CHECK(fights[1].method == FinishMethod::UnanimousDecision);
    std::remove(kPath);
}

TEST(csv_trims_whitespace_and_skips_blank_lines) {
    writeFile("date,winner,loser,method\n\n  2024-01-01 , Alpha Fighter ,  Bravo , sub \n\n");
    const auto fights = loadFightsFromCsv(kPath);
    CHECK_EQ(fights.size(), 1u);
    CHECK_EQ(fights[0].winner, std::string("Alpha Fighter"));
    CHECK(fights[0].method == FinishMethod::Submission);
    std::remove(kPath);
}

TEST(csv_accepts_method_aliases) {
    CHECK(parseFinishMethod("KO").value() == FinishMethod::KO_TKO);
    CHECK(parseFinishMethod("tko").value() == FinishMethod::KO_TKO);
    CHECK(parseFinishMethod("Unanimous Decision").value() == FinishMethod::UnanimousDecision);
    CHECK(parseFinishMethod("sd").value() == FinishMethod::SplitDecision);
    CHECK(!parseFinishMethod("Draw").has_value());
    CHECK(!parseFinishMethod("").has_value());
}

TEST(csv_missing_file_throws) {
    CHECK_THROWS(loadFightsFromCsv("this_file_does_not_exist.csv"));
}

TEST(csv_bad_column_count_reports_line_number) {
    writeFile("date,winner,loser,method\n2024-01-01,A,B,UD\n2024-02-01,A,B\n");
    std::string message;
    try {
        loadFightsFromCsv(kPath);
    } catch (const std::exception& e) {
        message = e.what();
    }
    CHECK(message.find("Line 3") != std::string::npos);
    std::remove(kPath);
}

TEST(csv_unknown_method_throws) {
    writeFile("date,winner,loser,method\n2024-01-01,A,B,Draw\n");
    CHECK_THROWS(loadFightsFromCsv(kPath));
    std::remove(kPath);
}

TEST(csv_empty_fighter_name_throws) {
    writeFile("date,winner,loser,method\n2024-01-01,,B,UD\n");
    CHECK_THROWS(loadFightsFromCsv(kPath));
    std::remove(kPath);
}

TEST(csv_append_round_trips) {
    writeFile("date,winner,loser,method\n2024-01-01,A,B,UD");  // note: no trailing newline
    appendFightToCsv(kPath, Fight{"2024-06-01", "C", "D", FinishMethod::SplitDecision});
    const auto fights = loadFightsFromCsv(kPath);
    CHECK_EQ(fights.size(), 2u);
    CHECK_EQ(fights[1].winner, std::string("C"));
    CHECK(fights[1].method == FinishMethod::SplitDecision);
    std::remove(kPath);
}

TEST(csv_toString_round_trips_through_parser) {
    const FinishMethod all[] = {FinishMethod::KO_TKO, FinishMethod::Submission,
                                FinishMethod::UnanimousDecision, FinishMethod::SplitDecision};
    for (FinishMethod m : all) {
        CHECK(parseFinishMethod(toString(m)).value() == m);
    }
}
