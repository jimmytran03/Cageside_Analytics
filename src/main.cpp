#include "ufc/Backtest.hpp"
#include "ufc/CsvLoader.hpp"
#include "ufc/Exporter.hpp"
#include "ufc/FightScorer.hpp"
#include "ufc/Leaderboard.hpp"
#include "ufc/Profile.hpp"
#include "ufc/RatingEngine.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace {

// ---------- small helpers ----------

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    return s.substr(b, s.find_last_not_of(ws) - b + 1);
}

std::string todayIso() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return buf;
}

bool isIsoDate(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

std::string askDate() {
    while (true) {
        std::cout << "Date (YYYY-MM-DD, blank = today): ";
        std::string d;
        if (!std::getline(std::cin, d)) return todayIso();
        d = trim(d);
        if (d.empty()) return todayIso();
        if (isIsoDate(d)) return d;
        std::cout << "Please use the format YYYY-MM-DD.\n";
    }
}

// Uses the database spelling of a name when the fighter already exists.
std::string canonicalName(const ufc::RatingEngine& engine, const std::string& name) {
    const ufc::Fighter* f = engine.find(name);
    return f ? f->name : name;
}

double ratingOf(const ufc::RatingEngine& engine, const std::string& name) {
    const ufc::Fighter* f = engine.find(name);
    return f ? f->rating : engine.calculator().config().startRating;
}

// True if the name is known, or the user agrees to add a new fighter.
bool confirmFighter(const ufc::RatingEngine& engine, const std::string& name) {
    if (engine.find(name)) return true;
    std::cout << "'" << name << "' is not in the database. Add as a new fighter at "
              << engine.calculator().config().startRating << "? (y/n): ";
    std::string answer;
    std::getline(std::cin, answer);
    return !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y');
}

bool askYesNo(const std::string& question) {
    std::cout << question << " (y/n): ";
    std::string answer;
    std::getline(std::cin, answer);
    return !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y');
}

void printMenu() {
    std::cout << "\n[1] Leaderboard\n"
              << "[2] Score a fight from stats\n"
              << "[3] Search fighter profile & Elo history\n"
              << "[4] Backtest model accuracy\n"
              << "[5] Export / save\n"
              << "[6] Quit\n"
              << "> ";
}

// ---------- applying a fight ----------

void commitFight(ufc::RatingEngine& engine, const ufc::Fight& fight,
                 const std::string& csvPath) {
    const double winnerBefore = ratingOf(engine, fight.winner);
    const double loserBefore  = ratingOf(engine, fight.loser);

    engine.processFight(fight);

    const ufc::Fighter* w = engine.find(fight.winner);
    const ufc::Fighter* l = engine.find(fight.loser);

    std::cout << std::fixed << std::setprecision(1)
              << "\nResult recorded (" << ufc::toString(fight.method) << ")\n"
              << "  " << w->name << ": " << winnerBefore << " -> " << w->rating
              << "  (" << std::showpos << (w->rating - winnerBefore) << std::noshowpos << ")\n"
              << "  " << l->name << ": " << loserBefore << " -> " << l->rating
              << "  (" << std::showpos << (l->rating - loserBefore) << std::noshowpos << ")\n";

    try {
        ufc::appendFightToCsv(csvPath, fight);
        std::cout << "Saved to " << csvPath << "\n";
    } catch (const std::exception& e) {
        std::cout << "Warning: rating updated, but the fight was not saved: " << e.what() << "\n";
    }
}

// ---------- menu actions ----------

// ---------- stats entry ----------

std::optional<int> promptInt(const std::string& label) {
    while (true) {
        std::cout << "  " << label << ": ";
        std::string line;
        if (!std::getline(std::cin, line)) return std::nullopt;
        line = trim(line);
        const bool ok = !line.empty() && line.size() <= 6 &&
                        std::all_of(line.begin(), line.end(),
                                    [](unsigned char c) { return std::isdigit(c) != 0; });
        if (ok) return std::stoi(line);
        std::cout << "  Please enter a whole number (0 or more).\n";
    }
}

std::optional<int> promptControl(const std::string& label) {
    while (true) {
        std::cout << "  " << label << " (m:ss or seconds): ";
        std::string line;
        if (!std::getline(std::cin, line)) return std::nullopt;
        if (const auto seconds = ufc::parseControlTime(trim(line))) return seconds;
        std::cout << "  Please enter a time like 3:45 or 225.\n";
    }
}

std::optional<ufc::FighterStats> readStats(const std::string& name) {
    while (true) {
        std::cout << "\nStats for " << name << ":\n";
        ufc::FighterStats s;

        auto ask = [](const std::string& label, int& field) {
            const auto v = promptInt(label);
            if (!v) return false;
            field = *v;
            return true;
        };

        const bool complete =
            ask("Knockdowns", s.knockdowns) &&
            ask("Significant strikes landed", s.sigLanded) &&
            ask("Significant strikes attempted", s.sigAttempted) &&
            ask("Total strikes landed", s.totalLanded) &&
            ask("Total strikes attempted", s.totalAttempted) &&
            ask("Takedowns landed", s.takedownsLanded) &&
            ask("Takedowns attempted", s.takedownsAttempted) &&
            ask("Submission attempts", s.submissionAttempts);
        if (!complete) return std::nullopt;

        const auto control = promptControl("Control time");
        if (!control) return std::nullopt;
        s.controlSeconds = *control;

        if (const auto error = ufc::validateStats(s)) {
            std::cout << "\nInvalid stats: " << *error << " Please re-enter.\n";
            continue;
        }
        return s;
    }
}

void scoreFromStats(ufc::RatingEngine& engine, const std::string& csvPath) {
    std::string nameA, nameB;
    std::cout << "Fighter A: ";
    std::getline(std::cin, nameA);
    std::cout << "Fighter B: ";
    std::getline(std::cin, nameB);
    nameA = trim(nameA);
    nameB = trim(nameB);

    if (nameA.empty() || nameB.empty()) {
        std::cout << "Names cannot be empty.\n";
        return;
    }
    if (lower(nameA) == lower(nameB)) {
        std::cout << "A fighter cannot fight themselves.\n";
        return;
    }
    nameA = canonicalName(engine, nameA);
    nameB = canonicalName(engine, nameB);

    const auto statsA = readStats(nameA);
    if (!statsA) return;
    const auto statsB = readStats(nameB);
    if (!statsB) return;

    const ufc::FightScorer scorer;
    const ufc::FightVerdict verdict = scorer.judge(*statsA, *statsB);
    ufc::printVerdict(std::cout, nameA, *statsA, nameB, *statsB, verdict);

    if (verdict.winner == ufc::Winner::Draw) {
        std::cout << "No winner, so nothing to record.\n";
        return;
    }
    if (!askYesNo("Record this result and update Elo?")) return;

    const bool aWon = (verdict.winner == ufc::Winner::FighterA);
    const std::string& winner = aWon ? nameA : nameB;
    const std::string& loser  = aWon ? nameB : nameA;

    if (!confirmFighter(engine, winner) || !confirmFighter(engine, loser)) {
        std::cout << "Cancelled.\n";
        return;
    }
    const std::string date = askDate();
    commitFight(engine, ufc::Fight{date, winner, loser, verdict.method}, csvPath);
}

// ---------- leaderboard ----------

void showLeaderboard(const ufc::RatingEngine& engine) {
    std::cout << "How many fighters? (blank = 20, 'all' = everyone): ";
    std::string answer;
    std::getline(std::cin, answer);
    answer = trim(answer);

    std::size_t limit = 20;  // 0 means "everyone"
    if (lower(answer) == "all") {
        limit = 0;
    } else if (!answer.empty()) {
        const bool digits = answer.size() <= 6 &&
                            std::all_of(answer.begin(), answer.end(),
                                        [](unsigned char c) { return std::isdigit(c) != 0; });
        if (!digits || std::stoi(answer) == 0) {
            std::cout << "Please enter a positive number or 'all'.\n";
            return;
        }
        limit = static_cast<std::size_t>(std::stoi(answer));
    }

    const auto ranked = ufc::buildLeaderboard(engine);
    ufc::printLeaderboard(std::cout, ranked, limit);
    if (limit != 0 && ranked.size() > limit) {
        std::cout << "(showing top " << limit << " of " << ranked.size() << " fighters)\n";
    }
}

// ---------- profile search ----------

// Asks for a name, handles multiple matches, and returns the chosen fighter
// (nullptr if nothing was found or the user cancelled).
const ufc::Fighter* pickFighter(const ufc::RatingEngine& engine) {
    std::cout << "Search fighter name: ";
    std::string query;
    std::getline(std::cin, query);
    query = trim(query);
    if (query.empty()) return nullptr;

    const auto matches = engine.search(query);
    if (matches.empty()) {
        std::cout << "No fighters found matching '" << query << "'.\n";
        return nullptr;
    }

    constexpr std::size_t kMaxListed = 15;
    if (matches.size() > kMaxListed) {
        std::cout << matches.size() << " fighters match '" << query
                  << "'. Please type more of the name.\n";
        return nullptr;
    }

    const ufc::Fighter* chosen = matches.front();
    if (matches.size() > 1) {
        std::cout << "\nMultiple matches:\n";
        for (std::size_t i = 0; i < matches.size(); ++i) {
            std::cout << "  [" << (i + 1) << "] " << matches[i]->name << '\n';
        }
        std::cout << "Select (1-" << matches.size() << ", blank to cancel): ";
        std::string pick;
        std::getline(std::cin, pick);
        pick = trim(pick);
        const bool digits = !pick.empty() && pick.size() <= 6 &&
                            std::all_of(pick.begin(), pick.end(),
                                        [](unsigned char c) { return std::isdigit(c) != 0; });
        if (!digits) return nullptr;
        const std::size_t index = static_cast<std::size_t>(std::stoi(pick));
        if (index < 1 || index > matches.size()) {
            std::cout << "Invalid selection.\n";
            return nullptr;
        }
        chosen = matches[index - 1];
    }

    return chosen;
}

void searchProfile(const ufc::RatingEngine& engine) {
    const ufc::Fighter* chosen = pickFighter(engine);
    if (!chosen) return;

    const auto ranked = ufc::buildLeaderboard(engine);
    std::size_t rank = 0;
    for (std::size_t i = 0; i < ranked.size(); ++i) {
        if (ranked[i].name == chosen->name) { rank = i + 1; break; }
    }
    ufc::printProfile(std::cout, *chosen, rank, ranked.size(),
                      engine.calculator().config().startRating);
}

// ---------- backtesting ----------

void runBacktestAction(const std::string& csvPath, const ufc::EloConfig& config) {
    try {
        // Reload from disk so fights recorded this session are included.
        const auto fights = ufc::loadFightsFromCsv(csvPath);
        ufc::runBacktestReport(std::cout, fights, config);
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
}

// ---------- export / save ----------

const char* const kExportDir = "exports";

// Asks for a file name. Blank = default name. A bare name goes inside exports/,
// and a missing extension is filled in from the default. Returns "" if cancelled.
std::string askExportPath(const std::string& defaultName) {
    std::cout << "File name (blank = " << kExportDir << "/" << defaultName << "): ";
    std::string name;
    std::getline(std::cin, name);
    name = trim(name);

    std::string path;
    if (name.empty()) {
        path = std::string(kExportDir) + "/" + defaultName;
    } else {
        if (name.find('.') == std::string::npos) {
            name += defaultName.substr(defaultName.find_last_of('.'));
        }
        const bool hasFolder = name.find('/') != std::string::npos ||
                               name.find('\\') != std::string::npos;
        path = hasFolder ? name : std::string(kExportDir) + "/" + name;
    }

    std::error_code ec;
    if (std::filesystem::exists(path, ec) &&
        !askYesNo("'" + path + "' already exists. Overwrite?")) {
        std::cout << "Cancelled.\n";
        return "";
    }
    return path;
}

void exportLeaderboardAction(const ufc::RatingEngine& engine) {
    const std::string path = askExportPath("leaderboard.csv");
    if (path.empty()) return;
    try {
        const auto ranked = ufc::buildLeaderboard(engine);
        ufc::exportLeaderboardCsv(path, ranked);
        std::cout << "Saved " << ranked.size() << " fighters to " << path << "\n";
    } catch (const std::exception& e) {
        std::cout << "Error: could not save: " << e.what() << "\n";
    }
}

void exportHistoryAction(const ufc::RatingEngine& engine) {
    const ufc::Fighter* fighter = pickFighter(engine);
    if (!fighter) return;
    const std::string path = askExportPath(ufc::sanitizeFilename(fighter->name) + "_elo_history.csv");
    if (path.empty()) return;
    try {
        ufc::exportFighterHistoryCsv(path, *fighter);
        std::cout << "Saved " << fighter->history.size() << " fights for " << fighter->name
                  << " to " << path << "\n";
    } catch (const std::exception& e) {
        std::cout << "Error: could not save: " << e.what() << "\n";
    }
}

void exportBacktestAction(const std::string& csvPath, const ufc::EloConfig& config) {
    const std::string path = askExportPath("backtest_report.txt");
    if (path.empty()) return;
    try {
        const auto fights = ufc::loadFightsFromCsv(csvPath);   // includes fights saved this session
        std::ostringstream report;
        ufc::runBacktestReport(report, fights, config);
        ufc::writeTextFile(path, report.str());
        std::cout << "Saved backtest report to " << path << "\n";
    } catch (const std::exception& e) {
        std::cout << "Error: could not save: " << e.what() << "\n";
    }
}

void exportMenu(const ufc::RatingEngine& engine, const std::string& csvPath) {
    std::cout << "\nExport / save:\n"
              << "  [1] Leaderboard (CSV)\n"
              << "  [2] A fighter's Elo history (CSV)\n"
              << "  [3] Backtest report (text file)\n"
              << "  [4] Back\n"
              << "> ";
    std::string choice;
    if (!std::getline(std::cin, choice)) return;
    choice = trim(choice);

    if (choice == "1") {
        exportLeaderboardAction(engine);
    } else if (choice == "2") {
        exportHistoryAction(engine);
    } else if (choice == "3") {
        exportBacktestAction(csvPath, engine.calculator().config());
    } else if (choice != "4") {
        std::cout << "Please choose 1, 2, 3 or 4.\n";
    }
}

} // namespace

int main(int argc, char* argv[]) {
    const std::string path = (argc > 1) ? argv[1] : "data/fights.csv";

    ufc::RatingEngine engine;
    try {
        const auto fights = ufc::loadFightsFromCsv(path);
        engine.processAll(fights);
        std::cout << "Processed " << fights.size() << " fights, "
                  << engine.fighterCount() << " fighters.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    std::string choice;
    while (true) {
        printMenu();
        if (!std::getline(std::cin, choice)) break;

        if (choice == "1") {
            showLeaderboard(engine);
        } else if (choice == "2") {
            scoreFromStats(engine, path);
        } else if (choice == "3") {
            searchProfile(engine);
        } else if (choice == "4") {
            runBacktestAction(path, engine.calculator().config());
        } else if (choice == "5") {
            exportMenu(engine, path);
        } else if (choice == "6") {
            break;
        } else {
            std::cout << "Please choose 1, 2, 3, 4, 5 or 6.\n";
        }
    }
    return 0;
}
