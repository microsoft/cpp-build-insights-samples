#include <iostream>
#include <string>
#include <unordered_map>
#include <CppBuildInsights.hpp>

using namespace Microsoft::Cpp::BuildInsights;
using namespace Activities;
using namespace SimpleEvents;

class BottleneckCompileFinder : public IAnalyzer
{
    struct InvocationInfo
    {
        bool IsBottleneck;
        bool UsesParallelFlag;
    };

public:
    BottleneckCompileFinder()
    {}

    AnalysisControl OnStartActivity(const EventStack& eventStack)
        override
    {
        MatchEventStackInMemberFunction(eventStack, this,
            &BottleneckCompileFinder::OnStartInvocation);

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnStopActivity(const EventStack& eventStack)
        override
    {
        MatchEventStackInMemberFunction(eventStack, this,
            &BottleneckCompileFinder::OnStopInvocation);

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnSimpleEvent(const EventStack& eventStack)
        override
    {
        MatchEventStackInMemberFunction(eventStack, this,
            &BottleneckCompileFinder::OnCompilerCommandLine);

        return AnalysisControl::CONTINUE;
    }

    void OnStartInvocation(InvocationGroup group)
    {
        // We need to match groups because CL can
        // start a linker, and a linker can restart
        // itself. When this happens, the event stack
        // contains the parent invocations in earlier
        // positions.

        // A linker that is spawned by a previous tool is 
        // not considered an invocation that runs in
        // parallel with the tool that spawned it.
        if (group.Size() > 1) {
            return;
        }

        // An invocation is speculatively considered a bottleneck 
        // if no other invocations are currently running when it starts.
        bool isBottleneck = concurrentInvocations_.empty();

        // If there is already an invocation running, it is no longer
        // considered a bottleneck because we are spawning another one
        // that will run alongside it. Clear its bottleneck flag.
        if (concurrentInvocations_.size() == 1) {
            concurrentInvocations_.begin()->second.IsBottleneck = false;
        }

        InvocationInfo& info = concurrentInvocations_[group.Back().EventInstanceId()];

        info.IsBottleneck = isBottleneck;
    }

    void OnCompilerCommandLine(Compiler cl, CommandLine commandLine)
    {
        auto it = concurrentInvocations_.find(cl.EventInstanceId());

        if (it == concurrentInvocations_.end()) {
            return;
        }

        // Keep track of CL invocations that don't use MP so that we can
        // warn the user if this invocation is a bottleneck.

        std::wstring str = commandLine.Value();

        if (str.find(L" /MP ") != std::wstring::npos ||
            str.find(L" -MP ") != std::wstring::npos)
        {
            it->second.UsesParallelFlag = true;
        }
    }

    void OnStopInvocation(Invocation invocation)
    {
        using namespace std::chrono;

        auto it = concurrentInvocations_.find(invocation.EventInstanceId());

        if (it == concurrentInvocations_.end()) {
            return;
        }

        if (invocation.Type() == Invocation::Type::CL &&
            it->second.IsBottleneck &&
            !it->second.UsesParallelFlag)
        {
            std::cout << std::endl << "WARNING: Found a compiler invocation that is a " <<
                "bottleneck but that doesn't use the /MP flag. Consider adding " <<
                "the /MP flag." << std::endl;

            std::cout << "Information about the invocation:" << std::endl;
            std::wcout << "Working directory: " << invocation.WorkingDirectory() << std::endl;
            std::cout << "Duration: " << duration_cast<seconds>(invocation.Duration()).count() <<
                " s" << std::endl;
        }

        concurrentInvocations_.erase(invocation.EventInstanceId());
    }

private:
    // A hash table that maps cl or link invocations to a flag
    // that indicates whether this invocation is a bottleneck.
    // In this sample, an invocation is considered a bottleneck 
    // when no other compiler or linker is running alonside it 
    // at any point.
    std::unordered_map<unsigned long long, InvocationInfo> concurrentInvocations_;
};

int main(int argc, char* argv[])
{
    if (argc <= 1) return -1;

    BottleneckCompileFinder bcf;

    auto group = MakeStaticAnalyzerGroup(&bcf);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 1;
    return Analyze(argv[1], numberOfPasses, group);
}