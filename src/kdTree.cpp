#include "kdTree.hpp"

#include <algorithm>
#include <limits>
#include <numeric>

kdTree::kdTree(std::vector<Point> const& points) : points(points), root(nullptr) {}

kdTree::~kdTree() {
    clear(root);
}

kdTree::kdTree(kdTree&& other) noexcept : points(other.points), root(other.root) {
    other.root = nullptr;
}

kdTree& kdTree::operator=(kdTree&& other) noexcept {
    if (this != &other) {
        clear(root);
        root = other.root;
        other.root = nullptr;
    }
    return *this;
}

void kdTree::build(size_t maxLeafSize) {
    clear(root);
    root = nullptr;

    if (points.empty()) return;

    std::vector<size_t> indices(points.size());
    std::iota(indices.begin(), indices.end(), 0);

    // Calculate bounding box for the entire point set
    Point globalMin = {std::numeric_limits<float>::max(),
                       std::numeric_limits<float>::max(),
                       std::numeric_limits<float>::max()};
    Point globalMax = {std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest()};

    for (const Point& p : points) {
        for (int i = 0; i < 3; ++i) {
            globalMin[i] = std::min(globalMin[i], p[i]);
            globalMax[i] = std::max(globalMax[i], p[i]);
        }
    }

    root = buildRecursive(indices, 0, indices.size(), globalMin, globalMax, maxLeafSize);
}

KDNode* kdTree::buildRecursive(std::vector<size_t>& indices, size_t start, size_t end,
                               Point currentMin, Point currentMax, size_t maxLeafSize) {
    if (start >= end) return nullptr;

    size_t count = end - start;

    if (count <= maxLeafSize) {
        std::vector<size_t> leafPoints(indices.begin() + start, indices.begin() + end);
        return new KDNode(std::move(leafPoints));
    }

    // Determine the longest axis for splitting
    Point extent = {
        currentMax[0] - currentMin[0],
        currentMax[1] - currentMin[1],
        currentMax[2] - currentMin[2]
    };
    int dim = 0;
    if (extent[1] > extent[dim]) dim = 1;
    if (extent[2] > extent[dim]) dim = 2;

    size_t mid = start + count / 2;

    // Fast O(N) median partitioning
    std::nth_element(indices.begin() + start, indices.begin() + mid, indices.begin() + end,
                     [&](size_t a, size_t b) { return points[a][dim] < points[b][dim]; });

    float splitVal = points[indices[mid]][dim];

    KDNode* node = new KDNode();
    node->splitAxis = dim;
    node->splitValue = splitVal;

    Point leftMax = currentMax;
    leftMax[dim] = splitVal;
    Point rightMin = currentMin;
    rightMin[dim] = splitVal;

    node->left = buildRecursive(indices, start, mid, currentMin, leftMax, maxLeafSize);
    node->right = buildRecursive(indices, mid, end, rightMin, currentMax, maxLeafSize);

    return node;
}

void kdTree::clear(KDNode* node) {
    if (node == nullptr) return;
    clear(node->left);
    clear(node->right);
    delete node;
}

// 3D Euclidean radius search
std::vector<size_t> kdTree::collectInRadius(Point const& p, float radius) const {
    std::vector<size_t> result;
    if (!root) return result;
    collectInRadiusRecursive(root, p, radius * radius, result);
    return result;
}

void kdTree::collectInRadiusRecursive(KDNode* node, Point const& p, float radiusSquared,
                                      std::vector<size_t>& result) const {
    if (!node) return;

    if (node->isLeaf) {
        for (size_t idx : node->pointIndices) {
            if (distanceSquared(points[idx], p) <= radiusSquared) {
                result.push_back(idx);
            }
        }
        return;
    }

    int dim = node->splitAxis;
    float distanceToPlane = p[dim] - node->splitValue;
    KDNode* nearChild = (distanceToPlane < 0.0f) ? node->left : node->right;
    KDNode* farChild  = (distanceToPlane < 0.0f) ? node->right : node->left;

    collectInRadiusRecursive(nearChild, p, radiusSquared, result);

    if ((distanceToPlane * distanceToPlane) <= radiusSquared) {
        collectInRadiusRecursive(farChild, p, radiusSquared, result);
    }
}

// 2.5D Radius search: enforces Z-layer matching and checks 2D radius on (X, Y)
std::vector<size_t> kdTree::collectInRadius2D(Point const& p, float radius, float layerTolerance) const {
    std::vector<size_t> result;
    if (!root) return result;
    collectInRadiusRecursive2D(root, p, radius * radius, layerTolerance, result);
    return result;
}

