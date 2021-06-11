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

class LongModuleFinder : public IAnalyzer
{
    struct FrontEndPassData
    {
        std::wstring Name;
        unsigned InvocationId;
        double Duration;

        bool operator<(const FrontEndPassData& other) const {
            return Duration > other.Duration;
        }
    };

public:
    LongModuleFinder() :
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
                &LongModuleFinder::OnStopFrontEndPass);

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnSimpleEvent(const EventStack& eventStack) override
    {
        MatchEventStackInMemberFunction(eventStack, this,
                &LongModuleFinder::OnModuleEvent);

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

        std::wstring inputSourcePathWstr(frontEndPass.InputSourcePath());

        FrontEndPassData_[frontEndPass.EventInstanceId()] = { inputSourcePathWstr, cl.InvocationId(), duration };
    }

    void OnModuleEvent(FrontEndPass frontEndPass, Module m)
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

        for (auto& frontEndPassData : sortedFrontEndPassData)
        {
            std::cout << "File Name: ";
            std::wcout << frontEndPassData.Name;
            std::cout << "\t\tCL Invocation " << frontEndPassData.InvocationId << "\t\tDuration: " << frontEndPassData.Duration << " s " << std::endl;
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

    LongModuleFinder lmf;

    auto group = MakeStaticAnalyzerGroup(&lmf);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 1;

    return Analyze(argv[1], numberOfPasses, group);
}