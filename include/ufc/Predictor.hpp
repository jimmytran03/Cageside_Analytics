#pragma once
#include "ufc/Fighter.hpp"
#include "ufc/RatingEngine.hpp"
#include <iosfwd>
#include <optional>
#include <string>

namespace ufc {

struct Prediction {
    Fighter fighterA;
    Fighter fighterB;
    double probA = 0.0; // 0..1
    double probB = 0.0;
};

// Returns nullopt if either fighter is unknown.
std::optional<Prediction> predictMatchup(const RatingEngine& engine,
                                         const std::string& nameA,
                                         const std::string& nameB);

void printTaleOfTheTape(std::ostream& out, const Prediction& prediction);

} 
