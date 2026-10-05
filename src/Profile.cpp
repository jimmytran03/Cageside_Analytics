#include "ufc/Profile.hpp"
#include <algorithm>
#include <iomanip>
#include <ostream>
#include <string>

namespace ufc {

void printProfile(std::ostream& out, const Fighter& f,
                  std::size_t rank, std::size_t totalFighters, double startRating) {
    double peak = startRating;
    double low = startRating;
    for (const FightRecord& r : f.history) {
        peak = std::max(peak, r.ratingAfter);
        low  = std::min(low, r.ratingAfter);
    }
    const int fights = f.wins + f.losses;
    const double winRate = fights > 0 ? 100.0 * f.wins / fights : 0.0;

    const std::string heavy(70, '=');
    const std::string light(70, '-');

    out << '\n' << heavy << '\n' << "  " << f.name << '\n' << heavy << '\n'
        << std::fixed << std::setprecision(1)
        << std::left
        << std::setw(18) << "Current Elo:" << f.rating
        << "   (rank " << rank << " of " << totalFighters << ")\n"
        << std::setw(18) << "Record (W-L):" << f.wins << "-" << f.losses
        << "   (" << winRate << "% win rate)\n"
        << std::setw(18) << "Peak Elo:" << peak << '\n'
        << std::setw(18) << "Lowest Elo:" << low << '\n'
        << light << '\n' << "ELO HISTORY\n" << light << '\n';

    if (f.history.empty()) {
        out << "No fights on record.\n" << heavy << '\n';
        return;
    }

    out << std::left << std::setw(4) << "#" << std::setw(12) << "Date"
        << std::setw(18) << "Opponent" << std::setw(6) << "Res"
        << std::setw(20) << "Method" << std::right
        << std::setw(8) << "Change" << std::setw(9) << "Elo" << '\n'
        << light << '\n';

    int n = 1;
    for (const FightRecord& r : f.history) {
        out << std::left << std::setw(4) << n++ << std::setw(12) << r.date
            << std::setw(18) << r.opponent << std::setw(6) << (r.won ? "W" : "L")
            << std::setw(20) << toString(r.method) << std::right
            << std::showpos << std::setw(8) << (r.ratingAfter - r.ratingBefore)
            << std::noshowpos << std::setw(9) << r.ratingAfter << '\n';
    }
    out << heavy << '\n';
}

} // namespace ufc
