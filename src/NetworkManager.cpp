#include "NetworkManager.hpp"
#include "constants.hpp"
#include "toolbox.hpp"

#include <algorithm>
#include <cmath>

static float distToSegmentSq(const sf::Vector2f& p, const sf::Vector2f& a, const sf::Vector2f& b) {
    sf::Vector2f ab = b - a;
    float lenSq = ab.x * ab.x + ab.y * ab.y;
    if (lenSq < 1e-6f) {
        float dx = p.x - a.x;
        float dy = p.y - a.y;
        return dx * dx + dy * dy;
    }
    float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lenSq, 0.0f, 1.0f);
    sf::Vector2f proj = a + ab * t;
    float dx = p.x - proj.x;
    float dy = p.y - proj.y;
    return dx * dx + dy * dy;
}

NetworkManager::NetworkManager() = default;

size_t NetworkManager::createNode(const sf::Vector2f& position, RoadNetwork::NodeType type, float radius) {
    if (type == RoadNetwork::NodeType::Intersection) {
        return createIntersection(position, radius > 0.0f ? radius : roadNetwork::defaults::intersectionRadius);
    } else {
        return createGateway(position, radius > 0.0f ? radius : roadNetwork::defaults::gatewayRadius);
    }
}

size_t NetworkManager::createIntersection(const sf::Vector2f& position, float radius) {
    size_t id = getNextNodeId();
    nodes.emplace(id, std::make_shared<RoadNetwork::Intersection>(id, position, radius));
    print::info("Created Intersection I" + std::to_string(id) + " at (" +
                std::to_string(static_cast<int>(position.x)) + ", " +
                std::to_string(static_cast<int>(position.y)) + ")");
    return id;
}

size_t NetworkManager::createGateway(const sf::Vector2f& position, float radius) {
    size_t id = getNextNodeId();
    nodes.emplace(id, std::make_shared<RoadNetwork::Gateway>(id, position, radius));
    print::info("Created Gateway G" + std::to_string(id) + " at (" +
                std::to_string(static_cast<int>(position.x)) + ", " +
                std::to_string(static_cast<int>(position.y)) + ")");
    return id;
}

bool NetworkManager::convertGatewayToIntersection(size_t nodeId) {
    auto it = nodes.find(nodeId);
    if (it == nodes.end() || !it->second->isGateway()) {
        return false;
    }

    auto oldGateway = it->second;
    auto newIntersection = std::make_shared<RoadNetwork::Intersection>(
        oldGateway->getId(),
        oldGateway->getPosition(),
        roadNetwork::defaults::intersectionRadius
    );

    newIntersection->getIncomingEdgeIds() = oldGateway->getIncomingEdgeIds();
    newIntersection->getOutgoingEdgeIds() = oldGateway->getOutgoingEdgeIds();
    newIntersection->getConnectedRoadIds() = oldGateway->getConnectedRoadIds();

    it->second = newIntersection;

    for (size_t rId : newIntersection->getConnectedRoadIds()) {
        auto rIt = roads.find(rId);
        if (rIt != roads.end()) {
            updateRoadGeometry(rIt->second);
        }
    }

    rebuildIntersectionTurns(nodeId);
    print::info("Converted Gateway G" + std::to_string(nodeId) + " to Intersection I" + std::to_string(nodeId) + ".");
    return true;
}

bool NetworkManager::convertIntersectionToGateway(size_t nodeId) {
    auto it = nodes.find(nodeId);
    if (it == nodes.end() || !it->second->isIntersection()) {
        return false;
    }

    auto oldIntersection = it->second;
    auto newGateway = std::make_shared<RoadNetwork::Gateway>(
        oldIntersection->getId(),
        oldIntersection->getPosition(),
        roadNetwork::defaults::gatewayRadius
    );

    newGateway->getIncomingEdgeIds() = oldIntersection->getIncomingEdgeIds();
    newGateway->getOutgoingEdgeIds() = oldIntersection->getOutgoingEdgeIds();
    newGateway->getConnectedRoadIds() = oldIntersection->getConnectedRoadIds();

    it->second = newGateway;

    turnLanes.erase(
        std::remove_if(turnLanes.begin(), turnLanes.end(),
                       [nodeId](const RoadNetwork::RoadSegment& seg) {
                           return seg.fromNodeId == nodeId && seg.toNodeId == nodeId;
                       }),
        turnLanes.end()
    );

    for (size_t rId : newGateway->getConnectedRoadIds()) {
        auto rIt = roads.find(rId);
        if (rIt != roads.end()) {
            updateRoadGeometry(rIt->second);
        }
    }

    print::info("Converted Intersection I" + std::to_string(nodeId) + " to Gateway G" + std::to_string(nodeId) + ".");
    return true;
}

