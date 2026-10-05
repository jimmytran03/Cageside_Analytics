#include "TestFramework.hpp"
#include "ufc/Exporter.hpp"
#include "ufc/Leaderboard.hpp"
#include "ufc/RatingEngine.hpp"
#include <filesystem>
#include <fstream>

using namespace ufc;
namespace fs = std::filesystem;

static const char* kDir = "test_tmp_exports";

static void cleanup() {
    std::error_code ec;
    fs::remove_all(kDir, ec);
}

static std::vector<std::string> readLines(const std::string& path) {
    std::ifstream in(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    return lines;
}

static std::size_t commas(const std::string& s) {
    std::size_t n = 0;
    for (char c : s) n += (c == ',');
    return n;
}

static RatingEngine smallLeague() {
    RatingEngine engine;
    engine.processFight(Fight{"2024-01-01", "Alpha", "Bravo", FinishMethod::KO_TKO});
    engine.processFight(Fight{"2024-02-01", "Alpha", "Cole", FinishMethod::UnanimousDecision});
    engine.processFight(Fight{"2024-03-01", "Bravo", "Alpha", FinishMethod::Submission});
    return engine;
}

TEST(export_csv_escape) {
    CHECK_EQ(csvEscape("plain"), std::string("plain"));
    CHECK_EQ(csvEscape("Smith, Bob"), std::string("\"Smith, Bob\""));
    CHECK_EQ(csvEscape("say \"hi\""), std::string("\"say \"\"hi\"\"\""));
    CHECK_EQ(csvEscape(""), std::string(""));
}

TEST(export_sanitize_filename) {
    CHECK_EQ(sanitizeFilename("Jon 'Bones' Jones"), std::string("jon_bones_jones"));
    CHECK_EQ(sanitizeFilename("  Max   Holloway "), std::string("max_holloway"));
    CHECK_EQ(sanitizeFilename("../../etc/passwd"), std::string("etc_passwd"));
    CHECK_EQ(sanitizeFilename("!!!"), std::string("fighter"));
    CHECK_EQ(sanitizeFilename(""), std::string("fighter"));
}

TEST(export_leaderboard_contents_and_parent_dirs_created) {
    cleanup();
    const RatingEngine engine = smallLeague();
    const auto ranked = buildLeaderboard(engine);
    const std::string path = std::string(kDir) + "/nested/lb.csv";

    exportLeaderboardCsv(path, ranked);

    const auto lines = readLines(path);
    CHECK_EQ(lines.size(), ranked.size() + 1);
    CHECK_EQ(lines[0], std::string("rank,fighter,elo,wins,losses,fights,win_rate_pct"));
    CHECK(lines[1].rfind("1," + ranked[0].name + ",", 0) == 0);   // rank 1 first
    for (const auto& line : lines) CHECK_EQ(commas(line), 6u);
    cleanup();
}

TEST(export_leaderboard_overwrites_existing_file) {
    cleanup();
    const RatingEngine engine = smallLeague();
    const auto ranked = buildLeaderboard(engine);
    const std::string path = std::string(kDir) + "/lb.csv";

    exportLeaderboardCsv(path, ranked);
    exportLeaderboardCsv(path, {ranked.front()});
    CHECK_EQ(readLines(path).size(), 2u);   // header + 1 fighter, nothing left over
    cleanup();
}

TEST(export_fighter_history_contents) {
    cleanup();
    const RatingEngine engine = smallLeague();
    const Fighter* alpha = engine.find("Alpha");
    const std::string path = std::string(kDir) + "/alpha.csv";

    exportFighterHistoryCsv(path, *alpha);

    const auto lines = readLines(path);
    CHECK_EQ(lines.size(), 4u);   // header + 3 fights
    CHECK_EQ(lines[0], std::string("fight_number,date,opponent,result,method,rating_before,rating_after,change"));
    CHECK(lines[1].find("2024-01-01,Bravo,W,KO/TKO,1500.00,1524.00,24.00") != std::string::npos);
    CHECK(lines[3].find(",L,Submission,") != std::string::npos);
    for (const auto& line : lines) CHECK_EQ(commas(line), 7u);
    cleanup();
}

TEST(export_names_with_commas_are_quoted) {
    cleanup();
    RatingEngine engine;
    engine.processFight(Fight{"2024-01-01", "Smith, Bob", "Other", FinishMethod::KO_TKO});
    const std::string path = std::string(kDir) + "/quoted.csv";
    exportLeaderboardCsv(path, buildLeaderboard(engine));
    const auto lines = readLines(path);
    CHECK(lines[1].find("\"Smith, Bob\"") != std::string::npos);
    cleanup();
}

TEST(export_empty_history_writes_header_only) {
    cleanup();
    Fighter nobody;
    nobody.name = "Nobody";
    const std::string path = std::string(kDir) + "/empty.csv";
    exportFighterHistoryCsv(path, nobody);
    CHECK_EQ(readLines(path).size(), 1u);
    cleanup();
}

TEST(export_text_file_round_trips) {
    cleanup();
    const std::string path = std::string(kDir) + "/report.txt";
    writeTextFile(path, "line one\nline two\n");
    const auto lines = readLines(path);
    CHECK_EQ(lines.size(), 2u);
    CHECK_EQ(lines[1], std::string("line two"));
    cleanup();
}

TEST(export_unwritable_path_throws) {
    CHECK_THROWS(writeTextFile("/dev/null/cannot/exist.txt", "x"));
    CHECK_THROWS(exportLeaderboardCsv("/dev/null/cannot/exist.csv", {}));
}
