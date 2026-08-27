#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <optional>

#include "RoadNetwork.hpp"

namespace roadNetwork {
    namespace colors {
        const sf::Color fwdLane(80, 220, 255);          ///< Color for forward lane segments
        const sf::Color bwdLane(100, 255, 180);         ///< Color for backward lane segments
        const sf::Color oneWayLane(80, 220, 255);       ///< Color for one-way lane segments
        const sf::Color turnLane(255, 210, 60, 240);    ///< Color for intersection turn lanes
    }

    namespace defaults {
        constexpr float laneWidth = 14.0f;
        constexpr float speedLimit = 5.0f;
        constexpr float turnLaneSpeedLimit = 3.5f;
        constexpr float turnLaneWidth = 12.0f;

        constexpr float intersectionRadius = 26.0f;
        constexpr float gatewayRadius = 20.0f;
        constexpr float defaultNodeRadius = 24.0f;

        constexpr float nodeClickExtraRadius = 8.0f;
        constexpr float roadClickTolerance = 18.0f;
        constexpr float handleClickRadius = 14.0f;

        constexpr int roadSampleCount = 30;
        constexpr int turnLaneSampleCount = 24;
        constexpr float roadTrimRatio = 0.45f;
        constexpr float handleScaleRatio = 2.5f;
        constexpr float maxTurnHandleRatio = 1.2f;
    }
}

/**
 * @brief Core manager and graph data structure for the traffic simulation road network.
 *
 * Manages polymorphically allocated Intersection and Gateway nodes, two-way and one-way roads,
 * directed lane segments, and automatically computes internal turning lanes within intersections.
 */
class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager() = default;

    // ------------------------------------------------------------------------
    // Node Management
    // ------------------------------------------------------------------------
    size_t createNode(const sf::Vector2f& position,
                      RoadNetwork::NodeType type = RoadNetwork::NodeType::Intersection,
                      float radius = roadNetwork::defaults::defaultNodeRadius);
    size_t createIntersection(const sf::Vector2f& position,
                              float radius = roadNetwork::defaults::intersectionRadius);
    size_t createGateway(const sf::Vector2f& position,
                         float radius = roadNetwork::defaults::gatewayRadius);

    bool removeNode(size_t nodeId);
    bool moveNode(size_t nodeId, const sf::Vector2f& newPosition);

    // Node Type Conversion
    bool convertGatewayToIntersection(size_t nodeId);
    bool convertIntersectionToGateway(size_t nodeId);
    bool toggleNodeType(size_t nodeId);

    RoadNetwork::Node* getNode(size_t nodeId);
    const RoadNetwork::Node* getNode(size_t nodeId) const;

    RoadNetwork::Intersection* getIntersection(size_t nodeId);
    const RoadNetwork::Intersection* getIntersection(size_t nodeId) const;

    RoadNetwork::Gateway* getGateway(size_t nodeId);
    const RoadNetwork::Gateway* getGateway(size_t nodeId) const;

    const std::unordered_map<size_t, std::shared_ptr<RoadNetwork::Node>>& getNodes() const { return nodes; }

    [[nodiscard]] int findNodeAt(const sf::Vector2f& pos,
                                 float extraRadius = roadNetwork::defaults::nodeClickExtraRadius) const;

    // ------------------------------------------------------------------------
    // Road & Segment Management (Two-Way and One-Way Roads)
    // ------------------------------------------------------------------------
    size_t createStraightTwoWayRoad(size_t nodeA, size_t nodeB,
                                    float speedLimit = roadNetwork::defaults::speedLimit,
                                    float laneWidth = roadNetwork::defaults::laneWidth);
    size_t createCurvedTwoWayRoad(size_t nodeA, size_t nodeB,
                                  const sf::Vector2f& cp1, const sf::Vector2f& cp2,
                                  float speedLimit = roadNetwork::defaults::speedLimit,
                                  float laneWidth = roadNetwork::defaults::laneWidth);

    size_t createStraightOneWayRoad(size_t fromNode, size_t toNode,
                                    float speedLimit = roadNetwork::defaults::speedLimit,
                                    float laneWidth = roadNetwork::defaults::laneWidth);
    size_t createCurvedOneWayRoad(size_t fromNode, size_t toNode,
                                  const sf::Vector2f& cp1, const sf::Vector2f& cp2,
                                  float speedLimit = roadNetwork::defaults::speedLimit,
                                  float laneWidth = roadNetwork::defaults::laneWidth);

    bool removeRoad(size_t roadId);

    bool moveRoadControlPoint(size_t roadId, int handleIndex, const sf::Vector2f& newPos);

    RoadNetwork::Road* getRoad(size_t roadId);
    const RoadNetwork::Road* getRoad(size_t roadId) const;
    const std::unordered_map<size_t, RoadNetwork::Road>& getRoads() const { return roads; }

    RoadNetwork::RoadSegment* getSegment(size_t segmentId);
    const RoadNetwork::RoadSegment* getSegment(size_t segmentId) const;
    const std::unordered_map<size_t, RoadNetwork::RoadSegment>& getSegments() const { return segments; }
    const std::vector<RoadNetwork::RoadSegment>& getTurnLanes() const { return turnLanes; }

    [[nodiscard]] int findRoadAt(const sf::Vector2f& pos,
                                 float maxDistance = roadNetwork::defaults::roadClickTolerance) const;
    [[nodiscard]] int findRoadControlPointAt(size_t roadId, const sf::Vector2f& pos,
                                             float radius = roadNetwork::defaults::handleClickRadius) const;

    // ------------------------------------------------------------------------
    // Intersection Turn Lane Generation
    // ------------------------------------------------------------------------
    void rebuildIntersectionTurns(size_t nodeId);
    void rebuildAllIntersections();

    // ------------------------------------------------------------------------
    // Network Operations & Demos
    // ------------------------------------------------------------------------
    void clear();
    void loadSampleNetwork(unsigned int windowWidth = 1280, unsigned int windowHeight = 720);

    // Queries
    std::vector<size_t> getIncomingSegments(size_t nodeId) const;
    std::vector<size_t> getOutgoingSegments(size_t nodeId) const;
    std::vector<size_t> getConnectedRoads(size_t nodeId) const;

private:
    void updateRoadGeometry(RoadNetwork::Road& road);
    size_t getNextNodeId() { return nextNodeId++; }
    size_t getNextSegmentId() { return nextSegmentId++; }
    size_t getNextRoadId() { return nextRoadId++; }

    std::unordered_map<size_t, std::shared_ptr<RoadNetwork::Node>> nodes;
    std::unordered_map<size_t, RoadNetwork::RoadSegment> segments;
    std::unordered_map<size_t, RoadNetwork::Road> roads;
    std::vector<RoadNetwork::RoadSegment> turnLanes;

    size_t nextNodeId = 1;
    size_t nextSegmentId = 1;
    size_t nextRoadId = 1;
};