bool NetworkManager::toggleNodeType(size_t nodeId) {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) return false;

    if (it->second->isGateway()) {
        return convertGatewayToIntersection(nodeId);
    } else {
        return convertIntersectionToGateway(nodeId);
    }
}

bool NetworkManager::removeNode(size_t nodeId) {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) return false;

    std::vector<size_t> roadsToRemove = it->second->getConnectedRoadIds();
    for (size_t rId : roadsToRemove) {
        removeRoad(rId);
    }

    nodes.erase(it);
    rebuildAllIntersections();
    print::info("Removed Node " + std::to_string(nodeId));
    return true;
}

bool NetworkManager::moveNode(size_t nodeId, const sf::Vector2f& newPosition) {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) return false;

    it->second->setPosition(newPosition);

    for (size_t rId : it->second->getConnectedRoadIds()) {
        auto roadIt = roads.find(rId);
        if (roadIt != roads.end()) {
            updateRoadGeometry(roadIt->second);
        }
    }

    rebuildIntersectionTurns(nodeId);
    for (size_t rId : it->second->getConnectedRoadIds()) {
        auto roadIt = roads.find(rId);
        if (roadIt != roads.end()) {
            size_t otherNode = (roadIt->second.nodeA == nodeId) ? roadIt->second.nodeB : roadIt->second.nodeA;
            rebuildIntersectionTurns(otherNode);
        }
    }

    return true;
}

bool NetworkManager::moveRoadControlPoint(size_t roadId, int handleIndex, const sf::Vector2f& newPos) {
    auto it = roads.find(roadId);
    if (it == roads.end() || !it->second.isCurved) return false;

    if (handleIndex == 1) {
        it->second.controlPoint1 = newPos;
    } else if (handleIndex == 2) {
        it->second.controlPoint2 = newPos;
    } else {
        return false;
    }

    updateRoadGeometry(it->second);
    rebuildIntersectionTurns(it->second.nodeA);
    rebuildIntersectionTurns(it->second.nodeB);
    return true;
}

RoadNetwork::Node* NetworkManager::getNode(size_t nodeId) {
    auto it = nodes.find(nodeId);
    return it != nodes.end() ? it->second.get() : nullptr;
}

const RoadNetwork::Node* NetworkManager::getNode(size_t nodeId) const {
    auto it = nodes.find(nodeId);
    return it != nodes.end() ? it->second.get() : nullptr;
}

RoadNetwork::Intersection* NetworkManager::getIntersection(size_t nodeId) {
    auto* n = getNode(nodeId);
    return (n && n->isIntersection()) ? static_cast<RoadNetwork::Intersection*>(n) : nullptr;
}

const RoadNetwork::Intersection* NetworkManager::getIntersection(size_t nodeId) const {
    const auto* n = getNode(nodeId);
    return (n && n->isIntersection()) ? static_cast<const RoadNetwork::Intersection*>(n) : nullptr;
}

RoadNetwork::Gateway* NetworkManager::getGateway(size_t nodeId) {
    auto* n = getNode(nodeId);
    return (n && n->isGateway()) ? static_cast<RoadNetwork::Gateway*>(n) : nullptr;
}

const RoadNetwork::Gateway* NetworkManager::getGateway(size_t nodeId) const {
    const auto* n = getNode(nodeId);
    return (n && n->isGateway()) ? static_cast<const RoadNetwork::Gateway*>(n) : nullptr;
}

int NetworkManager::findNodeAt(const sf::Vector2f& pos, float extraRadius) const {
    for (const auto& [id, node] : nodes) {
        float effectiveRadius = node->getRadius() + extraRadius;
        float dx = node->getPosition().x - pos.x;
        float dy = node->getPosition().y - pos.y;
        if ((dx * dx + dy * dy) <= (effectiveRadius * effectiveRadius)) {
            return static_cast<int>(id);
        }
    }
    return -1;
}

