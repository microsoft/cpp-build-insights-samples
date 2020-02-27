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
| LongCodeGenFinder | Lists the functions that take more than 500 milliseconds to generate in your entire build. |

## Prerequisites

In order to build and run the samples in this repository, you need:

- Visual Studio 2017 and above.
- Windows 8 and above.

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
        1. Open an elevated x64 Native Tools Command Prompt for VS 2019.
        1. Run the following command: `vcperf /start MySessionName`
        1. Build your project. You do not need to use the same command prompt for building.
        1. Run the following command: `vcperf /stopnoanalyze MySessionName outputTraceFile.etl`
    1. Programmatically: see the [C++ Build Insights SDK (ADD LINK)]() documentation for details.
1. Invoke the sample, passing your trace as the first parameter.

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit [https://cla.opensource.microsoft.com](https://cla.opensource.microsoft.com).

When you submit a pull request, a CLA bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
