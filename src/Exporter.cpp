#include "ufc/Exporter.hpp"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace ufc
{
    namespace
    {
        std::ofstream openForWrite(const std::string& path)
        {
            const std::filesystem::path p(path);
            if (p.has_parent_path())
            {
                std::filesystem::create_directories(p.parent_path());
            }

            std::ofstream out(path, std::ios::trunc);
            if (!out)
            {
                throw std::runtime_error("Could not open file for writing: " + path);
            }

            return out;
        }

        void checkWritten(std::ofstream& out, const std::string& path)
        {
            out.flush();
            if (!out)
            {
                throw std::runtime_error("Failed while writing: " + path);
            }
        }

    } // namespace

    std::string csvEscape(const std::string& field)
    {
        if (field.find_first_of(",\"\r\n") == std::string::npos)
        {
            return field;
        }

        std::string quoted{ "\"" };
        for (char c : field)
        {
            if (c == '"')
            {
                quoted += '"';   // double any quote
            }

            quoted += c;
        }

        return quoted + "\"";
    }

    std::string sanitizeFilename(const std::string& name)
    {
        std::string out{};
        bool lastWasSeparator{ true };   // also trims leading separators
        for (unsigned char c : name)
        {
            if (std::isalnum(c))
            {
                out += static_cast<char>(std::tolower(c));
                lastWasSeparator = false;
            }
            else if (!lastWasSeparator)
            {
                out += '_';
                lastWasSeparator = true;
            }
        }

        while (!out.empty() && out.back() == '_')
        {
            out.pop_back();
        }

        return out.empty() ? "fighter" : out;
    }

    void exportLeaderboardCsv(const std::string& path, const std::vector<Fighter>& ranked)
    {
        std::ofstream out{ openForWrite(path) };
        out << "rank,fighter,elo,wins,losses,fights,win_rate_pct\n" << std::fixed;
        for (std::size_t i{ 0 }; i < ranked.size(); ++i)
        {
            const Fighter& f{ ranked[i] };
            const int fights{ f.wins + f.losses };
            const double winRate{ fights > 0 ? 100.0 * f.wins / fights : 0.0 };
            out << (i + 1) << ',' << csvEscape(f.name) << ','
                << std::setprecision(2) << f.rating << ','
                << f.wins << ',' << f.losses << ',' << fights << ','
                << std::setprecision(1) << winRate << '\n';
        }

        checkWritten(out, path);
    }

    void exportFighterHistoryCsv(const std::string& path, const Fighter& fighter)
    {
        std::ofstream out{ openForWrite(path) };
        out << "fight_number,date,opponent,result,method,rating_before,rating_after,change\n"
            << std::fixed << std::setprecision(2);
        int n{ 1 };
        for (const FightRecord& r : fighter.history)
        {
            out << n++ << ',' << csvEscape(r.date) << ',' << csvEscape(r.opponent) << ','
                << (r.won ? "W" : "L") << ',' << csvEscape(toString(r.method)) << ','
                << r.ratingBefore << ',' << r.ratingAfter << ','
                << (r.ratingAfter - r.ratingBefore) << '\n';
        }

        checkWritten(out, path);
    }

    void writeTextFile(const std::string& path, const std::string& content)
    {
        std::ofstream out{ openForWrite(path) };
        out << content;
        checkWritten(out, path);
    }

} 