int NetworkManager::findRoadAt(const sf::Vector2f& pos, float maxDistance) const {
    float bestDistSq = maxDistance * maxDistance;
    int bestRoadId = -1;

    for (const auto& [id, road] : roads) {
        const auto* seg = getSegment(road.forwardSegmentId);
        if (!seg || seg->spline.empty()) continue;

        const int samples = roadNetwork::defaults::roadSampleCount;
        sf::Vector2f prev = seg->spline.eval(0.0f);
        for (int i = 1; i <= samples; ++i) {
            float t = static_cast<float>(i) / samples;
            sf::Vector2f curr = seg->spline.eval(t);
            float dSq = distToSegmentSq(pos, prev, curr);
            if (dSq < bestDistSq) {
                bestDistSq = dSq;
                bestRoadId = static_cast<int>(id);
            }
            prev = curr;
        }
    }
    return bestRoadId;
}

int NetworkManager::findRoadControlPointAt(size_t roadId, const sf::Vector2f& pos, float radius) const {
    auto it = roads.find(roadId);
    if (it == roads.end() || !it->second.isCurved) return 0;

    float rSq = radius * radius;

    float dx1 = it->second.controlPoint1.x - pos.x;
    float dy1 = it->second.controlPoint1.y - pos.y;
    if ((dx1 * dx1 + dy1 * dy1) <= rSq) return 1;

    float dx2 = it->second.controlPoint2.x - pos.x;
    float dy2 = it->second.controlPoint2.y - pos.y;
    if ((dx2 * dx2 + dy2 * dy2) <= rSq) return 2;

    return 0;
}

size_t NetworkManager::createStraightTwoWayRoad(size_t nodeA, size_t nodeB, float speedLimit, float laneWidth) {
    if (nodeA == nodeB || nodes.find(nodeA) == nodes.end() || nodes.find(nodeB) == nodes.end()) {
        print::warning("Cannot create road: invalid or identical endpoints.");
        return 0;
    }

    for (const auto& [rId, road] : roads) {
        if ((road.nodeA == nodeA && road.nodeB == nodeB) || (road.nodeA == nodeB && road.nodeB == nodeA)) {
            print::warning("Road already exists between Node " + std::to_string(nodeA) + " and Node " + std::to_string(nodeB));
            return rId;
        }
    }

    size_t roadId = getNextRoadId();
    size_t fwdSegId = getNextSegmentId();
    size_t bwdSegId = getNextSegmentId();

    RoadNetwork::RoadSegment fwdSeg(fwdSegId, nodeA, nodeB, CubicBezierSpline(), speedLimit, true, laneWidth,
                                    RoadNetwork::SegmentType::NormalLane, roadNetwork::colors::fwdLane);
    fwdSeg.parentRoadId = roadId;

    RoadNetwork::RoadSegment bwdSeg(bwdSegId, nodeB, nodeA, CubicBezierSpline(), speedLimit, true, laneWidth,
                                    RoadNetwork::SegmentType::NormalLane, roadNetwork::colors::bwdLane);
    bwdSeg.parentRoadId = roadId;

    segments.emplace(fwdSegId, fwdSeg);
    segments.emplace(bwdSegId, bwdSeg);

    RoadNetwork::Road road(roadId, nodeA, nodeB, fwdSegId, bwdSegId, speedLimit, laneWidth);
    roads.emplace(roadId, road);

    nodes[nodeA]->addConnectedRoad(roadId);
    nodes[nodeA]->addOutgoingEdge(fwdSegId);
    nodes[nodeA]->addIncomingEdge(bwdSegId);

    nodes[nodeB]->addConnectedRoad(roadId);
    nodes[nodeB]->addIncomingEdge(fwdSegId);
    nodes[nodeB]->addOutgoingEdge(bwdSegId);

    updateRoadGeometry(roads[roadId]);
    rebuildIntersectionTurns(nodeA);
    rebuildIntersectionTurns(nodeB);

    print::info("Created Straight Two-Way Road " + std::to_string(roadId) + " between Node " +
                std::to_string(nodeA) + " and Node " + std::to_string(nodeB));
    return roadId;
}

