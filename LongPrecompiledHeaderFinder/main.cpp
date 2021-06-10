#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <CppBuildInsights.hpp>

using namespace Microsoft::Cpp::BuildInsights;
using namespace Activities;
using namespace SimpleEvents;

class LongPrecompiledHeaderFinder : public IAnalyzer
{
    struct FrontEndPassData
    {
        std::string Name;
        unsigned InvocationId;
        double Duration;

        bool operator<(const FrontEndPassData& other) const {
            return Duration > other.Duration;
        }
    };

public:
    LongPrecompiledHeaderFinder() :
        cachedFrontEndPassIds_{},
        FrontEndPassData_{}
    {}

    AnalysisControl OnBeginAnalysisPass() override
    {
        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnStopActivity(const EventStack& eventStack)
        override
    {
        MatchEventStackInMemberFunction(eventStack, this,
            &LongPrecompiledHeaderFinder::OnStopFrontEndPass);

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnSimpleEvent(const EventStack& eventStack) override
    {
        MatchEventStackInMemberFunction(eventStack, this,
            &LongPrecompiledHeaderFinder::OnPrecompiledHeaderEvent);

        return AnalysisControl::CONTINUE;
    }

    void OnStopFrontEndPass(Compiler cl, FrontEndPass frontEndPass)
    {
        // if the EventInstanceId of the current FrontEndPass has been saved

        auto itInvocation = cachedFrontEndPassIds_.find(
            frontEndPass.EventInstanceId());

        if (itInvocation == cachedFrontEndPassIds_.end()) {
            return;
        }

        using namespace std::chrono;

        if (frontEndPass.Duration() < std::chrono::seconds(1)) {
            return;
        }

        double duration = static_cast<double>(duration_cast<milliseconds>(frontEndPass.Duration()).count()) / 1000;

        std::wstring inputSourcePathWs(frontEndPass.InputSourcePath());
        std::string inputSourcePathStr(inputSourcePathWs.begin(), inputSourcePathWs.end());

        FrontEndPassData_[frontEndPass.EventInstanceId()] = { inputSourcePathStr, cl.InvocationId(), duration };
    }

    void OnPrecompiledHeaderEvent(FrontEndPass frontEndPass, PrecompiledHeader pch)
    {
        // Save the EventInstanceId of the current FrontEndPass
        cachedFrontEndPassIds_.insert(frontEndPass.EventInstanceId());
    }

    AnalysisControl OnEndAnalysis() override
    {
        std::vector<FrontEndPassData> sortedFrontEndPassData;

        for (auto& p : FrontEndPassData_) {
            sortedFrontEndPassData.push_back(p.second);
        }

        std::sort(sortedFrontEndPassData.begin(), sortedFrontEndPassData.end());

        for (auto& func : sortedFrontEndPassData)
        {
            std::cout << "File Name: " << func.Name << "\t\tCL Invocation " << func.InvocationId << "\t\tDuration: " << func.Duration << " s " << std::endl;
        }

        return AnalysisControl::CONTINUE;
    }

private:
    std::unordered_set<unsigned long long> cachedFrontEndPassIds_;

    std::unordered_map<unsigned long long,
        FrontEndPassData> FrontEndPassData_;
};

int main(int argc, char* argv[])
{
    if (argc <= 1) return -1;

    LongPrecompiledHeaderFinder lpchf;

    auto group = MakeStaticAnalyzerGroup(&lpchf);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 1;

    return Analyze(argv[1], numberOfPasses, group);
}