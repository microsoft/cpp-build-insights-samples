#include "PerfDataCollector.h"
#include <algorithm>

AnalysisControl PerfDataCollector::OnStopActivity(const EventStack& eventStack)
{
    //
    // Overall C1, C1/analyze, C1 Passes
    // 

    // FE Pass
    MatchEventStackInMemberFunction(eventStack, this,
        &PerfDataCollector::GetPassDuration<FrontEndPass, &PerfData::FEPass>);
    // CA Pass
    MatchEventStackInMemberFunction(eventStack, this,
        &PerfDataCollector::GetPassDuration<CodeAnalysisPass, &PerfData::CAPass>);
    // BE Pass
    MatchEventStackInMemberFunction(eventStack, this,
        &PerfDataCollector::GetPassDuration<BackEndPass, &PerfData::BEPass>);

    //
    // Activities in FE during CA Pass
    //

    // AST Creation
    MatchEventStackInMemberFunction(eventStack, this,
        &PerfDataCollector::GetCodeAnalysisEventDuration<AstCreation, &PerfData::ASTCreation>);
    // SST Clients, e.g., PREfast
    MatchEventStackInMemberFunction(eventStack, this,
        &PerfDataCollector::GetCodeAnalysisEventDuration<CodeAnalysisPlugins, &PerfData::ASTClients>);

    //
    // Activities in PREfast during CA Pass
    //

    // CA PREfast's Function Analysis (including its own plug-ins, EspXEngine, and EspXEngine extensions)
    MatchEventStackInMemberFunction(eventStack, this,
        &PerfDataCollector::GetCodeAnalysisEventDuration<CodeAnalysisFunction, &PerfData::CAFunction>);
    // PREfast's FPA Function Analysis (FPA = Function Path Analysis)
    MatchEventStackInMemberFunction(eventStack, this,
        &PerfDataCollector::GetCodeAnalysisEventDuration<CodeAnalysisPREfastFpaFunction, &PerfData::FPAFunction>);
    // EspXEngine's CFG Building
    MatchEventStackInMemberFunction(eventStack, this,
        &PerfDataCollector::GetCodeAnalysisEventDuration<EspXEngineCfgBuild, &PerfData::EspXCfgBuild>);
    // EspXEngine's All Checks (BufferCheck and extensions, whichever are enabled)
    MatchEventStackInMemberFunction(eventStack, this,
        &PerfDataCollector::GetCodeAnalysisEventDuration<EspXEngineChecks, &PerfData::EspXAllChecks>);
    // EspXEngine's PathSensitiveChecks (BufferCheck and some of the extensions, whichever are enabled)
    MatchEventStackInMemberFunction(eventStack, this,
        &PerfDataCollector::GetCodeAnalysisEventDuration<EspXEnginePathSimulation, &PerfData::EspXPathSensitiveChecks>);

    // Tell the analysis driver to proceed to the next event
    return AnalysisControl::CONTINUE;
}

// Prints the per-TU performance data in CSV format. The first column is the file path.
// The rest of the columns are the performance data in microseconds.
void PerfDataCollector::PrintRecords()
{
    constexpr auto allZero = PerfData{};

    // Print Header
    std::wcout << L"File Path, " << PerfData::PerfDataHeader << std::endl;

    for (const auto& [pathId, data] : perfDataPerTu)
    {
        // If CA Pass is less than FE Pass, it is for files that are not target of Code Analysis,
        // or CA Pass is not captured properly - e.g., due to errors only occurring during CA Pass.
        // or exceptions during Code Analysis.
        // Ignore the record by reporting everything as 0. We still want the TU to be listed.
        const auto& d = (data.CAPass < data.FEPass) ? allZero : data;

        std::wcout
            << *pathId.path << L", " << d.FEPass.count() << L", " << d.BEPass.count() << L", " << d.CAPass.count() << L", "
            << d.ASTCreation.count() << L", " << d.ASTClients.count() << L", " << d.CAFunction.count() << L", "
            << d.FPAFunction.count() << L", " << d.EspXCfgBuild.count() << L", " << d.EspXAllChecks.count() << L", "
            << d.EspXPathSensitiveChecks.count() << std::endl;
    }
}

// Utility to restore the state of std::wcout
struct WcoutStateRestorer
{
    ~WcoutStateRestorer()
    {
        std::wcout.precision(precision_);
        std::wcout.flags(flags_);
    }
private:
    std::ios_base::fmtflags flags_{ std::wcout.flags() };
    std::streamsize precision_{ std::wcout.precision() };
};

// Represents an indentation printing. Use Indenter to manage indentation level per scope.
template<size_t IndentSize>
struct Indent
{
    void operator++() { buffer_.append(IndentSize, L' '); }
    void operator--() { buffer_.resize(buffer_.size() - IndentSize); }
    friend std::wostream& operator<<(std::wostream& out, const Indent& indent) { out << indent.buffer_; return out; }
private:
    std::wstring buffer_{ };
};

