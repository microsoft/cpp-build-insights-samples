#include "PerfDataCollector.h"
#include <filesystem>
#include <cassert>

// In this project, we use the following terms / acronyms throughout the project:
//  - FE: Front End
//  - BE: Back End
//  - CA: Code Analysis
//  - AST: Abstract Syntax Tree
//  - CFG: Control Flow Graph
//  - PREfast: Main driver of Code Analysis
//  - PREfast Plug-ins: Clients of PREfast that can process / analyze ASTs
//  - PREfast FPA: PREfast's built in checker that performs path-sensitive analysis
//  - EspXEngine: Main PREfast plug-in that drives other checker extensions
//  - EspXEngine Extensions: Clients of EspXEngine that can process / analyze CFGs
//  - EspXEngine Path-sensitive Checks: EspXEngine's built-in or extensions that perform perform path-sensitive analysis

enum class OutputFormat
{
    Summary,    // Summary only. Default format
    CSV,        // CSV output of per-TU performance data
    Both        // Both Summary and CSV. Summary will be printed first.
};

struct Options
{
    bool verbose{};                             // Verbose output
    OutputFormat outputFormat{};                // Output format, defaults to OptputFormat::Summary
    std::vector<const char*> traceFilePaths{};  // Paths to the trace files to process

    void PrintUsage(const char* programName)
    {
        std::filesystem::path path{ programName };
        std::wcout << L"Usage: " << path.replace_extension().filename()
            << L" [-v[erbose]] [-f[ormat]:Summary|CSV|Both]] <paths to trace files>\n"
            << L"    -v[erbose] : Print verbose output. Optional.\n"
            << L"    -f[ormat]:<format> : Output format (Summary, CSV, or Both). Optional. Defaults to Summary.\n"
            << L"    <paths to trace files> : Paths to one or more ETW trace files, separated by space. Required.\n"
            << L"Option names and values are case-insensitive." << std::endl;
    }

    // Returns zero if the program should continue, non-zero if it should exit.
    int ParseCommandLine(int argc, char* argv[])
    {
        if (argc < 2)
        {
            PrintUsage(argv[0]);
            return -1;
        }

        auto filePathArgIndex = 1;

        for (int i = 1; i < argc; ++i)
        {
            const auto arg = argv[i];
            if (strlen(arg) < 2 || !(arg[0] == '-' || arg[0] == '/'))
            {
                continue;
            }

            if (_stricmp(&arg[1], "v") == 0 || _stricmp(&arg[1], "verbose") == 0)
            {
                verbose = true;
                ++filePathArgIndex;
            }
            else if (_strnicmp(&arg[1], "f:", 2) == 0 || _strnicmp(&arg[1], "format:", 7) == 0)
            {
                auto formatArgOffset = 3;
                if (arg[2] == 'o' || arg[2] == 'O')
                    formatArgOffset += 5;
                if (strlen(&arg[formatArgOffset]) == 0)
                {
                    std::wcerr << L"Output format is not specified." << std::endl;
                    return -1;
                }

                // Get output format
                if (_stricmp(&arg[formatArgOffset], "summary") == 0)
                {
                    outputFormat = OutputFormat::Summary;
                }
                else if (_stricmp(&arg[formatArgOffset], "csv") == 0)
                {
                    outputFormat = OutputFormat::CSV;
                }
                else if (_stricmp(&arg[formatArgOffset], "both") == 0)
                {
                    outputFormat = OutputFormat::Both;
                }
                else
                {
                    std::wcerr << L"Unknown output format: " << &arg[formatArgOffset] << std::endl;
                    return -1;
                }
                ++filePathArgIndex;
            }
            else if (_stricmp(&arg[1], "h") == 0 || _stricmp(&arg[1], "help") == 0)
            {
                PrintUsage(argv[0]);
                return 0;
            }
            else
            {
                std::wcerr << L"Unknown option: " << arg << std::endl;
                return -1;
            }
        }

        if (argc <= filePathArgIndex)
        {
            PrintUsage(argv[0]);
            return -1;
        }

        for (int i = filePathArgIndex; i < argc; ++i)
        {
            auto filePath = argv[i];

            // Check the path to the trace file
            if (!std::filesystem::exists(filePath))
            {
                std::wcerr << L"File not found: " << filePath << std::endl;
                return -1;
            }

            traceFilePaths.push_back(filePath);
        }

        return 0;
    }
};

int main(int argc, char *argv[])
{
    Options options;
    if (const auto result = options.ParseCommandLine(argc, argv); result != 0)
        return result;

    PerfDataCollector perfDataCollector(options.verbose);

    auto analyzers = MakeStaticAnalyzerGroup(&perfDataCollector);

    assert(!options.traceFilePaths.empty());
    for (auto traceFilePath : options.traceFilePaths)
    {
        std::wcout << L"Analyzing " << traceFilePath << L"..." << std::endl;

        constexpr int NumberOfPasses = 1;
        const auto result = Analyze(traceFilePath, NumberOfPasses, analyzers);
        if (result != RESULT_CODE_SUCCESS)
        {
            std::wcerr << L"Failed to analyze the trace file \"" << traceFilePath << L"\": ";

            switch (result)
            {
            case RESULT_CODE_FAILURE_DROPPED_EVENTS:
                std::wcerr << L"Log is missing some important events." << std::endl;
                break;
            case RESULT_CODE_FAILURE_INVALID_INPUT_LOG_FILE:
                std::wcerr << L"Input log file is invalid." << std::endl;
                break;
            case RESULT_CODE_FAILURE_NO_CONTEXT_INFO_AVAILABLE:
                std::wcerr << L"Failed to get context information from the trace file." << std::endl;
                break;
            default:
                std::wcerr << L"Error Code = " << static_cast<int>(result) << std::endl;
                break;
            }

            return -1;
        }
    }

    // Print Summary, CSV, or both as requested.
    switch (options.outputFormat)
    {
    case OutputFormat::Summary:
        perfDataCollector.PrintSummary();
        break;
    case OutputFormat::CSV: 
        perfDataCollector.PrintRecords();
        break;
    case OutputFormat::Both:
        perfDataCollector.PrintSummary();
        std::wcout << L"\n\n";
        perfDataCollector.PrintRecords();
        break;
    default:
        break;  // Should not be reached
    }

    return 0;
}