#include <algorithm>

#include <list>
#include <vector>

#include <random>

template <class T, class TMetric>
class TRandomVantagePointChooser {
private:
    struct TItemWithDistance {
        T* Item;
        double Dist;

        TItemWithDistance(T* item, double dist)
            : Item(item)
            , Dist(dist)
        {
        }
    };

    std::mt19937_64 RandomGenerator;
    std::vector<TItemWithDistance> Distances;
public:
    void SelectVantagePoint(
        const TMetric& metric,
        T** items,
        const size_t count,
        size_t& innerNodeStart,
        size_t& outerNodeStart,
        double& radius)
    {
        SelectVantagePoint(metric, items, count, innerNodeStart, outerNodeStart, radius, RandomGenerator() % count, 0.5f);
    }

    void SelectVantagePoint(
        const TMetric& metric,
        T** items,
        const size_t count,
        size_t& innerNodeStart,
        size_t& outerNodeStart,
        double& radius,
        size_t vpItemIndex,
        float nodeSplitFraction)
    {
        std::swap(items[0], items[vpItemIndex]);

        Distances.clear();
        Distances.reserve(count);
        Distances.push_back(TItemWithDistance(items[0], 0));

        for (size_t i = 1; i < count; ++i) {
            const double d = metric.Distance(**items, *(items[i]));
            Distances.push_back(TItemWithDistance(items[i], d));
        }

        TItemWithDistance* beg = Distances.begin() + 1;
        TItemWithDistance* end = Distances.end() - 1;

        // Move items with distance = 0 forward
        while (beg <= end) {
            if (end->Dist <= 0) {
                std::swap(*beg, *end);
                ++beg;
            } else {
                --end;
            }
        }

        innerNodeStart = beg - Distances.begin();
        const size_t remainingCount = count - innerNodeStart;
        outerNodeStart = innerNodeStart + nodeSplitFraction * remainingCount;

        if (remainingCount > 0) {
            auto&& cmpByDistance = [](const TItemWithDistance& item1, const TItemWithDistance& item2) {
                return item1.Dist < item2.Dist;
            };
            std::nth_element(Distances.begin() + innerNodeStart, Distances.begin() + outerNodeStart, Distances.end(), cmpByDistance);
            radius = Distances[outerNodeStart].Dist;
            ++outerNodeStart;
            size_t endIndex = count - 1;

            while (outerNodeStart <= endIndex) {
                if (Distances[endIndex].Dist <= radius) {
                    std::swap(Distances[outerNodeStart], Distances[endIndex]);
                    ++outerNodeStart;
                } else {
                    --endIndex;
                }
            }
        }

        for (size_t i = 1; i < count; ++i) {
            items[i] = Distances[i].Item;
        }
    }
};

template <class T, class TMetric>
class TVantagePointTree {
public:
    struct TNode {
        double Radius = 0;
        size_t Size = 0;
        T** Items = nullptr;
        TNode* Inner = nullptr;
        TNode* Outer = nullptr;
    };

private:
    struct TItemWithDist {
        T* Item;
        double Dist;

        TItemWithDist(const T* item, double dist)
            : Item(item)
            , Dist(dist)
        {
        }
    };

    struct TNodeBuildParams {
        TNode** ParentRef;
        T** Items;
        size_t Count;

        TNodeBuildParams(TNode** parentRef, T** items, size_t count)
            : ParentRef(parentRef)
            , Items(items)
            , Count(count)
        {
        }
    };

    enum {
        MaxLeafSize = 5
    };

    TRandomVantagePointChooser<T, TMetric> VantagePointChooser;

    const TMetric Metric;
    std::vector<T*> Items;
    std::list<std::shared_ptr<TNode>> Nodes;
public:
    template <typename TInputIterator>
    TVantagePointTree(TInputIterator begin, TInputIterator end, const TMetric& metric = TMetric())
        : Metric(metric)
        , Items(std::distance(begin, end))
    {
        size_t i = 0;
        for (TInputIterator it = begin; it < end; ++it, ++i) {
            Items[i] = &(*it);
        }

        std::vector<TNodeBuildParams> nodesToBuild;
        TNode* root = nullptr;
        nodesToBuild.push_back(TNodeBuildParams(&root, Items.data(), Items.size()));

        while (!nodesToBuild.empty()) {
            TNodeBuildParams nodeParams = nodesToBuild.back();
            nodesToBuild.pop_back();
            *nodeParams.ParentRef = BuildNode(nodeParams.Items, nodeParams.Count, nodesToBuild);
        }
    }

    std::vector<T*> FindNearbyItems(const T& item, const double maxDistance, const size_t limit) {
        std::vector<T*> result;
        if (!Nodes.empty()) {
            FindNearbyItems(*Nodes.front(), item, maxDistance, result, limit);
        }
        return result;
    }
private:
    TNode* BuildNode(T** items, const size_t count, std::vector<TNodeBuildParams>& nodesToBuild) {
        Nodes.push_back(std::shared_ptr<TNode>(new TNode()));
        TNode& node = *Nodes.back();

        node.Items = items;

        if (count <= MaxLeafSize) {
            node.Size = count;
            return &node;
        }

        size_t outerNodeStart = 0;
        VantagePointChooser.SelectVantagePoint(Metric, items, count, node.Size, outerNodeStart, node.Radius);

        nodesToBuild.push_back(TNodeBuildParams(&node.Inner, items + node.Size, outerNodeStart - node.Size));
        nodesToBuild.push_back(TNodeBuildParams(&node.Outer, items + outerNodeStart, count - outerNodeStart));

        return &node;
    }

    bool FindNearbyItems(TNode& node, const T& item, double maxDistance, std::vector<T*>& results, const size_t limit) {
        if (results.size() == limit) {
            return true;
        }

        if (node.Inner == nullptr) {
            for (size_t i = 0; i < node.Size; ++i) {
                if (Metric.Distance(item, *node.Items[i]) <= maxDistance) {
                    results.push_back(node.Items[i]);
                    if (results.size() == limit) {
                        return true;
                    }
                }
            }
            return true;
        }

        const double distance = Metric.Distance(item, **node.Items);
        if (distance <= maxDistance) {
            for (size_t i = 0; i < node.Size; ++i) {
                results.push_back(node.Items[i]);
                if (results.size() == limit) {
                    return true;
                }
            }
        }

        if (node.Radius >= distance + maxDistance) {
            return FindNearbyItems(*node.Inner, item, maxDistance, results, limit);
        }

        if (distance > node.Radius + maxDistance) {
            return FindNearbyItems(*node.Outer, item, maxDistance, results, limit);
        }

        if (!FindNearbyItems(*node.Inner, item, maxDistance, results, limit)) {
            return false;
        }

        return FindNearbyItems(*node.Outer, item, maxDistance, results, limit);
    }
};
