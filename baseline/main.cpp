#include "args.h"

#include "dict.h"
#include "swipe.h"

#include <iostream>
#include <fstream>
#include <locale>
#include <codecvt>
#include <string>

int main(int argc, const char** argv) {
    std::string dictPath;
    std::string tasksPath;

    size_t clustersLimit = 20;
    size_t clustersCount = 1000;
    size_t iterationsCount = 5;

    {
        TArgsParser argsParser;
        argsParser.AddHandler("dict", &dictPath, "path to dictionary").Required();
        argsParser.AddHandler("tasks", &tasksPath, "path to tasks").Required();

        argsParser.AddHandler("clusters-limit", &clustersCount, "number of clusters for lookup").Optional();
        argsParser.AddHandler("clusters-count", &clustersLimit, "number of clusters").Optional();
        argsParser.AddHandler("iterations", &iterationsCount, "number of iterations").Optional();

        argsParser.DoParse(argc, argv);
    }

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    TDict dict;
    {
        std::ifstream dictIn(dictPath);
        char line[10000];
        while (dictIn.getline(line, 10000)) {
            const std::wstring wideLine = converter.from_bytes(line);
            dict.Words.push_back(wideLine);
        }
    }

    TKeyboardLayout layout;
    std::ifstream input(tasksPath);
    char line[100000];

    size_t correct = 0;
    size_t processed = 0;
    while (input.getline(line, 100000)) {
        const std::wstring wideLine = converter.from_bytes(line);

        if (layout.KeyInfos.empty()) {
            layout.LoadFromString(wideLine);
            std::cerr << "making clusters..." << std::endl;
            layout.MakeClusters(dict, clustersCount, iterationsCount);
            std::cerr << "building vp tree..." << std::endl;
            layout.BuildVPTree(dict);
            std::cerr << "built all!" << std::endl;
        }

        const TSwipeEvent swipeEvent = TSwipeEvent::FromString(wideLine);
        const std::vector<std::pair<double, std::wstring>> candidates = layout.GetCandidates(swipeEvent, dict, clustersLimit);
        const std::wstring candidate = candidates.front().second;
        if (candidate == swipeEvent.Target) {
            ++correct;
        }

        std::cout << converter.to_bytes(candidate) << "\n";

        ++processed;
        if (processed % 10 == 0) {
            std::cerr << processed << " " << "processed..." << std::endl;
        }
    }

    std::cerr << "accuracy: " << (double)correct / processed << std::endl;
}
