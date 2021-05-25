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

class PrecompiledHeaderFinder : public IAnalyzer
{

public:
    PrecompiledHeaderFinder()
    {}

    AnalysisControl OnSimpleEvent(const EventStack& eventStack) override
    {
        if (MatchEventStackInMemberFunction(eventStack, this,
            &PrecompiledHeaderFinder::ProcessPrecompiledHeaderFinder)) {
            std::cout << "Match\n";
        }

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnEndAnalysis() override
    {
        if (isPrecompiledHeader) {
            std::cout << "Found PrecompiledHeader(s)\n";
        }
        else {
            std::cout << "No PrecompiledHeaders\n";
        }

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl ProcessPrecompiledHeaderFinder(PrecompiledHeader m)
    {
        //std::cout << EVENT_ID_PrecompiledHeader;
        std::cout << "Found a PrecompiledHeader\n";
        isPrecompiledHeader = true;
        return AnalysisControl::CONTINUE;
    }


private:
    bool isPrecompiledHeader = false;
};

int main(int argc, char* argv[])
{
    if (argc <= 1) return -1;

    PrecompiledHeaderFinder lmf;

    auto group = MakeStaticAnalyzerGroup(&lmf);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 1;

    return Analyze(argv[1], numberOfPasses, group);
}