// Use this to automatically manage indentation level per scope for an instance of Indent<N>.
// For example:
//  Indent<2> indent;
//  {
//      Indenter _{ indent };
//      std::wcout << indent << L"Indented by 2 spaces" << std::endl;
//      {
//          Indenter _{ indent };
//          std::wcout << indent << L"Indented by 4 spaces" << std::endl;
//      }
//      std::wcout << indent << L"Indented by 2 spaces" << std::endl;
//  }
template<size_t IndentSize>
struct Indenter
{
    Indenter(Indent<IndentSize>& indent) : indent_{ indent } { ++indent_; }
    ~Indenter() { --indent_; }
private:
    Indent<IndentSize>& indent_;
};

// Print the summary of the performance data.
// Prints the total execution time for each of the compiler passes and activities in CA Pass.
// Then prints the percentage of each activity. Activities are displayed as children of
// their parent activity. For example, AST Creation and AST Clients are displayed under CA Pass.
// Activities' performance numbers are percentages over their immediate parent activity.
// So, sum of percentages of all siblings should equal to 100%.
// Also prints the number of TUs with CA Pass greater than FE Pass by over 600%, 300%, or 150%.
void PerfDataCollector::PrintSummary()
{
    struct Total
    {
        milliseconds FEPass;                    // FrontEnd pass
        milliseconds CAPass;                    // CodeAnalysis pass
        milliseconds BEPass;                    // BackEnd pass
        milliseconds ASTCreation;               // AST creation for Code Analysis
        milliseconds ASTClients;                // COM clients for Code Analysis, including PREfast
        milliseconds CAFunction;                // PREfast's Code Analysis of functions, including analysis of function by all of its plug-ins
        milliseconds FPAFunction;               // PREfast's path-sensitive Code Analysis of functions (FPA = Function Path Analysis)
        milliseconds EspXCfgBuild;              // CFG Building for functions to be used by EspXEngine and its extensions
        milliseconds EspXAllChecks;             // Analysis of functions by EspXEngine and its extensions
        milliseconds EspXPathSensitiveChecks;   // Path-sensitive analysis of functions by EspXEngine and its extensions
    } total{};

    size_t ignoredTUs{};
    size_t TUsWithCAPassGE600Percent{};
    size_t TUsWithCAPassGE300Percent{};
    size_t TUsWithCAPassGE150Percent{};

    for (const auto& [pathId, data] : perfDataPerTu)
    {
        // If CA Pass is less than FE Pass, it is likely CA Pass is not captured properly.
        // Ignore the record.
        if (data.FEPass > microseconds::zero() && data.CAPass < data.FEPass)
        {
            ++ignoredTUs;
            continue;
        }

        total.FEPass += std::chrono::duration_cast<milliseconds>(data.FEPass);
        total.BEPass += std::chrono::duration_cast<milliseconds>(data.BEPass);
        total.CAPass += std::chrono::duration_cast<milliseconds>(data.CAPass);
        total.ASTCreation += std::chrono::duration_cast<milliseconds>(data.ASTCreation);
        total.ASTClients += std::chrono::duration_cast<milliseconds>(data.ASTClients);
        total.CAFunction += std::chrono::duration_cast<milliseconds>(data.CAFunction);
        total.FPAFunction += std::chrono::duration_cast<milliseconds>(data.FPAFunction);
        total.EspXCfgBuild += std::chrono::duration_cast<milliseconds>(data.EspXCfgBuild);
        total.EspXAllChecks += std::chrono::duration_cast<milliseconds>(data.EspXAllChecks);
        total.EspXPathSensitiveChecks += std::chrono::duration_cast<milliseconds>(data.EspXPathSensitiveChecks);

        //
        // Count important records
        //

        // TUs with CA Pass greater than FE Pass by over 600%, 300%, or 150% and more.
        if (data.FEPass > microseconds::zero())
        {
            const auto ratio = data.CAPass.count() / static_cast<double>(data.FEPass.count());
            if (ratio >= 6.0)
                ++TUsWithCAPassGE600Percent;
            else if (ratio >= 3.0)
                ++TUsWithCAPassGE300Percent;
            else if (ratio >= 1.5)
                ++TUsWithCAPassGE150Percent;
        }
    }

    // Output indentation
    Indent<2> indent;

    const Indenter _{ indent };

    // The number of TUs compiled
    std::wcout << L"Number of TUs compiled: " << (perfDataPerTu.size() - ignoredTUs) << L"\n";
    // The number of TUs ignored due to CA Pass less than FE Pass
    if (ignoredTUs > 0)
        std::wcout << L"Number of TUs excluded (files not analyzed or had analysis error): " << ignoredTUs << L"\n";

    // Total Execution Time (milliseconds):
    std::wcout << L"Total Execution Time (milliseconds):" << L"\n"
        << indent << PerfData::PerfDataHeader << L"\n"
        << indent << total.FEPass.count() << L", " << total.BEPass.count() << L", " << total.CAPass.count() << L", "
        << total.ASTCreation.count() << L", " << total.ASTClients.count() << L", " << total.CAFunction.count() << L", "
        << total.FPAFunction.count() << L", " << total.EspXCfgBuild.count() << L", " << total.EspXAllChecks.count() << L", "
        << total.EspXPathSensitiveChecks.count() << std::endl;

    // Total pass = FE Pass + CA Pass + BE Pass
    const auto totalPass = total.FEPass + total.CAPass + total.BEPass;
    if (totalPass == milliseconds::zero())  // Very unlikely, but just in case.
        return;

    {
        WcoutStateRestorer restorer;

        std::wcout << std::fixed << std::setprecision(2);
        // Duration of each compiler pass in all compiler passes
        const auto totalPassTime = static_cast<double>(totalPass.count());
        std::wcout
            << L"Compiler Passes:\n"
            << indent << L"Front End Pass = " << (total.FEPass.count() / totalPassTime) * 100 << L"%\n"
            << indent << L"Back End Pass = " << (total.BEPass.count() / totalPassTime) * 100 << L"%\n"
            << indent << L"Code Analysis Pass = " << (total.CAPass.count() / totalPassTime) * 100 << L"%\n";
        if (total.CAPass > milliseconds::zero())
        {
            // Duration of each CA Pass activity in CA Pass
            const Indenter _{ indent };
            const auto totalCAPassTime = static_cast<double>(total.CAPass.count());
            std::wcout
                << indent << L"Compilation + Miscellaneous = " << ((total.CAPass - total.ASTCreation - total.ASTClients).count() / totalCAPassTime) * 100 << L"%\n"
                << indent << L"AST Creation = " << (total.ASTCreation.count() / totalCAPassTime) * 100 << L"%\n"
                << indent << L"All AST Clients = " << (total.ASTClients.count() / totalCAPassTime) * 100 << L"%\n";
            if (total.ASTClients > milliseconds::zero())
            {
                // Duration of activities in PREfast
                const Indenter _{ indent };
                const auto allAstClientsTime = static_cast<double>((total.ASTClients).count());
                std::wcout
                    << indent << L"Miscellaneous = " << ((total.ASTClients - total.CAFunction).count() / allAstClientsTime) * 100 << L"%\n"
                    << indent << L"Function Analysis = " << (total.CAFunction.count() / allAstClientsTime) * 100 << L"%\n";
                if (total.CAFunction > milliseconds::zero())
                {
                    // Duration of each component's function analysis in PREfast
                    const Indenter _{ indent };
                    const auto functionAnalysisTime = static_cast<double>(total.CAFunction.count());
                    std::wcout
                        << indent << L"Miscellaneous = " << ((total.CAFunction - total.FPAFunction - total.EspXCfgBuild - total.EspXAllChecks).count() / functionAnalysisTime) * 100 << L"%\n"
                        << indent << L"PREfast's FPA Analysis = " << (total.FPAFunction.count() / functionAnalysisTime) * 100 << L"%\n"
                        << indent << L"EspX CFG Building = " << (total.EspXCfgBuild.count() / functionAnalysisTime) * 100 << L"%\n"
                        << indent << L"EspX All Analysis = " << (total.EspXAllChecks.count() / functionAnalysisTime) * 100 << L"%\n";
                    if (total.EspXAllChecks > milliseconds::zero())
                    {
                        // Duration of path-sensitive vs data-flow function analysis in EspXEngine
                        const Indenter _{ indent };
                        const auto espxFunctionAnalysisTime = static_cast<double>(total.EspXAllChecks.count());
                        std::wcout
                            << indent << L"Path-sensitive Analysis = " << (total.EspXPathSensitiveChecks.count() / espxFunctionAnalysisTime) * 100 << L"%\n"
                            << indent << L"Data-flow Analysis + Miscellaneous = " << ((total.EspXAllChecks - total.EspXPathSensitiveChecks).count() / espxFunctionAnalysisTime) * 100 << L"%\n";
                    }
                }
            }
        }

        std::wcout.flush();
    }

    // Print the number of TUs with CA Pass greater than FE Pass
    if (TUsWithCAPassGE600Percent > 0 || TUsWithCAPassGE300Percent > 0 || TUsWithCAPassGE150Percent > 0)
    {
        std::wcout << L"Number of TUs with long Code Analysis Pass compared to Front End Pass:\n";
        std::wcout << indent << L"600% or more: " << TUsWithCAPassGE600Percent << "\n";
        std::wcout << indent << L"300% or more: " << TUsWithCAPassGE300Percent << "\n";
        std::wcout << indent << L"150% or more: " << TUsWithCAPassGE150Percent << std::endl;
    }
}