#include "dict.h"

void TDict::UpdateClusterWords(const size_t clustersCount, std::vector<std::vector<TCoord>>& shortWordEmbeddings) {
    double sumBestDistances = 0.;
    ClusterWords.assign(clustersCount, {});
    for (size_t wordIdx = 0; wordIdx < shortWordEmbeddings.size(); ++wordIdx) {
        std::pair<size_t, double> bestClusterInfo = GetClusterForShort(shortWordEmbeddings[wordIdx]);

        ClusterWords[bestClusterInfo.first].push_back(wordIdx);
        sumBestDistances += bestClusterInfo.second;
    }
    std::cerr << "score: " << (sumBestDistances / shortWordEmbeddings.size()) << std::endl;
}

void TDict::UpdateClusterCenters(const size_t clustersCount, std::vector<std::vector<TCoord>>& shortWordEmbeddings) {
    for (size_t clusterId = 0; clusterId < clustersCount; ++clusterId) {
        std::vector<TMeanCalculator> xCoords(ShortEmbeddingLength);
        std::vector<TMeanCalculator> yCoords(ShortEmbeddingLength);

        for (const size_t wordIdx : ClusterWords[clusterId]) {
            for (size_t i = 0; i < ShortEmbeddingLength; ++i) {
                const TCoord& shortEmbedding = shortWordEmbeddings[wordIdx][i];
                xCoords[i].Add(shortEmbedding.X);
                yCoords[i].Add(shortEmbedding.Y);
            }
        }

        for (size_t i = 0; i < ShortEmbeddingLength; ++i) {
            ClusterCenters[clusterId][i].X = xCoords[i].GetMean();
            ClusterCenters[clusterId][i].Y = yCoords[i].GetMean();
        }
    }
}

std::vector<TCoord> TDict::ShortenEmbedding(const std::vector<TCoord >& embedding) {
    std::vector<TCoord> shortEmbedding(ShortEmbeddingLength);
    for (size_t i = 0; i < ShortEmbeddingLength; ++i) {
        const size_t start = i * embedding.size() / ShortEmbeddingLength;
        const size_t end = (i + 1) * embedding.size() / ShortEmbeddingLength;

        TMeanCalculator x, y;
        for (size_t j = start; j < end; ++j) {
            x.Add(embedding[j].X);
            y.Add(embedding[j].Y);
        }

        shortEmbedding[i].X = x.GetMean();
        shortEmbedding[i].Y = y.GetMean();
    }

    return shortEmbedding;
}

std::pair<size_t, double> TDict::GetCluster(const std::vector<TCoord>& embedding) const {
    return GetClusterForShort(ShortenEmbedding(embedding));
}

std::pair<size_t, double> TDict::GetClusterForShort(const std::vector<TCoord>& shortEmbedding) const {
    size_t bestCluster = 0;
    double bestDistance = Distance(shortEmbedding, ClusterCenters[0]);
    for (size_t clusterId = 1; clusterId < ClusterCenters.size(); ++clusterId) {
        const double distance = Distance(shortEmbedding, ClusterCenters[clusterId]);
        if (distance > bestDistance) {
            continue;
        }
        bestDistance = distance;
        bestCluster = clusterId;
    }
    return std::make_pair(bestCluster, bestDistance);
}
