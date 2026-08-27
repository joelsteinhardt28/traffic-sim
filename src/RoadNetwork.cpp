#include "RoadNetwork.hpp"

namespace RoadNetwork {

Node::Node(size_t id, const sf::Vector2f& position, float radius)
    : id(id), position(position), radius(radius) {}

void Node::addIncomingEdge(size_t edgeId) {
    if (std::find(incomingEdgeIds.begin(), incomingEdgeIds.end(), edgeId) == incomingEdgeIds.end()) {
        incomingEdgeIds.push_back(edgeId);
    }
}

void Node::removeIncomingEdge(size_t edgeId) {
    incomingEdgeIds.erase(std::remove(incomingEdgeIds.begin(), incomingEdgeIds.end(), edgeId), incomingEdgeIds.end());
}

void Node::addOutgoingEdge(size_t edgeId) {
    if (std::find(outgoingEdgeIds.begin(), outgoingEdgeIds.end(), edgeId) == outgoingEdgeIds.end()) {
        outgoingEdgeIds.push_back(edgeId);
    }
}

void Node::removeOutgoingEdge(size_t edgeId) {
    outgoingEdgeIds.erase(std::remove(outgoingEdgeIds.begin(), outgoingEdgeIds.end(), edgeId), outgoingEdgeIds.end());
}

void Node::addConnectedRoad(size_t roadId) {
    if (std::find(connectedRoadIds.begin(), connectedRoadIds.end(), roadId) == connectedRoadIds.end()) {
        connectedRoadIds.push_back(roadId);
    }
}

void Node::removeConnectedRoad(size_t roadId) {
    connectedRoadIds.erase(std::remove(connectedRoadIds.begin(), connectedRoadIds.end(), roadId), connectedRoadIds.end());
}

Intersection::Intersection(size_t id, const sf::Vector2f& position, float radius)
    : Node(id, position, radius) {}

Gateway::Gateway(size_t id, const sf::Vector2f& position, float radius)
    : Node(id, position, radius) {}

} // namespace RoadNetwork
