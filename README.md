---
page_type: sample
languages:
- C++
products:
- cpp-build-insights
description: "This repository provides buildable and runnable samples for the C++ Build Insights SDK. Use it as a learning resource."
urlFragment: "cpp-build-insights-samples"
---

# C++ Build Insights SDK samples

![License](https://img.shields.io/badge/license-MIT-green.svg)

This repository provides buildable and runnable samples for the C++ Build Insights SDK. Use it as a learning resource.

## Contents

| Sample            | Description                                |
|-------------------|--------------------------------------------|
| BottleneckCompileFinder | Finds CL invocations that are bottlenecks and don't use /MP. |
| [CAToolsetPerfDataCollector](CAToolsetPerfDataCollector/README.md) | Prints summary of Code Analysis time relative to overall execution time. Optionally prints the summary for each source file. |
| FunctionBottlenecks | Prints a list of functions that are code generation bottlenecks within their CL or Link invocation. |
| LongCodeGenFinder | Lists the functions that take more than 500 milliseconds to generate in your entire build. |
| RecursiveTemplateInspector | Identifies costly recursive template instantiations. |
| TopHeaders | Determines which headers you might want to precompile. |
| LongModuleFinder | Identifies costly module interface IFC creation. Requires trace with code built using MSVC version 16.10 or later and using SDK version Microsoft.Cpp.BuildInsights 1.2.0 or later. |
| LongHeaderUnitFinder | Identifies costly header unit IFC creation. Requires trace with code built using MSVC version 16.10 or later and using SDK version Microsoft.Cpp.BuildInsights 1.2.0 or later. |
| LongPrecompiledHeaderFinder | Identifies costly precompiled header (PCH) IFC creation. Requires trace with code built using MSVC version 16.10 or later and using SDK version Microsoft.Cpp.BuildInsights 1.2.0 or later. |

## Prerequisites

In order to build and run the samples in this repository, you need:

- Visual Studio 2022 and above (requires platform toolset version v143).
- BuildInsights SDK version 1.4.2 or later
- Windows 10 and above. Windows 11 is recommended.

CAToolsetPerfDataCollector requires compiler and Code Analysis toolsets available in Visual Studio 2022 17.11 and later.

## Build steps

1. Clone the repository on your machine.
1. Open the Visual Studio solution file. Each sample is a separate project within the solution.
1. All samples rely on the C++ Build Insights SDK NuGet package. Restore NuGet packages and accept the license for the SDK.
1. Build the desired configuration for the samples that interest you. Available platforms are x86 and x64, and available configurations are *Debug* and *Release*.
1. Samples will be built in their own directory following this formula: `{RepositoryRoot}\out\{SampleName}`.

## Running the samples

1. The samples require *CppBuildInsights.dll* and *KernelTraceControl.dll* to run. These files are available in the C++ Build Insights NuGet package. When building samples, these files are automatically copied next to them in their respective output directory. If you are going to move a sample around on your machine, please be sure to move these DLL's along with it.
1. Collect a trace of the build you want to analyze with the sample. You can do this using two methods:
    1. Use vcperf:
        1. Open an elevated x64 Native Tools Command Prompt.
        1. Run the following command: `vcperf /start MySessionName`
            - It can be used with /noadmin option to run without elevation. Running vcperf without option or input displays supported options. 
        1. Build your project. You do not need to use the same command prompt for building.
        1. Run the following command: `vcperf /stopnoanalyze MySessionName outputTraceFile.etl`
    1. Programmatically: see the [C++ Build Insights SDK](https://docs.microsoft.com/cpp/build-insights/reference/sdk/overview?view=vs-2019) documentation for details.
1. Invoke the sample, passing your trace as the first parameter.

[CAToolsetPerfDataCollector](CAToolsetPerfDataCollector/README.md) has options to output results in different formats. Run the application without parameter or option to print the help on the options. Please find details about this sample in the [README.md](CAToolsetPerfDataCollector/README.md) for it.


## Contributing

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit [https://cla.opensource.microsoft.com](https://cla.opensource.microsoft.com).

When you submit a pull request, a CLA bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
