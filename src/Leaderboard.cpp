#include "ufc/Leaderboard.hpp"
#include <algorithm>
#include <iomanip>
#include <ostream>
#include <string>

namespace ufc {

std::vector<Fighter> buildLeaderboard(const RatingEngine& engine) 
{
    std::vector<Fighter> ranked;
    ranked.reserve(engine.fighterCount());
    for (const auto& entry : engine.fighters()) ranked.push_back(entry.second);

    std::sort(ranked.begin(), ranked.end(), [](const Fighter& a, const Fighter& b) 
    {
        if (a.rating != b.rating) return a.rating > b.rating;
        return a.name < b.name;
    });
    return ranked;
}

void printLeaderboard(std::ostream& out, const std::vector<Fighter>& ranked,
                      std::size_t limit) {
    const std::size_t rows = (limit == 0) ? ranked.size() : std::min(limit, ranked.size());
    const std::string rule(50, '-');

    out << rule << '\n'
        << std::left << std::setw(6) << "Rank" << std::setw(24) << "Fighter"
        << std::right << std::setw(10) << "Elo" << std::setw(10) << "W-L" << '\n'
        << rule << '\n';

    out << std::fixed << std::setprecision(1);
    for (std::size_t i = 0; i < rows; ++i) {
        const Fighter& f = ranked[i];
        const std::string record = std::to_string(f.wins) + "-" + std::to_string(f.losses);
        out << std::left << std::setw(6) << (i + 1) << std::setw(24) << f.name
            << std::right << std::setw(10) << f.rating << std::setw(10) << record << '\n';
    }
    out << rule << '\n';
}

}  
