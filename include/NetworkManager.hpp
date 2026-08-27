#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <optional>

#include "RoadNetwork.hpp"

/**
 * @brief Core manager and graph data structure for the traffic simulation road network.
 *
 * Manages polymorphically allocated Intersection and Gateway nodes, two-way roads,
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
                      float radius = 24.0f);
    size_t createIntersection(const sf::Vector2f& position, float radius = 26.0f);
    size_t createGateway(const sf::Vector2f& position, float radius = 20.0f);

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

    [[nodiscard]] int findNodeAt(const sf::Vector2f& pos, float extraRadius = 0.0f) const;

    // ------------------------------------------------------------------------
    // Road & Segment Management
    // ------------------------------------------------------------------------
    size_t createStraightTwoWayRoad(size_t nodeA, size_t nodeB,
                                    float speedLimit = 5.0f, float laneWidth = 14.0f);
    size_t createCurvedTwoWayRoad(size_t nodeA, size_t nodeB,
                                  const sf::Vector2f& cp1, const sf::Vector2f& cp2,
                                  float speedLimit = 5.0f, float laneWidth = 14.0f);
    bool removeRoad(size_t roadId);

    bool moveRoadControlPoint(size_t roadId, int handleIndex, const sf::Vector2f& newPos);

    RoadNetwork::TwoWayRoad* getRoad(size_t roadId);
    const RoadNetwork::TwoWayRoad* getRoad(size_t roadId) const;
    const std::unordered_map<size_t, RoadNetwork::TwoWayRoad>& getRoads() const { return roads; }

    RoadNetwork::RoadSegment* getSegment(size_t segmentId);
    const RoadNetwork::RoadSegment* getSegment(size_t segmentId) const;
    const std::unordered_map<size_t, RoadNetwork::RoadSegment>& getSegments() const { return segments; }
    const std::vector<RoadNetwork::RoadSegment>& getTurnLanes() const { return turnLanes; }

    [[nodiscard]] int findRoadAt(const sf::Vector2f& pos, float maxDistance = 18.0f) const;
    [[nodiscard]] int findRoadControlPointAt(size_t roadId, const sf::Vector2f& pos, float radius = 12.0f) const;

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
    void updateRoadGeometry(RoadNetwork::TwoWayRoad& road);
    size_t getNextNodeId() { return nextNodeId++; }
    size_t getNextSegmentId() { return nextSegmentId++; }
    size_t getNextRoadId() { return nextRoadId++; }

    std::unordered_map<size_t, std::shared_ptr<RoadNetwork::Node>> nodes;
    std::unordered_map<size_t, RoadNetwork::RoadSegment> segments;
    std::unordered_map<size_t, RoadNetwork::TwoWayRoad> roads;
    std::vector<RoadNetwork::RoadSegment> turnLanes;

    size_t nextNodeId = 1;
    size_t nextSegmentId = 1;
    size_t nextRoadId = 1;
};