size_t NetworkManager::createCurvedTwoWayRoad(size_t nodeA, size_t nodeB,
                                              const sf::Vector2f& cp1, const sf::Vector2f& cp2,
                                              float speedLimit, float laneWidth) {
    if (nodeA == nodeB || nodes.find(nodeA) == nodes.end() || nodes.find(nodeB) == nodes.end()) {
        print::warning("Cannot create curved road: invalid endpoints.");
        return 0;
    }

    size_t roadId = getNextRoadId();
    size_t fwdSegId = getNextSegmentId();
    size_t bwdSegId = getNextSegmentId();

    RoadNetwork::RoadSegment fwdSeg(fwdSegId, nodeA, nodeB, CubicBezierSpline(), speedLimit, true, laneWidth,
                                    RoadNetwork::SegmentType::NormalLane, roadNetwork::colors::fwdLane);
    fwdSeg.parentRoadId = roadId;

    RoadNetwork::RoadSegment bwdSeg(bwdSegId, nodeB, nodeA, CubicBezierSpline(), speedLimit, true, laneWidth,
                                    RoadNetwork::SegmentType::NormalLane, roadNetwork::colors::bwdLane);
    bwdSeg.parentRoadId = roadId;

    segments.emplace(fwdSegId, fwdSeg);
    segments.emplace(bwdSegId, bwdSeg);

    RoadNetwork::Road road(roadId, nodeA, nodeB, cp1, cp2, fwdSegId, bwdSegId, speedLimit, laneWidth);
    roads.emplace(roadId, road);

    nodes[nodeA]->addConnectedRoad(roadId);
    nodes[nodeA]->addOutgoingEdge(fwdSegId);
    nodes[nodeA]->addIncomingEdge(bwdSegId);

    nodes[nodeB]->addConnectedRoad(roadId);
    nodes[nodeB]->addIncomingEdge(fwdSegId);
    nodes[nodeB]->addOutgoingEdge(bwdSegId);

    updateRoadGeometry(roads[roadId]);
    rebuildIntersectionTurns(nodeA);
    rebuildIntersectionTurns(nodeB);

    print::info("Created Curved Two-Way Road " + std::to_string(roadId) + " between Node " +
                std::to_string(nodeA) + " and Node " + std::to_string(nodeB));
    return roadId;
}

size_t NetworkManager::createStraightOneWayRoad(size_t fromNode, size_t toNode, float speedLimit, float laneWidth) {
    if (fromNode == toNode || nodes.find(fromNode) == nodes.end() || nodes.find(toNode) == nodes.end()) {
        print::warning("Cannot create one-way road: invalid endpoints.");
        return 0;
    }

    size_t roadId = getNextRoadId();
    size_t segId = getNextSegmentId();

    RoadNetwork::RoadSegment seg(segId, fromNode, toNode, CubicBezierSpline(), speedLimit, true, laneWidth,
                                 RoadNetwork::SegmentType::NormalLane, roadNetwork::colors::oneWayLane);
    seg.parentRoadId = roadId;

    segments.emplace(segId, seg);

    RoadNetwork::Road road(roadId, fromNode, toNode, segId, speedLimit, laneWidth);
    roads.emplace(roadId, road);

    nodes[fromNode]->addConnectedRoad(roadId);
    nodes[fromNode]->addOutgoingEdge(segId);

    nodes[toNode]->addConnectedRoad(roadId);
    nodes[toNode]->addIncomingEdge(segId);

    updateRoadGeometry(roads[roadId]);
    rebuildIntersectionTurns(fromNode);
    rebuildIntersectionTurns(toNode);

    print::info("Created Straight One-Way Road " + std::to_string(roadId) + " from Node " +
                std::to_string(fromNode) + " to Node " + std::to_string(toNode));
    return roadId;
}

