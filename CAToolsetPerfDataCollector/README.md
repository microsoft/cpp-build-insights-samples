---
page_type: sample
languages:
- C++
products:
- CodeAnalysisPerfDataCollector
description: ""
urlFragment: "CAToolsetPerfDataCollector"
---

# Code Analysis Performance Data Collector

![License](https://img.shields.io/badge/license-MIT-green.svg)

This sample BuildInsights SDK application reads one or more Event Trace Log (ETW) files and prints summary of Code Analysis (CA) time relative to the overall execution time. It can also print execution time of each sub-process for each source file.  

## Prerequisites

This sample BuildInsights SDK application requires MSVC compiler and MSVC CA toolsets that are available in Visual Studio 2022 17.11 or later. It also requires BuildInsights SDK version 1.4.2 or later.

## Overview

We have added ETW events to MSVC compiler and MSVC CA toolsets to measure execution times of various steps during CA. These events allow us to correlate performance of CA with the rest of the build process – for example, “CA step X took y-percent of overall compilation time”.

The ability to collect and analyze CA performance data will allow users to measure CA performance for various configurations - such as different sets of rules enabled for CA, enabling CA during build, etc. - to find out the most efficient CA configurations during various phases of product development cycle.

## Running Code Analysis Performance Data Collector

This application can process one or more ETW log files (*.etl files) at once to produce a combined summary report. This application can also optionally output the performance data per translation unit in CSV format.

- Summary Report

  It is a summary report for the overall performance of CA, providing relative time spent in CA steps at various phases of the compilation process. This is the default output format that provides a quick overview of CA performance. 

- Per Translation Unit data records in CSV Format

  This is a set of rows with performance numbers (in unit of milliseconds) - each row corresponding to each of the Translation units that are compiled and analyzed – in CSV (comma-separated-values) format. This allows users to process the data further and create various reports using tools like MS Excel. 

Running the application without any option or input will provide help on the options it supports:

```
c:\bisamples\out\x64\Debug\CodeAnalysisPerfDataCollector> CodeAnalysisPerfDataCollector.exe

Usage: "CodeAnalysisPerfDataCollector" [-v[erbose]] [-f[ormat]:Summary|CSV|Both]] <paths to trace files>
    -v[erbose] : Print verbose output. Optional.
    -f[ormat]:<format> : Output format (Summary, CSV, or Both). Optional. Defaults to Summary.
    <paths to trace files> : Paths to one or more ETW trace files, separated by space. Required.
Option names and values are case-insensitive.
```

By default it will print CA performance summary report. If "-f:CSV" option is specified, it will print per translation unit performance data in CSV format for each and every translation unit that is built and analyzed. If "-f:both" option is specified, it will print both CA performance summary report and per translation unit performance data.

## Example Outputs

### CA Performance Summary Report
This is a example output of CA performance data summary report. Please note that the actual execution times and their relative percentage numbers will vary project by project, as well as CA configurations.

Percentage number for each child is relative to its immediate parent. So, the sum of the percentage numbers for all of the siblings should always equal 100%.

```
Number of TUs successfully analyzed: 3289 
Number of TUs excluded (files not analyzed or had analysis error): 1893 
Total Execution Time (milliseconds): 
  Front End Pass, BackEnd Pass, Code Analysis Pass, AST Creation, AST Clients, Function Analysis, FPA Function Analysis, EspX CFG Build, EspX Function Analysis, EspX Path-sensitive Analysis 
  6139105, 1184276, 57333281, 382762, 48323001, 46771695, 4680989, 3837409, 36653783, 34256891 
Compiler Passes [percentages are "percentage of parent" ("percentage of total")]: 
  Front End Pass = 9.49% 
  Back End Pass = 1.83% 
  Code Analysis Pass = 88.67% 
    Compilation + Miscellaneous = 15.05% (13.34%) 
    AST Creation = 0.67% (0.59%) 
    All AST Clients = 84.28% (74.74%) 
      Miscellaneous = 3.21% (2.40%) 
      Function Analysis = 96.79% (72.34%) 
        Miscellaneous = 3.42% (2.47%) 
        PREfast's Function Path Analysis = 10.01% (7.24%) 
        EspX CFG Building = 8.20% (5.94%) 
        EspX All Analysis = 78.37% (56.69%) 
          Path-sensitive Analysis = 93.46% (52.98%) 
          Data-flow Analysis + Miscellaneous = 6.54% (3.71%) 
Number of TUs with long Code Analysis Pass compared to Front End Pass: 
  600% or more: 1837 (55.85%) 
  300% or more: 945 (28.73%) 
  150% or more: 418 (12.71%) 
  Less than 150%: 89 (2.71%) 
```

### Per translation unit Performance Data in CSV Format

Here is an example of the per translation unit performance data output. First row is the column header, and the rest are per translation unit performance data - i.e., execution times of each step in milliseconds.

This can be imported into tools like MS Excel to create various reports.

```csv
File Path, Front End Pass, BackEnd Pass, Code Analysis Pass, AST Creation, AST Clients, Function Analysis, FPA Function Analysis, EspX CFG Build, EspX Function Analysis, EspX Path-sensitive Analysis
d:\test\file1.cpp, 253976, 111859, 5077991, 14019, 249835, 229841, 84557, 0, 0, 0
d:\test\file2.cpp, 0, 0, 108231, 56, 467, 312, 123, 0, 0, 0
```
