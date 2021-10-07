#include <algorithm>
#include <chrono>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <CppBuildInsights.hpp>

using namespace Microsoft::Cpp::BuildInsights;
using namespace Activities;
using namespace SimpleEvents;

class RecursiveTemplateInspector : public IAnalyzer
{
    struct TemplateSpecializationInfo
    {
        std::chrono::nanoseconds TotalInstantiationTime;
        size_t InstantiationCount;
        size_t MaxDepth;
        std::string RootSpecializationName;
        std::wstring File;

        std::unordered_set<unsigned long long> VisitedInstantiations;

        bool operator<(const TemplateSpecializationInfo& other) const {
            return TotalInstantiationTime > other.TotalInstantiationTime;
        }
    };

public:
    RecursiveTemplateInspector(int specializationCountToDump):
        specializationCountToDump_{
            specializationCountToDump > 0 ? specializationCountToDump : 5 }
    {
    }

    AnalysisControl OnStopActivity(const EventStack& eventStack)
        override
    {
        MatchEventStackInMemberFunction(eventStack, this,
            &RecursiveTemplateInspector::OnTemplateRecursionTreeBranch);

        return AnalysisControl::CONTINUE;
    }

    AnalysisControl OnSimpleEvent(const EventStack& eventStack)
        override
    {
        MatchEventStackInMemberFunction(eventStack, this,
            &RecursiveTemplateInspector::OnSymbolName);

        return AnalysisControl::CONTINUE;
    }

    void OnTemplateRecursionTreeBranch(FrontEndPass fe, 
        TemplateInstantiationGroup recursionTreeBranch)
    {
        const TemplateInstantiation& root = recursionTreeBranch[0];
        const TemplateInstantiation& current = recursionTreeBranch.Back();

        auto& info = rootSpecializations_[root.SpecializationSymbolKey()];

        auto& visitedSet = info.VisitedInstantiations;

        if (visitedSet.find(current.EventInstanceId()) == visitedSet.end())
        {
            // We have a new unvisited branch. Update the max depth of the
            // recursion tree.

            info.MaxDepth = std::max(info.MaxDepth, recursionTreeBranch.Size());

            for (size_t idx = recursionTreeBranch.Size(); idx-- > 0;)
            {
                const TemplateInstantiation& ti = recursionTreeBranch[idx];

                auto p = visitedSet.insert(ti.EventInstanceId());

                bool wasVisited = !p.second;

                if (wasVisited)
                {
                    // Stop once we reach a visited template instantiation,
                    // because its parents will also have been visited.
                    break;
                }

                ++info.InstantiationCount;
            }
        }

        if (recursionTreeBranch.Size() != 1) {
            return;
        }

        // The end of a hierarchy's instantiation corresponds to the stop
        // event of the root specialization's instantiation. When we reach
        // that point, we update the total instantiation time of the hierarchy.

        info.TotalInstantiationTime = root.Duration();

        info.File = fe.InputSourcePath() ? fe.InputSourcePath() :
            fe.OutputObjectPath();

        visitedSet.clear();
    }

    void OnSymbolName(SymbolName symbolName)
    {
        auto it = rootSpecializations_.find(symbolName.Key());

        if (it == rootSpecializations_.end()) {
            return;
        }

        it->second.RootSpecializationName = symbolName.Name();
    }

    AnalysisControl OnEndAnalysis() override
    {
        using namespace std::chrono;

        auto topSpecializations = GetTopInstantiations();
        
        if (specializationCountToDump_ == 1) {
            std::cout << "Top template instantiation hierarchy:";
        }
        else {
            std::cout << "Top " << specializationCountToDump_ << 
                " template instantiation " << "hierarchies";
        }
            
        std::cout << std::endl << std::endl;

        for (auto& info : topSpecializations)
        {
            std::wcout << "File:           " << 
                info.File << std::endl;
            std::cout  << "Duration:       " << 
                duration_cast<milliseconds>(
                    info.TotalInstantiationTime).count() << 
                " ms" << std::endl;
            std::cout  << "Max Depth:      " << 
                info.MaxDepth << std::endl;
            std::cout  << "Instantiations: " << 
                info.InstantiationCount << std::endl;
            std::cout  << "Root Name:      " << 
                info.RootSpecializationName << std::endl << std::endl;
        }

        return AnalysisControl::CONTINUE;
    }

private:
    std::multiset<TemplateSpecializationInfo> GetTopInstantiations()
    {
        std::multiset<TemplateSpecializationInfo> topSpecializations;

        for (auto& p : rootSpecializations_)
        {
            if (topSpecializations.size() < specializationCountToDump_) {
                topSpecializations.insert(p.second);
            }
            else 
            {
                auto itLast = --topSpecializations.end();

                if (p.second.TotalInstantiationTime >=
                    itLast->TotalInstantiationTime)
                {
                    topSpecializations.erase(itLast);
                    topSpecializations.insert(p.second);
                }
            }
        }

        return topSpecializations;
    }

    // A hash table that stores information about template instantiations
    // that are at the root of a recursive instantiation hierarchy.
    std::unordered_map<unsigned long long, TemplateSpecializationInfo> rootSpecializations_;

    int specializationCountToDump_;
};

int main(int argc, char* argv[])
{
    if (argc <= 1) return -1;

    int specializationCountToDump = 0;

    if (argc >= 3) {
        specializationCountToDump = std::atoi(argv[2]);
    }

    RecursiveTemplateInspector rti{specializationCountToDump};

    auto group = MakeStaticAnalyzerGroup(&rti);

    // argv[1] should contain the path to a trace file
    int numberOfPasses = 1;
    return Analyze(argv[1], numberOfPasses, group);
}