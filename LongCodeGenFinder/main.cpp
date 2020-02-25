#include <iostream>
#include <CppBuildInsights.hpp>

using namespace Microsoft::Cpp::BuildInsights;
using namespace Activities;

class LongCodeGenFinder : public IAnalyzer
{
public:
    // Called by the analysis driver every time an activity stop event
    // is seen in the trace. 
    AnalysisControl OnStopActivity(const EventStack& eventStack) override
    {
        // This will check whether the event stack matches
        // TopFunctionsFinder::CheckForTopFunction's signature.
        // If it does, it will forward the event to the function.

        MatchEventStackInMemberFunction(eventStack, this, 
            &LongCodeGenFinder::CheckForLongFunctionCodeGen);

        // Tells the analysis driver to proceed to the next event

        return AnalysisControl::CONTINUE;
    }

    // This function is used to capture Function activity events that are 
    // within a CodeGeneration activity, and to print a list of functions 
    // that take more than 500 milliseconds to generate.

    void CheckForLongFunctionCodeGen(CodeGeneration cg, Function f)
    {
        using namespace std::chrono;

        if (f.Duration() < milliseconds(500)) {
            return;
        }

        std::wcout << "Duration: " << duration_cast<milliseconds>(
            f.Duration()).count();

        std::wcout << "\t Function Name: " << f.Name() << std::endl;
    }
};

int main(int argc, char *argv[])
{
    if (argc <= 1) return -1;

    LongCodeGenFinder lcgf;

    // Let's make a group of analyzers that will receive
    // events in the trace. We only have one; easy!
    auto group = MakeStaticAnalyzerGroup(&lcgf);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 1;
    return Analyze(argv[1], numberOfPasses, group);
}