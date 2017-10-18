#pragma once

#include "dict.h"

#include <string>

#include <unordered_map>
#include <numeric>
#include <functional>

#include <fstream>

struct TSymbolInfo {
    wchar_t Symbol;
    unsigned int SumTimestamps;
    double SumWeightedTimestamps;
};

struct TKeyInfo {
    TCoord LeftUpper;

    double Width;
    double Height;

    TCoord Center() const {
        return TCoord(LeftUpper.X + Width / 2, LeftUpper.Y + Height / 2);
    }
};

struct TSwipeEvent {
    std::wstring Target;
    std::vector<TCoord> Points;

    static TSwipeEvent FromString(const std::wstring& source) {
        TSwipeEvent result;

        std::string xString, yString;

        size_t fieldNumber = 0;
        bool sawDelim = false;
        for (size_t i = 0; i < source.size(); ++i) {
            if (source[i] == '\t') {
                ++fieldNumber;
                continue;
            }
            if (fieldNumber == 2) {
                result.Target += source[i];
            }
            if (fieldNumber != 1) {
                continue;
            }

            if (source[i] == ':') {
                sawDelim = true;
                continue;
            }
            if (source[i] == ' ') {
                const int x = std::stoi(xString);
                const int y = std::stoi(yString);

                result.Points.push_back(TCoord(x, y));

                xString.clear();
                yString.clear();

                sawDelim = false;

                continue;
            }

            (sawDelim ? yString : xString) += source[i];
        }

        return result;
    }
};

struct TKeyboardLayout {
private:
    enum {
        EmbeddingLength = 50
    };
public:
    std::vector<wchar_t> Keys;
    std::vector<TShortEmbedding> ClusterEmbeddings;

    using TKeyInfosMap = std::unordered_map<wchar_t, TKeyInfo>;
    TKeyInfosMap KeyInfos;

    static std::vector<TCoord> ProducePoints(const std::vector<TCoord>& source, size_t neededPointsCount) {
        if (source.size() == 1) {
            return std::vector<TCoord>(neededPointsCount, source.front());
        }

        std::vector<double> distances;
        for (size_t i = 0; i + 1 < source.size(); ++i) {
            const double xDiff = source[i].X - source[i + 1].X;
            const double yDiff = source[i].Y - source[i + 1].Y;

            const double distance = xDiff * xDiff + yDiff * yDiff;
            distances.push_back(distance);
        }

        const double sumDistances = std::accumulate(distances.begin(), distances.end(), 0.);
        const double step = sumDistances / neededPointsCount;

        std::vector<TCoord> modifiedPoints(neededPointsCount);

        size_t segmentNumber = 0;

        double collectedDistance = 0.;
        double nextDistance = distances[segmentNumber];

        for (size_t point = 0; point < neededPointsCount; ++point) {
            collectedDistance += step;

            while (segmentNumber + 1 < distances.size() && collectedDistance > nextDistance) {
                ++segmentNumber;
                nextDistance += distances[segmentNumber];
            }

            const double passedSegmentDistance = collectedDistance - nextDistance + distances[segmentNumber];
            const double passedSegmentPart = passedSegmentDistance / (distances[segmentNumber] + 1e-10);

            const double x = source[segmentNumber].X * (1 - passedSegmentPart) + source[segmentNumber + 1].X * passedSegmentPart;
            const double y = source[segmentNumber].Y * (1 - passedSegmentPart) + source[segmentNumber + 1].Y * passedSegmentPart;

            modifiedPoints[point] = TCoord(x, y);
        }

        return modifiedPoints;
    }

