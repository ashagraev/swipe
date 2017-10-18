#pragma once

#include "vp_tree.h"
#include "welford.h"

#include <iostream>
#include <memory>

#include <algorithm>

#include <string>
#include <vector>

#include <cmath>

enum {
    ShortEmbeddingLength = 20
};

struct TCoord {
    double X;
    double Y;

    TCoord(const double x = 0., const double y = 0.)
        : X(x)
        , Y(y)
    {
    }
};

static inline double Distance(
    const std::vector<TCoord>& lhs,
    const std::vector<TCoord>& rhs)
{
    if (lhs.empty()) {
        return 0.;
    }

    double sumSquaredDistances = 0.;
    for (size_t i = 0; i < lhs.size(); ++i) {
        const double xDiff = lhs[i].X - rhs[i].X;
        const double yDiff = lhs[i].Y - rhs[i].Y;

        const double squaredDistance = xDiff * xDiff + yDiff * yDiff;

        sumSquaredDistances += squaredDistance;
    }
    return sqrt(std::max(0., sumSquaredDistances) / lhs.size());
}

struct TShortEmbedding {
    std::vector<TCoord> Coords;
    unsigned int Idx = 0;
};

struct TEmbeddingMetric {
    static double Distance (const TShortEmbedding& lhs, const TShortEmbedding& rhs) {
        return ::Distance(lhs.Coords, rhs.Coords);
    }
};

struct TDict {
    using TWordIndex = unsigned int;
    std::vector<std::wstring> Words;

    using TDictVPTree = TVantagePointTree<TShortEmbedding, TEmbeddingMetric>;
    std::unique_ptr<TDictVPTree> ClustersVPTree;

    std::vector<std::vector<TCoord>> ClusterCenters;
    std::vector<std::vector<TWordIndex>> ClusterWords;

    void UpdateClusterWords(const size_t clustersCount, std::vector<std::vector<TCoord>>& shortWordEmbeddings);
    void UpdateClusterCenters(const size_t clustersCount, std::vector<std::vector<TCoord>>& shortWordEmbeddings);

    static std::vector<TCoord> ShortenEmbedding(const std::vector<TCoord>& embedding);

    std::pair<size_t, double> GetCluster(const std::vector<TCoord>& embedding) const;
    std::pair<size_t, double> GetClusterForShort(const std::vector<TCoord>& shortEmbedding) const;
};
