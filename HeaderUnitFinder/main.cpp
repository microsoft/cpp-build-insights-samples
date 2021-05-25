#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <CppBuildInsights.hpp>

using namespace Microsoft::Cpp::BuildInsights;
using namespace Activities;
using namespace SimpleEvents;

class HeaderUnitFinder : public IAnalyzer
{

public:
    HeaderUnitFinder()
    {}

    AnalysisControl OnSimpleEvent(const EventStack& eventStack) override
    {
        if (MatchEventStackInMemberFunction(eventStack, this,
            &HeaderUnitFinder::ProcessHeaderUnitFinder)) {
            std::cout << "Match\n";
        }

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnEndAnalysis() override
    {
        if (isHeaderUnit) {
            std::cout << "Found HeaderUnit(s)\n";
        }
        else {
            std::cout << "No HeaderUnit\n";
        }

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl ProcessHeaderUnitFinder(HeaderUnit h)
    {
        //std::cout << EVENT_ID_HeaderUnit;
        std::cout << "Found a HeaderUnit\n";
        isHeaderUnit = true;
        return AnalysisControl::CONTINUE;
    }


private:
    bool isHeaderUnit = false;
};

int main(int argc, char* argv[])
{
    if (argc <= 1) return -1;

    HeaderUnitFinder lmf;

    auto group = MakeStaticAnalyzerGroup(&lmf);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 1;

    return Analyze(argv[1], numberOfPasses, group);
}