#include "ufc/Fight.hpp"
#include <cctype>

namespace ufc
{
    static std::string normalize(const std::string& s)
    {
        std::string out{};
        for (unsigned char c : s)
        {
            if (!std::isspace(c))
            {
                out.push_back(static_cast<char>(std::toupper(c)));
            }
        }

        return out;
    }

    std::optional<FinishMethod> parseFinishMethod(const std::string& text)
    {
        const std::string t{ normalize(text) };
        if (t == "KO/TKO" || t == "KO" || t == "TKO")
        {
            return FinishMethod::KO_TKO;
        }

        if (t == "SUBMISSION" || t == "SUB")
        {
            return FinishMethod::Submission;
        }

        if (t == "UNANIMOUSDECISION" || t == "UD")
        {
            return FinishMethod::UnanimousDecision;
        }

        if (t == "SPLITDECISION" || t == "SD")
        {
            return FinishMethod::SplitDecision;
        }

        return std::nullopt;
    }

    std::string toString(FinishMethod method)
    {
        switch (method)
        {
        case FinishMethod::KO_TKO:
            return "KO/TKO";
        case FinishMethod::Submission:
            return "Submission";
        case FinishMethod::UnanimousDecision:
            return "Unanimous Decision";
        case FinishMethod::SplitDecision:
            return "Split Decision";
        }

        return "Unknown";
    }

} 