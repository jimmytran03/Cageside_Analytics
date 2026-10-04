#include "ufc/CsvLoader.hpp"
#include "ufc/Leaderboard.hpp"
#include "ufc/Predictor.hpp"
#include "ufc/RatingEngine.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string todayIso() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return buf;
}

void printMenu() {
    std::cout << "\n[1] Leaderboard\n"
              << "[2] Predict a matchup\n"
              << "[3] Record a fight result\n"
              << "[4] Quit\n"
              << "> ";
}

void runPredictor(const ufc::RatingEngine& engine) {
    std::string a, b;
    std::cout << "Fighter A: ";
    std::getline(std::cin, a);
    std::cout << "Fighter B: ";
    std::getline(std::cin, b);

    if (const auto prediction = ufc::predictMatchup(engine, a, b)) {
        ufc::printTaleOfTheTape(std::cout, *prediction);
    } else {
        std::cout << "One or both fighters were not found in the database.\n";
    }
}

// Returns true if the name is known, or the user agrees to add a new fighter.
bool confirmFighter(const ufc::RatingEngine& engine, const std::string& name) {
    if (engine.find(name)) return true;
    std::cout << "'" << name << "' is not in the database. Add as a new fighter at "
              << engine.calculator().config().startRating << "? (y/n): ";
    std::string answer;
    std::getline(std::cin, answer);
    return !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y');
}

double ratingOf(const ufc::RatingEngine& engine, const std::string& name) {
    const ufc::Fighter* f = engine.find(name);
    return f ? f->rating : engine.calculator().config().startRating;
}

void recordFight(ufc::RatingEngine& engine, const std::string& csvPath) {
    std::string winner, loser, date, methodChoice;

    std::cout << "Winner: ";
    std::getline(std::cin, winner);
    std::cout << "Loser: ";
    std::getline(std::cin, loser);

    if (winner.empty() || loser.empty()) {
        std::cout << "Names cannot be empty.\n";
        return;
    }
    if (lower(winner) == lower(loser)) {
        std::cout << "A fighter cannot fight themselves.\n";
        return;
    }
    if (!confirmFighter(engine, winner) || !confirmFighter(engine, loser)) {
        std::cout << "Cancelled.\n";
        return;
    }

    std::cout << "Method: [1] KO/TKO  [2] Submission  [3] Unanimous Decision  [4] Split Decision\n> ";
    std::getline(std::cin, methodChoice);
    ufc::FinishMethod method;
    if      (methodChoice == "1") method = ufc::FinishMethod::KO_TKO;
    else if (methodChoice == "2") method = ufc::FinishMethod::Submission;
    else if (methodChoice == "3") method = ufc::FinishMethod::UnanimousDecision;
    else if (methodChoice == "4") method = ufc::FinishMethod::SplitDecision;
    else { std::cout << "Invalid method. Cancelled.\n"; return; }

    std::cout << "Date (YYYY-MM-DD, blank = today): ";
    std::getline(std::cin, date);
    if (date.empty()) date = todayIso();

    const double winnerBefore = ratingOf(engine, winner);
    const double loserBefore  = ratingOf(engine, loser);

    const ufc::Fight fight{date, winner, loser, method};
    engine.processFight(fight);

    const ufc::Fighter* w = engine.find(winner);
    const ufc::Fighter* l = engine.find(loser);

    std::cout << std::fixed << std::setprecision(1)
              << "\nResult recorded (" << ufc::toString(method) << ")\n"
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
            ufc::printLeaderboard(std::cout, ufc::buildLeaderboard(engine));
        } else if (choice == "2") {
            runPredictor(engine);
        } else if (choice == "3") {
            recordFight(engine, path);
        } else if (choice == "4") {
            break;
        } else {
            std::cout << "Please choose 1, 2, 3 or 4.\n";
        }
    }
    return 0;
}