size_t NetworkManager::createCurvedOneWayRoad(size_t fromNode, size_t toNode,
                                              const sf::Vector2f& cp1, const sf::Vector2f& cp2,
                                              float speedLimit, float laneWidth) {
    if (fromNode == toNode || nodes.find(fromNode) == nodes.end() || nodes.find(toNode) == nodes.end()) {
        print::warning("Cannot create curved one-way road: invalid endpoints.");
        return 0;
    }

    size_t roadId = getNextRoadId();
    size_t segId = getNextSegmentId();

    RoadNetwork::RoadSegment seg(segId, fromNode, toNode, CubicBezierSpline(), speedLimit, true, laneWidth,
                                 RoadNetwork::SegmentType::NormalLane, roadNetwork::colors::oneWayLane);
    seg.parentRoadId = roadId;

    segments.emplace(segId, seg);

    RoadNetwork::Road road(roadId, fromNode, toNode, cp1, cp2, segId, speedLimit, laneWidth);
    roads.emplace(roadId, road);

    nodes[fromNode]->addConnectedRoad(roadId);
    nodes[fromNode]->addOutgoingEdge(segId);

    nodes[toNode]->addConnectedRoad(roadId);
    nodes[toNode]->addIncomingEdge(segId);

    updateRoadGeometry(roads[roadId]);
    rebuildIntersectionTurns(fromNode);
    rebuildIntersectionTurns(toNode);

    print::info("Created Curved One-Way Road " + std::to_string(roadId) + " from Node " +
                std::to_string(fromNode) + " to Node " + std::to_string(toNode));
    return roadId;
}

bool NetworkManager::removeRoad(size_t roadId) {
    auto it = roads.find(roadId);
    if (it == roads.end()) return false;

    size_t nodeA = it->second.nodeA;
    size_t nodeB = it->second.nodeB;
    size_t fwdId = it->second.forwardSegmentId;
    size_t bwdId = it->second.backwardSegmentId;

    if (nodes.find(nodeA) != nodes.end()) {
        nodes[nodeA]->removeConnectedRoad(roadId);
        nodes[nodeA]->removeOutgoingEdge(fwdId);
        if (bwdId != 0) {
            nodes[nodeA]->removeIncomingEdge(bwdId);
        }
    }

    if (nodes.find(nodeB) != nodes.end()) {
        nodes[nodeB]->removeConnectedRoad(roadId);
        nodes[nodeB]->removeIncomingEdge(fwdId);
        if (bwdId != 0) {
            nodes[nodeB]->removeOutgoingEdge(bwdId);
        }
    }

    segments.erase(fwdId);
    if (bwdId != 0) {
        segments.erase(bwdId);
    }
    roads.erase(it);

    rebuildIntersectionTurns(nodeA);
    rebuildIntersectionTurns(nodeB);
    print::info("Removed Road " + std::to_string(roadId));
    return true;
}

RoadNetwork::Road* NetworkManager::getRoad(size_t roadId) {
    auto it = roads.find(roadId);
    return it != roads.end() ? &it->second : nullptr;
}

const RoadNetwork::Road* NetworkManager::getRoad(size_t roadId) const {
    auto it = roads.find(roadId);
    return it != roads.end() ? &it->second : nullptr;
}

RoadNetwork::RoadSegment* NetworkManager::getSegment(size_t segmentId) {
    auto it = segments.find(segmentId);
    return it != segments.end() ? &it->second : nullptr;
}

const RoadNetwork::RoadSegment* NetworkManager::getSegment(size_t segmentId) const {
    auto it = segments.find(segmentId);
    return it != segments.end() ? &it->second : nullptr;
}

