#pragma once
#include "ufc/EloCalculator.hpp"
#include "ufc/Fight.hpp"
#include "ufc/Fighter.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace ufc {

// Owns the fighter database and applies fights to it.
class RatingEngine {
public:
    explicit RatingEngine(EloCalculator calculator = EloCalculator{})
        : calculator_(calculator) {}

    // Registers unseen fighters at the baseline rating, then updates both.
    void processFight(const Fight& fight);

    // Fights must already be in chronological order.
    void processAll(const std::vector<Fight>& fights);

    // Case-insensitive lookup. Returns nullptr if the fighter is unknown.
    const Fighter* find(const std::string& name) const;

    // Case-insensitive substring search, sorted by name. An exact name match
    // returns just that fighter.
    std::vector<const Fighter*> search(const std::string& query) const;

    const std::unordered_map<std::string, Fighter>& fighters() const { return fighters_; }
    std::size_t fighterCount() const { return fighters_.size(); }
    const EloCalculator& calculator() const { return calculator_; }

private:
    Fighter& getOrCreate(const std::string& name);
    static std::string key(const std::string& name);

    EloCalculator calculator_;
    std::unordered_map<std::string, Fighter> fighters_; // key = lowercase name
};

} // namespace ufc
