#include "ufc/Predictor.hpp"
#include "ufc/EloCalculator.hpp"
#include <iomanip>
#include <ostream>
#include <sstream>

namespace ufc {

std::optional<Prediction> predictMatchup(const RatingEngine& engine,
                                         const std::string& nameA,
                                         const std::string& nameB) {
    const Fighter* a = engine.find(nameA);
    const Fighter* b = engine.find(nameB);
    if (!a || !b) return std::nullopt;

    Prediction p;
    p.fighterA = *a;
    p.fighterB = *b;
    p.probA = EloCalculator::expectedScore(a->rating, b->rating);
    p.probB = 1.0 - p.probA;
    return p;
}

static std::string fmt(double value, int precision, const std::string& suffix = "") 
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << value << suffix;
    return ss.str();
}

static std::string record(const Fighter& f) 
{
    return std::to_string(f.wins) + "-" + std::to_string(f.losses);
}

static void row(std::ostream& out, const std::string& left, const std::string& label,
                const std::string& right) {
    out << std::left << std::setw(22) << left
        << std::setw(12) << label << right << '\n';
}

void printTaleOfTheTape(std::ostream& out, const Prediction& p) 
{
    const std::string rule(56, '=');
    out << rule << '\n'
        << "                 TALE OF THE TAPE\n"
        << rule << '\n';
    row(out, p.fighterA.name, "vs", p.fighterB.name);
    row(out, fmt(p.fighterA.rating, 1), "Elo", fmt(p.fighterB.rating, 1));
    row(out, record(p.fighterA), "Record", record(p.fighterB));
    row(out, fmt(p.probA * 100.0, 1, "%"), "Win chance", fmt(p.probB * 100.0, 1, "%"));
    out << rule << '\n';
}

} 
