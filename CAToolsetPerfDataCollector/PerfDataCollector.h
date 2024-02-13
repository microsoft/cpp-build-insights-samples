#pragma once

#include <iostream>
#include <CppBuildInsights.hpp>
#include <map>
#include <unordered_map>
#include <compare>

#include <comutil.h>

using namespace Microsoft::Cpp::BuildInsights;
using namespace Activities;
using namespace SimpleEvents;
using namespace std::chrono;

struct PathId
{
    int id;
    const std::wstring* path;

    constexpr auto operator<=>(const PathId& b) const { return *path <=> *b.path; };
};

struct PerfData
{
    // Notes on FrontEndPass, CodeAnalysisPass, and BackEndPass:
    //  - FrontEndPass can be omitted if /analyze:only is used. In that case, the CodeAnalysisPass is the only pass.
    //  - CodeAnalysisPass is a special FrontEndPass for Code Analysis when /analyze option is used, and includes actual analysis of code.
    //  - BackEndPass can be omitted if /c and/or /analyze:only is used.
    microseconds FEPass;                    // FrontEnd pass
    microseconds BEPass;                    // BackEnd pass
    microseconds CAPass;                    // CodeAnalysis pass
    microseconds ASTCreation;               // AST creation for Code Analysis
    microseconds ASTClients;                // COM clients for Code Analysis, including PREfast
    microseconds CAFunction;                // PREfast's Code Analysis of functions, including analysis of function by all of its plug-ins
    microseconds FPAFunction;               // PREfast's path-sensitive Code Analysis of functions (FPA = Function Path Analysis)
    microseconds EspXCfgBuild;              // CFG Building for functions to be used by EspXEngine and its extensions
    microseconds EspXAllChecks;             // Analysis of functions by EspXEngine and its extensions
    microseconds EspXPathSensitiveChecks;   // Path-sensitive analysis of functions by EspXEngine and its extensions

    // This header is used to print the performance data to the console.
    // It should be updated if the PerfData struct is updated.
    // Note that this header does not include the file path, which should be printed separately if needed.
    static constexpr std::wstring_view PerfDataHeader =
        L"FrontEnd Pass, BackEnd Pass, CodeAnalysis Pass, "
        L"AST Creation, AST Clients, "                                      // Children of CodeAnalysis Pass
        L"Function Analysis, "                                              // Child of AST Clients
        L"FPA Function Analysis, EspX CFG Build, EspX Function Analysis, "  // Children of Function Analysis
        L"EspX Path-sensitive Analysis";                                    // Child of EspX Function Analysis
};

class PerfDataCollector : public IAnalyzer
{
private:
    std::unordered_map<std::wstring, PathId> filePaths{};   // List of file paths
    std::map<PathId, PerfData> perfDataPerTu{};                  // Performance data per TU

    // Find the PathId for the given file path.
    // If the file path is not found, a new PathId is created.
    PathId& FindFilePathId(std::wstring path)
    {
        static int id = 0;
        auto result = filePaths.try_emplace(path, PathId{});
        if (result.second)
        {
            result.first->second.id = id++;
            result.first->second.path = &result.first->first;
        }
        return result.first->second;
    }

    // Gets duration of the specified compiler pass (should be one of FE, BE, and CA pass),
    // finds or adds performance data for the file path, and adds the duration to the specified field
    // of the performance data.
    template<typename TPass, microseconds PerfData::* RecField>
    void GetPassDuration(TPass pass)
    {
        static_assert(std::is_base_of_v<CompilerPass, TPass>, "TPass must be derived from CompilerPass");

        const auto tuPathId = FindFilePathId(pass.InputSourcePath());

        if (verbose_)
        {
            std::wcout << L"[" << *tuPathId.path << L"](" << pass.EventName() << L") = "
                << std::chrono::duration_cast<microseconds>(pass.Duration()).count() << L"(microsec)" << std::endl;
        }

        auto& data = perfDataPerTu[tuPathId];
        data.*RecField += std::chrono::duration_cast<microseconds>(pass.Duration());
    }

    // Gets duration of the specified CA activity (should be a descendent of CA Pass),
    // finds performance data for the file path, and adds the duration to the specified field
    // of the performance data.
    template<typename TEvent, microseconds PerfData::* RecField>
    void GetCodeAnalysisEventDuration(CodeAnalysisPass pass, TEvent event)
    {
        static_assert(std::is_base_of_v<Activity, TEvent>, "TEvent must be derived from Activity");

        const auto tuPathId = FindFilePathId(pass.InputSourcePath());

        if (verbose_)
        {
            std::wcout << L"[" << *tuPathId.path << L"](" << event.EventName() << L") = "
                << std::chrono::duration_cast<microseconds>(event.Duration()).count() << L"(microsec)" << std::endl;
        }

        auto& data = perfDataPerTu[tuPathId];
        data.*RecField += std::chrono::duration_cast<microseconds>(event.Duration());
    }

    bool verbose_;

public:

    PerfDataCollector(bool verbose)
        : verbose_(verbose)
    {
    }

    // Called by the analysis driver every time an activity stop event is seen in the trace.
    AnalysisControl OnStopActivity(const EventStack& eventStack) override;

    // Prints the per-TU performance data in CSV format.
    void PrintRecords();

    // Print the summary of the performance data.
    void PrintSummary();
};

