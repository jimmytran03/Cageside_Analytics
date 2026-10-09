#include "ufc/RatingEngine.hpp"
#include <algorithm>
#include <cctype>

namespace ufc
{
    std::string RatingEngine::key(const std::string& name)
    {
        std::string k{ name };
        std::transform(k.begin(), k.end(), k.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        return k;
    }

    Fighter& RatingEngine::getOrCreate(const std::string& name)
    {
        auto [it, inserted] = fighters_.try_emplace(key(name));
        if (inserted)
        {
            it->second.name = name;
            it->second.rating = calculator_.config().startRating;
        }

        return it->second;
    }

    void RatingEngine::processFight(const Fight& fight)
    {
        Fighter& winner{ getOrCreate(fight.winner) };
        Fighter& loser{ getOrCreate(fight.loser) };

        // Delta uses pre-fight ratings and is applied to both (zero-sum).
        const double winnerBefore{ winner.rating };
        const double loserBefore{ loser.rating };
        const double delta{ calculator_.ratingDelta(winner.rating, loser.rating, fight.method) };

        winner.rating += delta;
        loser.rating -= delta;
        ++winner.wins;
        ++loser.losses;

        winner.history.push_back({ fight.date, loser.name, fight.method, true,
                                   winnerBefore, winner.rating });
        loser.history.push_back({ fight.date, winner.name, fight.method, false,
                                  loserBefore, loser.rating });
    }

    void RatingEngine::processAll(const std::vector<Fight>& fights)
    {
        for (const Fight& f : fights)
        {
            processFight(f);
        }
    }

    const Fighter* RatingEngine::find(const std::string& name) const
    {
        auto it{ fighters_.find(key(name)) };

        return (it == fighters_.end()) ? nullptr : &it->second;
    }

    std::vector<const Fighter*> RatingEngine::search(const std::string& query) const
    {
        std::vector<const Fighter*> results{};
        const std::string q{ key(query) };

        if (q.empty())
        {
            return results;
        }

        if (const Fighter* exact{ find(query) })
        {
            results.push_back(exact);

            return results;
        }

        for (const auto& entry : fighters_)
        {
            if (entry.first.find(q) != std::string::npos)
            {
                results.push_back(&entry.second);
            }
        }

        std::sort(results.begin(), results.end(),
            [](const Fighter* a, const Fighter* b)
            {
                return a->name < b->name;
            });

        return results;
    }

} // namespace ufc