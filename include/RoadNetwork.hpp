#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

#include "curves.hpp"

namespace RoadNetwork {

/**
 * @brief Identifies whether a node acts as an active multi-way intersection or boundary gateway.
 */
enum class NodeType {
    Intersection,  ///< Connects multiple road segments and generates internal turning lanes
    Gateway        ///< Entry/exit dead-end point of the network
};

/**
 * @brief Distinguishes between regular driving lanes, intersection turn connectors, and gateway tracks.
 */
enum class SegmentType {
    NormalLane,       ///< Regular driving lane along a road
    IntersectionTurn, ///< Turning lane within an intersection
    GatewayLane       ///< Lane connecting to a gateway
};

/**
 * @brief Abstract base class representing a node (junction or endpoint) in the road graph.
 */
class Node {
public:
    virtual ~Node() = default;

    [[nodiscard]] size_t getId() const { return id; }
    void setId(size_t newId) { id = newId; }

    [[nodiscard]] const sf::Vector2f& getPosition() const { return position; }
    void setPosition(const sf::Vector2f& pos) { position = pos; }

    [[nodiscard]] float getRadius() const { return radius; }
    void setRadius(float r) { radius = r; }

    [[nodiscard]] virtual NodeType getType() const = 0;
    [[nodiscard]] virtual std::string getTypeName() const = 0;
    [[nodiscard]] virtual std::string getShortLabel() const = 0;
    [[nodiscard]] virtual bool isIntersection() const { return false; }
    [[nodiscard]] virtual bool isGateway() const { return false; }

    // Edge & Road Connections
    [[nodiscard]] const std::vector<size_t>& getIncomingEdgeIds() const { return incomingEdgeIds; }
    [[nodiscard]] std::vector<size_t>& getIncomingEdgeIds() { return incomingEdgeIds; }

    [[nodiscard]] const std::vector<size_t>& getOutgoingEdgeIds() const { return outgoingEdgeIds; }
    [[nodiscard]] std::vector<size_t>& getOutgoingEdgeIds() { return outgoingEdgeIds; }

    [[nodiscard]] const std::vector<size_t>& getConnectedRoadIds() const { return connectedRoadIds; }
    [[nodiscard]] std::vector<size_t>& getConnectedRoadIds() { return connectedRoadIds; }

    void addIncomingEdge(size_t edgeId);
    void removeIncomingEdge(size_t edgeId);

    void addOutgoingEdge(size_t edgeId);
    void removeOutgoingEdge(size_t edgeId);

    void addConnectedRoad(size_t roadId);
    void removeConnectedRoad(size_t roadId);

protected:
    Node(size_t id, const sf::Vector2f& position, float radius = 24.0f);

    size_t id = 0;
    sf::Vector2f position = {0.0f, 0.0f};
    float radius = 24.0f;

    std::vector<size_t> incomingEdgeIds;
    std::vector<size_t> outgoingEdgeIds;
    std::vector<size_t> connectedRoadIds;
};

/**
 * @brief Concrete Node subclass representing a multi-way road intersection.
 * Supports automatic internal turning lane generation, traffic routing, and junction geometry.
 */
class Intersection : public Node {
public:
    Intersection(size_t id, const sf::Vector2f& position, float radius = 26.0f);
    virtual ~Intersection() = default;

    [[nodiscard]] virtual NodeType getType() const override { return NodeType::Intersection; }
    [[nodiscard]] virtual std::string getTypeName() const override { return "Intersection"; }
    [[nodiscard]] virtual std::string getShortLabel() const override { return "I" + std::to_string(id); }
    [[nodiscard]] virtual bool isIntersection() const override { return true; }
};

/**
 * @brief Concrete Node subclass representing a boundary gateway or dead-end entry/exit point.
 */
class Gateway : public Node {
public:
    Gateway(size_t id, const sf::Vector2f& position, float radius = 20.0f);
    virtual ~Gateway() = default;

    [[nodiscard]] virtual NodeType getType() const override { return NodeType::Gateway; }
    [[nodiscard]] virtual std::string getTypeName() const override { return "Gateway"; }
    [[nodiscard]] virtual std::string getShortLabel() const override { return "G" + std::to_string(id); }
    [[nodiscard]] virtual bool isGateway() const override { return true; }
};

/**
 * @brief Directed edge in the road graph, wrapping a CubicBezierSpline with traffic properties.
 */
struct RoadSegment {
    size_t id = 0;
    size_t fromNodeId = 0;
    size_t toNodeId = 0;
    size_t parentRoadId = 0;

    CubicBezierSpline spline;
    float speedLimit = 5.0f;
    bool isOneWay = true;
    float laneWidth = 14.0f;
    SegmentType type = SegmentType::NormalLane;
    sf::Color laneColor = sf::Color(100, 200, 255);

    RoadSegment() = default;
    RoadSegment(size_t id, size_t fromNode, size_t toNode, const CubicBezierSpline& spline,
                float speedLimit = 5.0f, bool isOneWay = true, float laneWidth = 14.0f,
                SegmentType type = SegmentType::NormalLane, sf::Color color = sf::Color(100, 200, 255))
        : id(id), fromNodeId(fromNode), toNodeId(toNode), parentRoadId(0),
          spline(spline), speedLimit(speedLimit), isOneWay(isOneWay), laneWidth(laneWidth),
          type(type), laneColor(color) {}
};

/**
 * @brief High-level road construct representing a classic two-way street with forward and backward lanes.
 */
struct TwoWayRoad {
    size_t id = 0;
    size_t nodeA = 0;
    size_t nodeB = 0;
    bool isCurved = false;
    sf::Vector2f controlPoint1 = {0.0f, 0.0f};
    sf::Vector2f controlPoint2 = {0.0f, 0.0f};

    size_t forwardSegmentId = 0;   ///< Lane from Node A -> Node B (Right-hand side)
    size_t backwardSegmentId = 0;  ///< Lane from Node B -> Node A (Right-hand side of return direction)
    float speedLimit = 5.0f;
    float laneWidth = 14.0f;

    TwoWayRoad() = default;
    TwoWayRoad(size_t id, size_t nodeA, size_t nodeB, size_t forwardId, size_t backwardId,
               float speedLimit = 5.0f, float laneWidth = 14.0f)
        : id(id), nodeA(nodeA), nodeB(nodeB), isCurved(false),
          forwardSegmentId(forwardId), backwardSegmentId(backwardId),
          speedLimit(speedLimit), laneWidth(laneWidth) {}

    TwoWayRoad(size_t id, size_t nodeA, size_t nodeB, const sf::Vector2f& cp1, const sf::Vector2f& cp2,
               size_t forwardId, size_t backwardId, float speedLimit = 5.0f, float laneWidth = 14.0f)
        : id(id), nodeA(nodeA), nodeB(nodeB), isCurved(true), controlPoint1(cp1), controlPoint2(cp2),
          forwardSegmentId(forwardId), backwardSegmentId(backwardId),
          speedLimit(speedLimit), laneWidth(laneWidth) {}
};

} // namespace RoadNetwork
