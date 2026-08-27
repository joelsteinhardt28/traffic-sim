#include "curves.hpp"

#include <array>
#include <stdexcept>


// ----------------------
// CLASS CubicBezierCurve
// ----------------------

CubicBezierCurve::CubicBezierCurve(const std::vector<sf::Vector2f>& points) {
    if (points.size() < 4) {
        throw std::invalid_argument("CubicBezierCurve requires at least 4 control points");
    }
    for (size_t i = 0; i < 4; ++i) {
        controlPoints[i] = points[i];
    }
}

CubicBezierCurve::CubicBezierCurve(const sf::Vector2f& p0, const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& p3)
    : controlPoints{p0, p1, p2, p3} {}


std::array<sf::Vector2f, 4> CubicBezierCurve::deCasteljau(float t, int steps) const {
    if (t < 0.0f || t > 1.0f) {
        throw std::out_of_range("Parameter t must be in the range [0, 1]");
    }

    std::array<sf::Vector2f, 4> points = {controlPoints[0], controlPoints[1], controlPoints[2], controlPoints[3]};

    for (int r = 1; r <= steps; ++r) {
        for (int i = 0; i <= degree - r; ++i) {
            points[i] = (1.0f - t) * points[i] + t * points[i + 1];
        }
    }

    return points;
}


sf::Vector2f CubicBezierCurve::getControlPoint(int index) const {
    if (index < 0 || index >= 4) {
        throw std::out_of_range("Control point index must be in the range [0, 3]");
    }
    return controlPoints[index];
}


sf::Vector2f CubicBezierCurve::eval(float t) const {
    return deCasteljau(t, degree)[0];
}


sf::Vector2f CubicBezierCurve::evalTangent(float t) const {
    std::array<sf::Vector2f, 4> points = deCasteljau(t, degree - 1);
    return static_cast<float>(degree) * (points[1] - points[0]);
}


sf::Vector2f CubicBezierCurve::evalSecondDerivative(float t) const {
    float u = 1.0f - t;
    sf::Vector2f p0 = controlPoints[0], p1 = controlPoints[1];
    sf::Vector2f p2 = controlPoints[2], p3 = controlPoints[3];

    return 6.0f * u * (p2 - 2.0f * p1 + p0) + 6.0f * t * (p3 - 2.0f * p2 + p1);
}


// -----------------------
// CLASS CubicBezierSpline
// -----------------------

std::pair<size_t, float> CubicBezierSpline::getSegmentAndLocalT(float t) const {
    if (t < 0.0f || t > 1.0f) {
        throw std::out_of_range("Parameter t must be in the range [0, 1]");
    }
    if (segments.empty()) {
        throw std::runtime_error("Spline contains no segments");
    }

    float scaledT = t * static_cast<float>(segments.size());
    size_t index = static_cast<size_t>(scaledT);

    if (index >= segments.size()) {
        index = segments.size() - 1;
    }

    float localT = scaledT - static_cast<float>(index);
    if (localT > 1.0f) localT = 1.0f;
    if (localT < 0.0f) localT = 0.0f;

    return {index, localT};
}


sf::Vector2f CubicBezierSpline::eval(float t) const {
    auto [index, localT] = getSegmentAndLocalT(t);
    return segments[index].eval(localT);
}


sf::Vector2f CubicBezierSpline::evalTangent(float t) const {
    auto [index, localT] = getSegmentAndLocalT(t);
    return segments[index].evalTangent(localT);
}


sf::Vector2f CubicBezierSpline::evalSecondDerivative(float t) const {
    auto [index, localT] = getSegmentAndLocalT(t);
    return segments[index].evalSecondDerivative(localT);
}


void CubicBezierSpline::addSegment(const CubicBezierCurve& segment) {
    segments.push_back(segment);
}


void CubicBezierSpline::addContinuousSegment(sf::Vector2f const& p1, sf::Vector2f const& p2, sf::Vector2f const& p3) {
    if (segments.empty()) return;

    sf::Vector2f p0 = segments.back().getControlPoint(3);
    segments.emplace_back(p0, p1, p2, p3);
}


sf::VertexArray CubicBezierSpline::getVertices(int resolutionPerCurve) const {
    sf::VertexArray vertices(sf::LineStrip);

    for (const CubicBezierCurve& segment: segments) {
        for (int i = 0; i <= resolutionPerCurve; ++i) {
            float t = static_cast<float>(i) / resolutionPerCurve;
            vertices.append(sf::Vertex(segment.eval(t), sf::Color::White));
        }
    }

    return vertices;
}