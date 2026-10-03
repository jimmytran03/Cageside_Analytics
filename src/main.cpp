#include "ufc/CsvLoader.hpp"
#include "ufc/Leaderboard.hpp"
#include "ufc/Predictor.hpp"
#include "ufc/RatingEngine.hpp"

#include <iostream>
#include <string>

namespace {

void printMenu() 
{
    std::cout << "\n[1] Leaderboard\n"
              << "[2] Predict a matchup\n"
              << "[3] Quit\n"
              << "> ";
}

void runPredictor(const ufc::RatingEngine& engine) 
{
    std::string a, b;
    std::cout << "Fighter A: ";
    std::getline(std::cin, a);
    std::cout << "Fighter B: ";
    std::getline(std::cin, b);

    if (const auto prediction = ufc::predictMatchup(engine, a, b)) 
    {
        ufc::printTaleOfTheTape(std::cout, *prediction);
    } else 
    {
        std::cout << "One or both fighters were not found in the database.\n";
    }
}

} 

int main(int argc, char* argv[]) 
{
    const std::string path = (argc > 1) ? argv[1] : "data/fights.csv";

    ufc::RatingEngine engine;
    try 
    {
        const auto fights = ufc::loadFightsFromCsv(path);
        engine.processAll(fights);
        std::cout << "Processed " << fights.size() << " fights, "
                  << engine.fighterCount() << " fighters.\n";
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    std::string choice;
    while (true) 
    {
        printMenu();
        if (!std::getline(std::cin, choice)) break;

        if (choice == "1") 
        {
            ufc::printLeaderboard(std::cout, ufc::buildLeaderboard(engine));
        } 
        else if (choice == "2") 
        {
            runPredictor(engine);
        } 
        else if (choice == "3") 
        {
            break;
        } 
        else 
        {
            std::cout << "Please choose 1, 2 or 3.\n";
        }
    }
    return 0;
}