    std::vector<std::pair<double, std::wstring>> GetCandidates(const TSwipeEvent& swipeEvent, const TDict& dict, const size_t clustersLimit) const {
        const std::vector<TCoord> points = MakePoints(swipeEvent);

        TShortEmbedding shortEmbedding;
        shortEmbedding.Coords = dict.ShortenEmbedding(points);

        double distanceLimit = 10000;

        std::vector<TShortEmbedding*> found = dict.ClustersVPTree->FindNearbyItems(shortEmbedding, distanceLimit, clustersLimit);

        while (found.empty() || found.size() == clustersLimit) {
            while (found.empty()) {
                found.clear();
                distanceLimit *= 2;
                found = dict.ClustersVPTree->FindNearbyItems(shortEmbedding, distanceLimit, clustersLimit);
            }
            while (found.size() == clustersLimit) {
                found.clear();
                distanceLimit *= 0.9;
                found = dict.ClustersVPTree->FindNearbyItems(shortEmbedding, distanceLimit, clustersLimit);
            }
        }

        std::vector<std::pair<double, std::wstring>> allCandidates;
        for (const TShortEmbedding* foundCluster : found) {
            const std::vector<TDict::TWordIndex>& cluster = dict.ClusterWords[foundCluster->Idx];
            for (const TDict::TWordIndex wordIndex : cluster) {
                const std::wstring& candidate = dict.Words[wordIndex];
                const double score = Score(candidate, points);

                allCandidates.push_back(std::make_pair(score, candidate));
            }
        }
        std::sort(allCandidates.begin(), allCandidates.end(), std::greater<>());
        if (allCandidates.size() > 10) {
            allCandidates.resize(10);
        }

        return allCandidates;
    }

    double Score(const std::wstring& candidate, const std::vector<TCoord> points) const {
        const std::vector<TCoord> candidatePoints = MakePoints(candidate);
        return Score(candidatePoints, points);
    }

    double Score(const std::wstring& candidate, const TSwipeEvent& swipeEvent) const {
        const std::vector<TCoord> points = MakePoints(swipeEvent);
        const std::vector<TCoord> candidatePoints = MakePoints(candidate);
        return Score(candidatePoints, points);
    }

    std::vector<TCoord> NeededPoints(const std::wstring text) const {
        std::vector<TCoord> neededPoints;
        for (size_t i = 0; i < text.size(); ++i) {
            TKeyInfosMap::const_iterator it = KeyInfos.find(text[i]);
            if (it == KeyInfos.end()) {
                continue;
            }

            neededPoints.push_back(it->second.Center());
        }
        return neededPoints;
    }

    std::vector<TCoord> MakePoints(const std::wstring text) const {
        return ProducePoints(NeededPoints(text), EmbeddingLength);
    }

    std::vector<TCoord> MakePoints(const TSwipeEvent& evt) const {
        return ProducePoints(evt.Points, EmbeddingLength);
    }

    double Distance(const TCoord& lhs, const TCoord& rhs) const {
        const double xDiff = lhs.X - rhs.X;
        const double yDiff = lhs.Y - rhs.Y;
        const double squaredDistance = xDiff * xDiff + yDiff * yDiff + 1e-5;

        return squaredDistance;
    }

    double Cosine(const TCoord& first, const TCoord& second, const TCoord& third) const {
        const double xFirstDiff = second.X - first.X;
        const double yFirstDiff = second.Y - first.Y;

        const double xSecondDiff = third.X - second.X;
        const double ySecondDiff = third.Y - second.Y;

        const double numerator = xFirstDiff * xSecondDiff + yFirstDiff * ySecondDiff;

        return numerator;
    }

    const std::vector<TCoord> GetInterestingPoints(const std::vector<TCoord>& originalPoints) const {
        std::vector<TCoord> result;

        result.push_back(originalPoints.front());
        for (size_t i = 0; i + 2 < originalPoints.size(); ++i) {
            if (Cosine(originalPoints[i], originalPoints[i + 1], originalPoints[i + 2]) < 0) {
                result.push_back(originalPoints[i + 1]);
            }
        }
        result.push_back(originalPoints.back());

        return result;
    }

