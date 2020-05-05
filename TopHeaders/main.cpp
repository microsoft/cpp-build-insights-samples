#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <CppBuildInsights.hpp>

using namespace Microsoft::Cpp::BuildInsights;
using namespace Activities;

class TopHeaders : public IAnalyzer
{
    struct FileInfo
    {
        std::chrono::nanoseconds TotalParsingTime;
        std::string Path;
        std::unordered_set<unsigned long long> PassIds;

        bool operator<(const FileInfo& other) const {
            return TotalParsingTime > other.TotalParsingTime;
        }
    };

public:
    TopHeaders(int headerCountToDump):
        headerCountToDump_{headerCountToDump  > 0 ? 
            headerCountToDump : 5},
        frontEndAggregatedDuration_{0},
        fileInfo_{}
    {}

    AnalysisControl OnStopActivity(const EventStack& eventStack) override
    {

        switch (eventStack.Back().EventId())
        {
        case EVENT_ID_FRONT_END_FILE:
            MatchEventStackInMemberFunction(eventStack, this, 
                &TopHeaders::OnStopFile);
            break;

        case EVENT_ID_FRONT_END_PASS:
            // Keep track of the overall front-end aggregated duration.
            // We use this value when determining how significant is
            // a header's total parsing time when compared to the total
            // front-end time.
            frontEndAggregatedDuration_ += eventStack.Back().Duration();
            break;

        default:
            break;
        }

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnStopFile(FrontEndPass fe, FrontEndFile file)
    {
        // Make the path lowercase for comparing
        std::string path = file.Path();

        std::transform(path.begin(), path.end(), path.begin(),
            [](unsigned char c) { return std::tolower(c); });

        auto result = fileInfo_.try_emplace(std::move(path), FileInfo{});

        auto it = result.first;
        bool wasInserted = result.second;

        FileInfo& fi = it->second;

        fi.PassIds.insert(fe.EventInstanceId());
        fi.TotalParsingTime += file.Duration();

        if (result.second) {
            fi.Path = file.Path();
        }

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnEndAnalysis() override
    {
        using namespace std::chrono;

        auto topHeaders = GetTopHeaders();

        if (headerCountToDump_ == 1) {
            std::cout << "Top header file:";
        }
        else {
            std::cout << "Top " << headerCountToDump_ <<
                " header files:";
        }

        std::cout << std::endl << std::endl;

        for (auto& info : topHeaders)
        {
            double frontEndPercentage = 
                static_cast<double>(info.TotalParsingTime.count()) /
                frontEndAggregatedDuration_.count() * 100.;

            std::cout << "Aggregated Parsing Duration: " <<
                duration_cast<milliseconds>(
                    info.TotalParsingTime).count() << 
                " ms" << std::endl;
            std::cout << "Front-End Time Percentage:   " <<
                std::setprecision(2) << frontEndPercentage << "% " << 
                std::endl;
            std::cout << "Inclusion Count:             " <<
                info.PassIds.size() << std::endl;
            std::cout << "Path: " <<
                info.Path << std::endl << std::endl;
        }

        return AnalysisControl::CONTINUE;
    }

private:
    std::multiset<FileInfo> GetTopHeaders()
    {
        std::multiset<FileInfo> topHeaders;

        for (auto& p : fileInfo_)
        {
            if (topHeaders.size() < headerCountToDump_) {
                topHeaders.insert(p.second);
            }
            else
            {
                auto itLast = --topHeaders.end();

                if (p.second.TotalParsingTime >
                    itLast->TotalParsingTime)
                {
                    topHeaders.insert(p.second);
                    topHeaders.erase(itLast);
                }
            }
        }

        return topHeaders;
    }

    int headerCountToDump_;

    std::chrono::nanoseconds frontEndAggregatedDuration_;

    std::unordered_map<std::string, FileInfo> fileInfo_;
};

int main(int argc, char* argv[])
{
    if (argc <= 1) return -1;

    int headerCountToDump = 0;

    if (argc >= 3) {
        headerCountToDump = std::atoi(argv[2]);
    }

    TopHeaders th{ headerCountToDump };

    auto group = MakeStaticAnalyzerGroup(&th);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 1;
    return Analyze(argv[1], numberOfPasses, group);
}