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

class FunctionBottlenecks : public IAnalyzer
{
    struct IdentifiedFunction
    {
        std::string Name;
        std::chrono::milliseconds Duration;
        double Percent;
        unsigned ForceInlineeSize;

        bool operator<(const IdentifiedFunction& other) const {
            return Duration > other.Duration;
        }
    };

public:
    FunctionBottlenecks():
        pass_{0},
        cachedInvocationDurations_{},
        identifiedFunctions_{},
        forceInlineSizeCache_{}
    {}

    AnalysisControl OnBeginAnalysisPass() override
    {
        ++pass_;
        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnStopActivity(const EventStack& eventStack)
        override
    {
        switch (pass_)
        {
        case 1:
            MatchEventStackInMemberFunction(eventStack, this,
                &FunctionBottlenecks::OnStopInvocation);
            break;

        case 2:
            MatchEventStackInMemberFunction(eventStack, this,
                &FunctionBottlenecks::OnStopFunction);
            break;

        default:
            break;
        }

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnSimpleEvent(const EventStack& eventStack)
    {
        if (pass_ > 1) {
            return AnalysisControl::CONTINUE;
        }

        MatchEventStackInMemberFunction(eventStack, this,
            &FunctionBottlenecks::ProcessForceInlinee);

        return AnalysisControl::CONTINUE;
    }

    void OnStopInvocation(Invocation invocation)
    {
        using namespace std::chrono;

        // Ignore very short invocations
        if (invocation.Duration() < std::chrono::seconds(1)) {
            return;
        }

        cachedInvocationDurations_[invocation.EventInstanceId()] =
            duration_cast<milliseconds>(invocation.Duration());
    }

    void OnStopFunction(Invocation invocation, Function func)
    {
        using namespace std::chrono;

        auto itInvocation = cachedInvocationDurations_.find(
            invocation.EventInstanceId());

        if (itInvocation == cachedInvocationDurations_.end()) {
            return;
        }

        auto itForceInlineSize = forceInlineSizeCache_.find(
            func.EventInstanceId());

        unsigned forceInlineSize =
            itForceInlineSize == forceInlineSizeCache_.end() ?
                0 : itForceInlineSize->second;

        milliseconds functionMilliseconds = 
            duration_cast<milliseconds>(func.Duration());

        double functionTime = static_cast<double>(
            functionMilliseconds.count());

        double invocationTime = static_cast<double>(
            itInvocation->second.count());

        double percent = functionTime / invocationTime;

        if (percent > 0.05 && func.Duration() >= seconds(1))
        {
            identifiedFunctions_[func.EventInstanceId()]= 
                { func.Name(), functionMilliseconds, percent, 
                  forceInlineSize };
        }
    }

    void ProcessForceInlinee(Function func, ForceInlinee inlinee)
    {
        forceInlineSizeCache_[func.EventInstanceId()] += 
            inlinee.Size();
    }

    AnalysisControl OnEndAnalysis() override
    {
        std::vector<IdentifiedFunction> sortedFunctions;

        for (auto& p : identifiedFunctions_) {
            sortedFunctions.push_back(p.second);
        }

        std::sort(sortedFunctions.begin(), sortedFunctions.end());

        for (auto& func : sortedFunctions)
        {
            bool forceInlineHeavy = func.ForceInlineeSize >= 10000;

            std::string forceInlineIndicator = forceInlineHeavy ?
                ", *" : "";

            int percent = static_cast<int>(func.Percent * 100);

            std::string percentString = "(" + 
                std::to_string(percent) + "%" + 
                forceInlineIndicator + ")";

            std::cout << std::setw(9) << std::right << 
                func.Duration.count();
            std::cout << " ms ";
            std::cout << std::setw(9) << std::left << 
                percentString;
            std::cout << " " << func.Name << std::endl;
        }

        return AnalysisControl::CONTINUE;
    }

private:
    unsigned pass_;

    std::unordered_map<unsigned long long,
        std::chrono::milliseconds> cachedInvocationDurations_;

    std::unordered_map<unsigned long long, 
        IdentifiedFunction> identifiedFunctions_;

    std::unordered_map<unsigned long long, 
        unsigned> forceInlineSizeCache_;
};

int main(int argc, char* argv[])
{
    if (argc <= 1) return -1;

    std::cout.imbue(std::locale(""));

    FunctionBottlenecks fb;

    auto group = MakeStaticAnalyzerGroup(&fb);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 2;
    return Analyze(argv[1], numberOfPasses, group);
}