void NetworkManager::updateRoadGeometry(RoadNetwork::Road& road) {
    auto itA = nodes.find(road.nodeA);
    auto itB = nodes.find(road.nodeB);
    if (itA == nodes.end() || itB == nodes.end()) return;

    auto itFwd = segments.find(road.forwardSegmentId);
    if (itFwd == segments.end()) return;

    const sf::Vector2f posA = itA->second->getPosition();
    const sf::Vector2f posB = itB->second->getPosition();
    const float radA = itA->second->getRadius();
    const float radB = itB->second->getRadius();
    const float wOff = road.laneWidth * 0.5f;

    if (road.isOneWay()) {
        // One-Way Road: single lane running directly along the road centerline
        if (!road.isCurved) {
            sf::Vector2f diff = posB - posA;
            float len = std::hypot(diff.x, diff.y);
            if (len < 1e-3f) return;

            sf::Vector2f u = diff / len;
            float trimA = std::min(radA, len * roadNetwork::defaults::roadTrimRatio);
            float trimB = std::min(radB, len * roadNetwork::defaults::roadTrimRatio);

            sf::Vector2f startCenter = posA + u * trimA;
            sf::Vector2f endCenter = posB - u * trimB;
            float effLen = std::max(1.0f, len - trimA - trimB);

            sf::Vector2f p0 = startCenter;
            sf::Vector2f p1 = p0 + u * (effLen / 3.0f);
            sf::Vector2f p2 = endCenter - u * (effLen / 3.0f);
            sf::Vector2f p3 = endCenter;

            itFwd->second.spline = CubicBezierSpline();
            itFwd->second.spline.addSegment(CubicBezierCurve(p0, p1, p2, p3));
        } else {
            sf::Vector2f t0 = road.controlPoint1 - posA;
            if (std::hypot(t0.x, t0.y) < 1e-3f) t0 = road.controlPoint2 - posA;
            float len0 = std::hypot(t0.x, t0.y);
            sf::Vector2f u0 = len0 > 1e-4f ? t0 / len0 : sf::Vector2f(1, 0);

            sf::Vector2f t3 = posB - road.controlPoint2;
            if (std::hypot(t3.x, t3.y) < 1e-3f) t3 = posB - road.controlPoint1;
            float len3 = std::hypot(t3.x, t3.y);
            sf::Vector2f u3 = len3 > 1e-4f ? t3 / len3 : sf::Vector2f(1, 0);

            sf::Vector2f startCenter = posA + u0 * radA;
            sf::Vector2f endCenter = posB - u3 * radB;

            sf::Vector2f p0 = startCenter;
            sf::Vector2f p1 = road.controlPoint1;
            sf::Vector2f p2 = road.controlPoint2;
            sf::Vector2f p3 = endCenter;

            itFwd->second.spline = CubicBezierSpline();
            itFwd->second.spline.addSegment(CubicBezierCurve(p0, p1, p2, p3));
        }
    } else {
        // Two-Way Road: two parallel lanes offset from centerline
        auto itBwd = segments.find(road.backwardSegmentId);
        if (itBwd == segments.end()) return;

        if (!road.isCurved) {
            sf::Vector2f diff = posB - posA;
            float len = std::hypot(diff.x, diff.y);
            if (len < 1e-3f) return;

            sf::Vector2f u = diff / len;
            sf::Vector2f n(-u.y, u.x);

            float trimA = std::min(radA, len * roadNetwork::defaults::roadTrimRatio);
            float trimB = std::min(radB, len * roadNetwork::defaults::roadTrimRatio);

            sf::Vector2f startCenter = posA + u * trimA;
            sf::Vector2f endCenter = posB - u * trimB;
            float effLen = std::max(1.0f, len - trimA - trimB);

            // Forward Lane (A -> B)
            sf::Vector2f p0 = startCenter + n * wOff;
            sf::Vector2f p1 = p0 + u * (effLen / 3.0f);
            sf::Vector2f p2 = endCenter + n * wOff - u * (effLen / 3.0f);
            sf::Vector2f p3 = endCenter + n * wOff;

            itFwd->second.spline = CubicBezierSpline();
            itFwd->second.spline.addSegment(CubicBezierCurve(p0, p1, p2, p3));

            // Backward Lane (B -> A)
            sf::Vector2f q0 = endCenter - n * wOff;
            sf::Vector2f q1 = q0 - u * (effLen / 3.0f);
            sf::Vector2f q2 = startCenter - n * wOff + u * (effLen / 3.0f);
            sf::Vector2f q3 = startCenter - n * wOff;

            itBwd->second.spline = CubicBezierSpline();
            itBwd->second.spline.addSegment(CubicBezierCurve(q0, q1, q2, q3));
        } else {
            sf::Vector2f t0 = road.controlPoint1 - posA;
            if (std::hypot(t0.x, t0.y) < 1e-3f) t0 = road.controlPoint2 - posA;
            float len0 = std::hypot(t0.x, t0.y);
            sf::Vector2f u0 = len0 > 1e-4f ? t0 / len0 : sf::Vector2f(1, 0);
            sf::Vector2f n0(-u0.y, u0.x);

            sf::Vector2f t3 = posB - road.controlPoint2;
            if (std::hypot(t3.x, t3.y) < 1e-3f) t3 = posB - road.controlPoint1;
            float len3 = std::hypot(t3.x, t3.y);
            sf::Vector2f u3 = len3 > 1e-4f ? t3 / len3 : sf::Vector2f(1, 0);
            sf::Vector2f n3(-u3.y, u3.x);

            sf::Vector2f t1 = road.controlPoint2 - posA;
            float len1 = std::hypot(t1.x, t1.y);
            sf::Vector2f u1 = len1 > 1e-4f ? t1 / len1 : u0;
            sf::Vector2f n1(-u1.y, u1.x);

            sf::Vector2f t2 = posB - road.controlPoint1;
            float len2 = std::hypot(t2.x, t2.y);
            sf::Vector2f u2 = len2 > 1e-4f ? t2 / len2 : u3;
            sf::Vector2f n2(-u2.y, u2.x);

            sf::Vector2f startCenter = posA + u0 * radA;
            sf::Vector2f endCenter = posB - u3 * radB;

            // Forward Lane (A -> B)
            sf::Vector2f p0 = startCenter + n0 * wOff;
            sf::Vector2f p1 = road.controlPoint1 + n1 * wOff;
            sf::Vector2f p2 = road.controlPoint2 + n2 * wOff;
            sf::Vector2f p3 = endCenter + n3 * wOff;

            itFwd->second.spline = CubicBezierSpline();
            itFwd->second.spline.addSegment(CubicBezierCurve(p0, p1, p2, p3));

            // Backward Lane (B -> A)
            sf::Vector2f q0 = endCenter - n3 * wOff;
            sf::Vector2f q1 = road.controlPoint2 - n2 * wOff;
            sf::Vector2f q2 = road.controlPoint1 - n1 * wOff;
            sf::Vector2f q3 = startCenter - n0 * wOff;

            itBwd->second.spline = CubicBezierSpline();
            itBwd->second.spline.addSegment(CubicBezierCurve(q0, q1, q2, q3));
        }
    }
}

