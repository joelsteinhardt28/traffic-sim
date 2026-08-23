#pragma once

#include <cstddef>
#include <queue>
#include <utility>
#include <vector>

#include "structs.hpp"

// Node structure for the KD-tree
struct KDNode {
    bool isLeaf;
    int splitAxis;
    float splitValue;
    std::vector<size_t> pointIndices;

    KDNode* left;
    KDNode* right;

    KDNode() : isLeaf(false), splitAxis(0), splitValue(0.0f), left(nullptr), right(nullptr) {}
    explicit KDNode(std::vector<size_t> indices)
        : isLeaf(true), splitAxis(0), splitValue(0.0f), pointIndices(std::move(indices)), left(nullptr), right(nullptr) {}
};

// KD-Tree for 2.5D / 3D spatial queries in traffic simulation
class kdTree {
public:
    explicit kdTree(std::vector<Point> const& points);
    ~kdTree();

    // Prevent copying to avoid double-free on root pointer
    kdTree(const kdTree&) = delete;
    kdTree& operator=(const kdTree&) = delete;

    // Move semantics
    kdTree(kdTree&& other) noexcept;
    kdTree& operator=(kdTree&& other) noexcept;

    void build(size_t maxLeafSize = DEFAULT_MAX_LEAF_SIZE);

    [[nodiscard]] KDNode* getRoot() const { return root; }

    // 3D Euclidean radius search
    [[nodiscard]] std::vector<size_t> collectInRadius(Point const& p, float radius) const;

    // 2.5D radius search: strictly matches Z-layer within layerTolerance (default 0.5f) and 2D radius on (X, Y)
    [[nodiscard]] std::vector<size_t> collectInRadius2D(Point const& p, float radius, float layerTolerance = 0.5f) const;

    // 3D K-Nearest Neighbors search
    [[nodiscard]] std::vector<size_t> collectKNearest(Point const& p, unsigned int k) const;

    // 2.5D K-Nearest Neighbors search with Z-layer matching
    [[nodiscard]] std::vector<size_t> collectKNearest2D(Point const& p, unsigned int k, float layerTolerance = 0.5f) const;

private:
    const std::vector<Point>& points;
    KDNode* root;

    KDNode* buildRecursive(std::vector<size_t>& indices, size_t start, size_t end,
                           Point currentMin, Point currentMax, size_t maxLeafSize);
    void clear(KDNode* node);

    void collectInRadiusRecursive(KDNode* node, Point const& p, float radiusSquared,
                                  std::vector<size_t>& result) const;
    void collectInRadiusRecursive2D(KDNode* node, Point const& p, float radiusSquared,
                                    float layerTolerance, std::vector<size_t>& result) const;
    void collectKNearestRecursive(KDNode* node, Point const& p, unsigned int k,
                                  std::priority_queue<std::pair<float, size_t>>& knnQueue) const;
    void collectKNearestRecursive2D(KDNode* node, Point const& p, unsigned int k,
                                    float layerTolerance,
                                    std::priority_queue<std::pair<float, size_t>>& knnQueue) const;
};