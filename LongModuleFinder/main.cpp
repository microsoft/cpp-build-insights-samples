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

class LongModuleFinder : public IAnalyzer
{

public:
    LongModuleFinder()
    {}

    AnalysisControl OnSimpleEvent(const EventStack& eventStack) override
    {
        if (MatchEventStackInMemberFunction(eventStack, this,
            &LongModuleFinder::ProcessModuleFinder)) {
            std::cout << "Match\n";
        }

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnEndAnalysis() override
    {
        if (isModule) {
            std::cout << "Found module(s)\n";
        }
        else {
            std::cout << "No module\n";
        }

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl ProcessModuleFinder(Module m)
    {
        //std::cout << EVENT_ID_MODULE;
        std::cout << "Found a module\n";
        isModule = true;
        return AnalysisControl::CONTINUE;
    }


private:
    bool isModule = false;
};

int main(int argc, char* argv[])
{
    if (argc <= 1) return -1;

    LongModuleFinder lmf;

    auto group = MakeStaticAnalyzerGroup(&lmf);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 1;

    return Analyze(argv[1], numberOfPasses, group);
}