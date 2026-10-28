#include "ufc/FightScorer.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace ufc
{
    ScoreBreakdown FightScorer::score(const FighterStats& s) const
    {
        ScoreBreakdown b{};
        const int otherStrikes{ std::max(0, s.totalLanded - s.sigLanded) };
        b.knockdowns = s.knockdowns * weights_.knockdown;
        b.striking = s.sigLanded * weights_.sigStrike + otherStrikes * weights_.otherStrike;
        b.grappling = s.takedownsLanded * weights_.takedown +
                      s.submissionAttempts * weights_.submissionAttempt;
        b.control = (s.controlSeconds / 60.0) * weights_.controlPerMinute;

        return b;
    }

    FightVerdict FightScorer::judge(const FighterStats& a, const FighterStats& b) const
    {
        FightVerdict v{};
        v.a = score(a);
        v.b = score(b);

        const double scoreA{ v.a.total() };
        const double scoreB{ v.b.total() };
        const double diff{ scoreA - scoreB };
        const double sum{ scoreA + scoreB };

        if (std::fabs(diff) > 1e-9)
        {
            v.winner = (diff > 0) ? Winner::FighterA : Winner::FighterB;
        }
        else if (a.knockdowns != b.knockdowns)           // tiebreakers
        {
            v.winner = (a.knockdowns > b.knockdowns) ? Winner::FighterA : Winner::FighterB;
        }
        else if (a.sigLanded != b.sigLanded)
        {
            v.winner = (a.sigLanded > b.sigLanded) ? Winner::FighterA : Winner::FighterB;
        }
        else if (a.controlSeconds != b.controlSeconds)
        {
            v.winner = (a.controlSeconds > b.controlSeconds) ? Winner::FighterA : Winner::FighterB;
        }
        else
        {
            v.winner = Winner::Draw;
        }

        v.margin = (sum > 0.0) ? std::fabs(diff) / sum : 0.0;
        v.method = (v.margin < weights_.closeMargin) ? FinishMethod::SplitDecision
                                                     : FinishMethod::UnanimousDecision;

        return v;
    }

    static std::string fmt(double value)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << value;

        return ss.str();
    }

    static std::string ratio(int landed, int attempted)
    {
        return std::to_string(landed) + "/" + std::to_string(attempted);
    }

    void printVerdict(std::ostream& out,
                      const std::string& nameA, const FighterStats& a,
                      const std::string& nameB, const FighterStats& b,
                      const FightVerdict& v)
    {
        const std::string rule(60, '=');
        auto row{ [&out](const std::string& left, const std::string& label,
                         const std::string& right)
            {
                out << std::left << std::setw(24) << left << std::setw(16) << label << right << '\n';
            } };

        out << '\n' << rule << '\n' << "                      FIGHT STATS\n" << rule << '\n';
        row(nameA, "", nameB);
        row(std::to_string(a.knockdowns), "Knockdowns", std::to_string(b.knockdowns));
        row(ratio(a.sigLanded, a.sigAttempted), "Sig. strikes", ratio(b.sigLanded, b.sigAttempted));
        row(ratio(a.totalLanded, a.totalAttempted), "Total strikes", ratio(b.totalLanded, b.totalAttempted));
        row(ratio(a.takedownsLanded, a.takedownsAttempted), "Takedowns", ratio(b.takedownsLanded, b.takedownsAttempted));
        row(std::to_string(a.submissionAttempts), "Sub attempts", std::to_string(b.submissionAttempts));
        row(formatControlTime(a.controlSeconds), "Control time", formatControlTime(b.controlSeconds));

        out << rule << '\n' << "                       SCORECARD\n" << rule << '\n';
        row(nameA, "", nameB);
        row(fmt(v.a.knockdowns), "Knockdowns", fmt(v.b.knockdowns));
        row(fmt(v.a.striking), "Striking", fmt(v.b.striking));
        row(fmt(v.a.grappling), "Grappling", fmt(v.b.grappling));
        row(fmt(v.a.control), "Control", fmt(v.b.control));
        row(fmt(v.a.total()), "TOTAL", fmt(v.b.total()));
        out << rule << '\n';

        if (v.winner == Winner::Draw)
        {
            out << "Result: too close to call. The stats are completely even.\n";
        }
        else
        {
            const std::string& winnerName{ (v.winner == Winner::FighterA) ? nameA : nameB };
            out << "Winner: " << winnerName << '\n'
                << "Scored as: " << toString(v.method)
                << " (score margin " << fmt(v.margin * 100.0) << "%)\n";
        }

        out << rule << '\n';
    }

} 