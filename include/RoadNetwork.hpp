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
 * @brief Categorizes roads as bidirectional two-way streets or single-direction one-way streets.
 */
enum class RoadType {
    TwoWay,  ///< Classic two-way street with forward and backward lanes
    OneWay   ///< Single-direction one-way road with one driving lane
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
 * @brief Core road structure representing either a TwoWay street or a OneWay street.
 */
struct Road {
    size_t id = 0;
    RoadType type = RoadType::TwoWay;
    size_t nodeA = 0;   ///< Start / First node (For OneWay: fromNode)
    size_t nodeB = 0;   ///< End / Second node (For OneWay: toNode)
    bool isCurved = false;
    sf::Vector2f controlPoint1 = {0.0f, 0.0f};
    sf::Vector2f controlPoint2 = {0.0f, 0.0f};

    size_t forwardSegmentId = 0;   ///< Forward lane (For OneWay: the single directed lane)
    size_t backwardSegmentId = 0;  ///< Backward lane (0 for OneWay)
    float speedLimit = 5.0f;
    float laneWidth = 14.0f;

    Road() = default;

    // Two-Way Constructor (Straight)
    Road(size_t id, size_t nodeA, size_t nodeB, size_t forwardId, size_t backwardId,
         float speedLimit = 5.0f, float laneWidth = 14.0f)
        : id(id), type(RoadType::TwoWay), nodeA(nodeA), nodeB(nodeB), isCurved(false),
          forwardSegmentId(forwardId), backwardSegmentId(backwardId),
          speedLimit(speedLimit), laneWidth(laneWidth) {}

    // Two-Way Constructor (Curved)
    Road(size_t id, size_t nodeA, size_t nodeB, const sf::Vector2f& cp1, const sf::Vector2f& cp2,
         size_t forwardId, size_t backwardId, float speedLimit = 5.0f, float laneWidth = 14.0f)
        : id(id), type(RoadType::TwoWay), nodeA(nodeA), nodeB(nodeB), isCurved(true),
          controlPoint1(cp1), controlPoint2(cp2),
          forwardSegmentId(forwardId), backwardSegmentId(backwardId),
          speedLimit(speedLimit), laneWidth(laneWidth) {}

    // One-Way Constructor (Straight)
    Road(size_t id, size_t fromNode, size_t toNode, size_t segmentId,
         float speedLimit = 5.0f, float laneWidth = 14.0f)
        : id(id), type(RoadType::OneWay), nodeA(fromNode), nodeB(toNode), isCurved(false),
          forwardSegmentId(segmentId), backwardSegmentId(0),
          speedLimit(speedLimit), laneWidth(laneWidth) {}

    // One-Way Constructor (Curved)
    Road(size_t id, size_t fromNode, size_t toNode, const sf::Vector2f& cp1, const sf::Vector2f& cp2,
         size_t segmentId, float speedLimit = 5.0f, float laneWidth = 14.0f)
        : id(id), type(RoadType::OneWay), nodeA(fromNode), nodeB(toNode), isCurved(true),
          controlPoint1(cp1), controlPoint2(cp2),
          forwardSegmentId(segmentId), backwardSegmentId(0),
          speedLimit(speedLimit), laneWidth(laneWidth) {}

    [[nodiscard]] bool isOneWay() const { return type == RoadType::OneWay; }
    [[nodiscard]] bool isTwoWay() const { return type == RoadType::TwoWay; }
};

// Aliases for compatibility
using TwoWayRoad = Road;

struct OneWayRoad {
    size_t id = 0;
    size_t fromNode = 0;
    size_t toNode = 0;
    bool isCurved = false;
    sf::Vector2f controlPoint1 = {0.0f, 0.0f};
    sf::Vector2f controlPoint2 = {0.0f, 0.0f};
    size_t segmentId = 0;
    float speedLimit = 5.0f;
    float laneWidth = 14.0f;

    OneWayRoad() = default;
    OneWayRoad(size_t id, size_t fromNode, size_t toNode, size_t segmentId,
               float speedLimit = 5.0f, float laneWidth = 14.0f)
        : id(id), fromNode(fromNode), toNode(toNode), isCurved(false),
          segmentId(segmentId), speedLimit(speedLimit), laneWidth(laneWidth) {}

    OneWayRoad(size_t id, size_t fromNode, size_t toNode, const sf::Vector2f& cp1, const sf::Vector2f& cp2,
               size_t segmentId, float speedLimit = 5.0f, float laneWidth = 14.0f)
        : id(id), fromNode(fromNode), toNode(toNode), isCurved(true),
          controlPoint1(cp1), controlPoint2(cp2),
          segmentId(segmentId), speedLimit(speedLimit), laneWidth(laneWidth) {}
};

} // namespace RoadNetwork
