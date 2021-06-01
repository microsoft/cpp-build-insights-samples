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

class LongPrecompiledHeaderFinder : public IAnalyzer
{
    struct IdentifiedCompilerInvocation
    {
        unsigned InvocationId;
        double InvocationTime;

        bool operator<(const IdentifiedCompilerInvocation& other) const {
            return InvocationTime > other.InvocationTime;
        }
    };

public:
    LongPrecompiledHeaderFinder():
        pass_{ 0 },
        cachedInvocationDurations_{},
        identifiedCompilerInvocations_{}
    {}

    AnalysisControl OnBeginAnalysisPass() override
    {
        ++pass_;
        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnStopActivity(const EventStack& eventStack)
        override
    {
        if (pass_ == 1) {
            MatchEventStackInMemberFunction(eventStack, this,
                &LongPrecompiledHeaderFinder::OnStopCompiler);
        }

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnSimpleEvent(const EventStack& eventStack) override
    {
        if (pass_ == 2) {
            MatchEventStackInMemberFunction(eventStack, this,
                &LongPrecompiledHeaderFinder::OnPrecompiledHeaderEvent);
        }

        return AnalysisControl::CONTINUE;
    }

    void OnStopCompiler(Compiler cl)
    {
        using namespace std::chrono;

        if (cl.Duration() < std::chrono::seconds(1)) {
            return;
        }

        cachedInvocationDurations_[cl.EventInstanceId()] =
            duration_cast<milliseconds>(cl.Duration());
    }

    void OnPrecompiledHeaderEvent(Compiler cl, PrecompiledHeader pch)
    {
        using namespace std::chrono;

        auto itInvocation = cachedInvocationDurations_.find(
            cl.EventInstanceId());

        if (itInvocation == cachedInvocationDurations_.end()) {
            return;
        }

        double invocationTime = static_cast<double>(
            itInvocation->second.count()) / 1000;

        identifiedCompilerInvocations_[cl.EventInstanceId()] =
            { cl.InvocationId(), invocationTime};
    }

    AnalysisControl OnEndAnalysis() override
    {
        std::vector<IdentifiedCompilerInvocation> sortedCompilerInvocations;

        for (auto& p : identifiedCompilerInvocations_) {
            sortedCompilerInvocations.push_back(p.second);
        }

        std::sort(sortedCompilerInvocations.begin(), sortedCompilerInvocations.end());

        for (auto& func : sortedCompilerInvocations)
        {
            std::cout << "CL Invocation " << func.InvocationId << "\t\tDuration: " << func.InvocationTime << " s " << std::endl;
        }

        return AnalysisControl::CONTINUE;
    }

private:
    unsigned pass_;

    std::unordered_map<unsigned long long,
        std::chrono::milliseconds> cachedInvocationDurations_;

    std::unordered_map<unsigned long long,
        IdentifiedCompilerInvocation> identifiedCompilerInvocations_;
};

int main(int argc, char* argv[])
{
    if (argc <= 1) return -1;

    LongPrecompiledHeaderFinder lmf;

    auto group = MakeStaticAnalyzerGroup(&lmf);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 2;

    return Analyze(argv[1], numberOfPasses, group);
}