void NetworkManager::rebuildIntersectionTurns(size_t nodeId) {
    auto nodeIt = nodes.find(nodeId);
    if (nodeIt == nodes.end() || !nodeIt->second->isIntersection()) {
        return;
    }

    turnLanes.erase(
        std::remove_if(turnLanes.begin(), turnLanes.end(),
                       [nodeId](const RoadNetwork::RoadSegment& seg) {
                           return seg.fromNodeId == nodeId && seg.toNodeId == nodeId;
                       }),
        turnLanes.end()
    );

    const auto& inEdgeIds = nodeIt->second->getIncomingEdgeIds();
    const auto& outEdgeIds = nodeIt->second->getOutgoingEdgeIds();

    for (size_t inId : inEdgeIds) {
        auto inIt = segments.find(inId);
        if (inIt == segments.end() || inIt->second.spline.empty()) continue;

        sf::Vector2f pIn = inIt->second.spline.eval(1.0f);
        sf::Vector2f vIn = inIt->second.spline.evalTangent(1.0f);
        float lenIn = std::hypot(vIn.x, vIn.y);
        sf::Vector2f uIn = lenIn > 1e-4f ? vIn / lenIn : sf::Vector2f(1, 0);

        for (size_t outId : outEdgeIds) {
            auto outIt = segments.find(outId);
            if (outIt == segments.end() || outIt->second.spline.empty()) continue;

            if (inIt->second.fromNodeId == outIt->second.toNodeId && inEdgeIds.size() > 1) {
                continue;
            }

            sf::Vector2f pOut = outIt->second.spline.eval(0.0f);
            sf::Vector2f vOut = outIt->second.spline.evalTangent(0.0f);
            float lenOut = std::hypot(vOut.x, vOut.y);
            sf::Vector2f uOut = lenOut > 1e-4f ? vOut / lenOut : sf::Vector2f(1, 0);

            float dist = std::hypot(pOut.x - pIn.x, pOut.y - pIn.y);
            float h = std::min(dist / roadNetwork::defaults::handleScaleRatio,
                               nodeIt->second->getRadius() * roadNetwork::defaults::maxTurnHandleRatio);

            sf::Vector2f t0 = pIn;
            sf::Vector2f t1 = pIn + uIn * h;
            sf::Vector2f t2 = pOut - uOut * h;
            sf::Vector2f t3 = pOut;

            CubicBezierSpline turnSpline;
            turnSpline.addSegment(CubicBezierCurve(t0, t1, t2, t3));

            RoadNetwork::RoadSegment turnLane(getNextSegmentId(), nodeId, nodeId, turnSpline,
                                             roadNetwork::defaults::turnLaneSpeedLimit, true,
                                             roadNetwork::defaults::turnLaneWidth,
                                             RoadNetwork::SegmentType::IntersectionTurn,
                                             roadNetwork::colors::turnLane);
            turnLanes.push_back(turnLane);
        }
    }
}