    double AsymmetricDistance(const std::vector<TCoord>& lhs, const std::vector<TCoord>& rhs) const {
        size_t rLast = 0;
        double distance = 0.;
        for (size_t i = 0; i < lhs.size(); ++i) {
            double bestDistance = Distance(lhs[i], rhs[rLast]);
            size_t bestNext = rLast;
            for (size_t j = rLast + 1; j < rhs.size(); ++j) {
                const double curDistance = Distance(lhs[i], rhs[j]);
                if (curDistance < bestDistance) {
                    bestDistance = curDistance;
                    bestNext = j;
                }
            }
            rLast = bestNext;
            distance += bestDistance;
        }

        return distance;
    }

    double SymmetricDistance(const std::vector<TCoord>& lhs, const std::vector<TCoord>& rhs) const {
        return std::max(AsymmetricDistance(lhs, rhs), AsymmetricDistance(rhs, lhs));
    }

    double Score(const std::vector<TCoord>& modifiedNeededPoints,
                 const std::vector<TCoord>& modifiedObservedPoints) const
    {
        //const std::vector<TCoord> interestingNeededPoints = GetInterestingPoints(modifiedNeededPoints);
        //const std::vector<TCoord> interestingObservedPoints = GetInterestingPoints(modifiedObservedPoints);

        double distance = 0.;
        for (size_t i = 0; i < modifiedNeededPoints.size(); ++i) {
            const double xDiff = modifiedNeededPoints[i].X - modifiedObservedPoints[i].X;
            const double yDiff = modifiedNeededPoints[i].Y - modifiedObservedPoints[i].Y;

            distance += xDiff * xDiff + yDiff * yDiff;
        }

        return -distance;
    }

    void LoadFromString(const std::wstring& source) {
        std::vector<std::wstring> parts;
        std::wstring current;

        auto add = [&](){
            TKeyInfo keyInfo;
            keyInfo.LeftUpper.X = stod(parts[0]);
            keyInfo.LeftUpper.Y = stod(parts[1]);
            keyInfo.Width = stod(parts[1]);
            keyInfo.Height = stod(parts[1]);

            KeyInfos[current.front()] = keyInfo;

            parts.clear();
            current.clear();
        };

        for (size_t i = 0; i < source.size(); ++i) {
            if (source[i] == '\t') {
                add();
                break;
            }
            if (source[i] == ' ') {
                add();
                continue;
            }
            if (source[i] == ':') {
                parts.push_back(current);
                current.clear();
                continue;
            }
            current += source[i];
        }
    }

    void BuildVPTree(TDict& dict) {
        dict.ClustersVPTree = std::unique_ptr<TDict::TDictVPTree>(new TDict::TDictVPTree(ClusterEmbeddings.begin(), ClusterEmbeddings.end(), TEmbeddingMetric()));
    }

    void MakeClusters(TDict& dict, const size_t clustersCount, const size_t iterationsCount) {
        std::vector<std::vector<TCoord>> wordEmbeddings;      // :)
        std::vector<std::vector<TCoord>> shortWordEmbeddings; // :)

        {
            for (const std::wstring& word: dict.Words) {
                wordEmbeddings.push_back(MakePoints(word));
                shortWordEmbeddings.push_back(TDict::ShortenEmbedding(wordEmbeddings.back()));
            }
        }

        dict.ClusterCenters = shortWordEmbeddings;

        std::mt19937_64 mersenne;
        std::shuffle(dict.ClusterCenters.begin(), dict.ClusterCenters.end(), mersenne);
        if (dict.ClusterCenters.size() > clustersCount) {
            dict.ClusterCenters.resize(clustersCount);
        }

        for (size_t iteration = 0; iteration < iterationsCount; ++iteration) {
            dict.UpdateClusterWords(clustersCount, shortWordEmbeddings);
            dict.UpdateClusterCenters(clustersCount, shortWordEmbeddings);
        }

        for (const std::vector<TCoord>& clusterCenter : dict.ClusterCenters) {
            TShortEmbedding clusterEmbedding;
            clusterEmbedding.Coords = clusterCenter;
            clusterEmbedding.Idx = ClusterEmbeddings.size();

            ClusterEmbeddings.push_back(clusterEmbedding);
        }
    }
};
