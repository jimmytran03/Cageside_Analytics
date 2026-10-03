#include "ufc/RatingEngine.hpp"
#include <algorithm>
#include <cctype>

namespace ufc {

std::string RatingEngine::key(const std::string& name) 
{
    std::string k = name;
    std::transform(k.begin(), k.end(), k.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return k;
}

Fighter& RatingEngine::getOrCreate(const std::string& name) 
{
    auto [it, inserted] = fighters_.try_emplace(key(name));
    if (inserted) {
        it->second.name = name;
        it->second.rating = calculator_.config().startRating;
    }
    return it->second;
}

void RatingEngine::processFight(const Fight& fight) 
{
    Fighter& winner = getOrCreate(fight.winner);
    Fighter& loser  = getOrCreate(fight.loser);

    // Delta uses pre-fight ratings and is applied to both (zero-sum).
    const double delta = calculator_.ratingDelta(winner.rating, loser.rating, fight.method);
    winner.rating += delta;
    loser.rating  -= delta;
    ++winner.wins;
    ++loser.losses;
}

void RatingEngine::processAll(const std::vector<Fight>& fights) 
{
    for (const Fight& f : fights) processFight(f);
}

const Fighter* RatingEngine::find(const std::string& name) const 
{
    auto it = fighters_.find(key(name));
    return it == fighters_.end() ? nullptr : &it->second;
}

} 