void NetworkManager::rebuildAllIntersections() {
    turnLanes.clear();
    for (const auto& [id, node] : nodes) {
        if (node->isIntersection()) {
            rebuildIntersectionTurns(id);
        }
    }
}

void NetworkManager::clear() {
    nodes.clear();
    segments.clear();
    roads.clear();
    turnLanes.clear();
    nextNodeId = 1;
    nextSegmentId = 1;
    nextRoadId = 1;
    print::info("Cleared road network.");
}

void NetworkManager::loadSampleNetwork(unsigned int windowWidth, unsigned int windowHeight) {
    clear();

    float cx = windowWidth * 0.5f;
    float cy = windowHeight * 0.5f;
    float spread = std::min(windowWidth, windowHeight) * 0.28f;

    size_t nCenter = createIntersection(sf::Vector2f(cx, cy), roadNetwork::defaults::intersectionRadius + 2.0f);
    size_t nNorth = createGateway(sf::Vector2f(cx, cy - spread * 1.1f), roadNetwork::defaults::gatewayRadius);
    size_t nSouth = createGateway(sf::Vector2f(cx, cy + spread * 1.1f), roadNetwork::defaults::gatewayRadius);
    size_t nWest = createGateway(sf::Vector2f(cx - spread * 1.2f, cy), roadNetwork::defaults::gatewayRadius);
    
    size_t nEastJunction = createIntersection(sf::Vector2f(cx + spread * 0.9f, cy), roadNetwork::defaults::intersectionRadius);
    size_t nEastNorth = createGateway(sf::Vector2f(cx + spread * 1.4f, cy - spread * 0.8f), roadNetwork::defaults::gatewayRadius);
    size_t nEastSouth = createGateway(sf::Vector2f(cx + spread * 1.4f, cy + spread * 0.8f), roadNetwork::defaults::gatewayRadius);

    createStraightTwoWayRoad(nCenter, nNorth);
    createStraightTwoWayRoad(nCenter, nSouth);
    createStraightTwoWayRoad(nCenter, nWest);
    createStraightTwoWayRoad(nCenter, nEastJunction);

    createCurvedTwoWayRoad(
        nEastJunction, nEastNorth,
        sf::Vector2f(cx + spread * 1.0f, cy - spread * 0.5f),
        sf::Vector2f(cx + spread * 1.2f, cy - spread * 0.7f)
    );

    createCurvedTwoWayRoad(
        nEastJunction, nEastSouth,
        sf::Vector2f(cx + spread * 1.0f, cy + spread * 0.5f),
        sf::Vector2f(cx + spread * 1.2f, cy + spread * 0.7f)
    );

    print::info("Loaded rich sample road network with intersections, turn lanes, and curved highways.");
}

std::vector<size_t> NetworkManager::getIncomingSegments(size_t nodeId) const {
    auto it = nodes.find(nodeId);
    return it != nodes.end() ? it->second->getIncomingEdgeIds() : std::vector<size_t>{};
}

std::vector<size_t> NetworkManager::getOutgoingSegments(size_t nodeId) const {
    auto it = nodes.find(nodeId);
    return it != nodes.end() ? it->second->getOutgoingEdgeIds() : std::vector<size_t>{};
}

std::vector<size_t> NetworkManager::getConnectedRoads(size_t nodeId) const {
    auto it = nodes.find(nodeId);
    return it != nodes.end() ? it->second->getConnectedRoadIds() : std::vector<size_t>{};
}