void kdTree::collectInRadiusRecursive2D(KDNode* node, Point const& p, float radiusSquared,
                                        float layerTolerance, std::vector<size_t>& result) const {
    if (!node) return;

    if (node->isLeaf) {
        for (size_t idx : node->pointIndices) {
            // Strictly check Z-layer match so vehicles on bridges/different elevations don't interfere
            if (std::abs(points[idx][2] - p[2]) <= layerTolerance &&
                distanceSquared2D(points[idx], p) <= radiusSquared) {
                result.push_back(idx);
            }
        }
        return;
    }

    int dim = node->splitAxis;
    float distanceToPlane = p[dim] - node->splitValue;
    KDNode* nearChild = (distanceToPlane < 0.0f) ? node->left : node->right;
    KDNode* farChild  = (distanceToPlane < 0.0f) ? node->right : node->left;

    collectInRadiusRecursive2D(nearChild, p, radiusSquared, layerTolerance, result);

    if (dim == 2) {
        // Z-axis split: check far child only if search layer tolerance intersects splitting plane
        if (std::abs(distanceToPlane) <= layerTolerance) {
            collectInRadiusRecursive2D(farChild, p, radiusSquared, layerTolerance, result);
        }
    } else {
        // X or Y axis split: check far child if search radius intersects splitting plane
        if ((distanceToPlane * distanceToPlane) <= radiusSquared) {
            collectInRadiusRecursive2D(farChild, p, radiusSquared, layerTolerance, result);
        }
    }
}

// 3D K-Nearest Neighbors search
std::vector<size_t> kdTree::collectKNearest(Point const& p, unsigned int k) const {
    if (k == 0 || !root || points.empty()) return {};

    std::priority_queue<std::pair<float, size_t>> knnQueue;
    collectKNearestRecursive(root, p, k, knnQueue);

    std::vector<size_t> result;
    result.reserve(knnQueue.size());
    while (!knnQueue.empty()) {
        result.push_back(knnQueue.top().second);
        knnQueue.pop();
    }
    std::reverse(result.begin(), result.end());
    return result;
}

void kdTree::collectKNearestRecursive(KDNode* node, Point const& p, unsigned int k,
                                      std::priority_queue<std::pair<float, size_t>>& knnQueue) const {
    if (!node || k == 0) return;

    if (node->isLeaf) {
        for (size_t idx : node->pointIndices) {
            float dsq = distanceSquared(points[idx], p);
            if (knnQueue.size() < k) {
                knnQueue.push({dsq, idx});
            } else if (dsq < knnQueue.top().first) {
                knnQueue.pop();
                knnQueue.push({dsq, idx});
            }
        }
        return;
    }

    int dim = node->splitAxis;
    float distanceToPlane = p[dim] - node->splitValue;
    KDNode* nearChild = (distanceToPlane < 0.0f) ? node->left : node->right;
    KDNode* farChild  = (distanceToPlane < 0.0f) ? node->right : node->left;

    collectKNearestRecursive(nearChild, p, k, knnQueue);

    if ((knnQueue.size() < k) || ((distanceToPlane * distanceToPlane) <= knnQueue.top().first)) {
        collectKNearestRecursive(farChild, p, k, knnQueue);
    }
}

// 2.5D K-Nearest Neighbors search with Z-layer matching
std::vector<size_t> kdTree::collectKNearest2D(Point const& p, unsigned int k, float layerTolerance) const {
    if (k == 0 || !root || points.empty()) return {};

    std::priority_queue<std::pair<float, size_t>> knnQueue;
    collectKNearestRecursive2D(root, p, k, layerTolerance, knnQueue);

    std::vector<size_t> result;
    result.reserve(knnQueue.size());
    while (!knnQueue.empty()) {
        result.push_back(knnQueue.top().second);
        knnQueue.pop();
    }
    std::reverse(result.begin(), result.end());
    return result;
}

void kdTree::collectKNearestRecursive2D(KDNode* node, Point const& p, unsigned int k,
                                        float layerTolerance,
                                        std::priority_queue<std::pair<float, size_t>>& knnQueue) const {
    if (!node || k == 0) return;

    if (node->isLeaf) {
        for (size_t idx : node->pointIndices) {
            if (std::abs(points[idx][2] - p[2]) <= layerTolerance) {
                float dsq = distanceSquared2D(points[idx], p);
                if (knnQueue.size() < k) {
                    knnQueue.push({dsq, idx});
                } else if (dsq < knnQueue.top().first) {
                    knnQueue.pop();
                    knnQueue.push({dsq, idx});
                }
            }
        }
        return;
    }

    int dim = node->splitAxis;
    float distanceToPlane = p[dim] - node->splitValue;
    KDNode* nearChild = (distanceToPlane < 0.0f) ? node->left : node->right;
    KDNode* farChild  = (distanceToPlane < 0.0f) ? node->right : node->left;

    collectKNearestRecursive2D(nearChild, p, k, layerTolerance, knnQueue);

    if (dim == 2) {
        if (std::abs(distanceToPlane) <= layerTolerance) {
            collectKNearestRecursive2D(farChild, p, k, layerTolerance, knnQueue);
        }
    } else {
        if ((knnQueue.size() < k) || ((distanceToPlane * distanceToPlane) <= knnQueue.top().first)) {
            collectKNearestRecursive2D(farChild, p, k, layerTolerance, knnQueue);
        }
    }
}