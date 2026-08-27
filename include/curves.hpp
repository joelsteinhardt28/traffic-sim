#pragma once

#include <array>
#include <utility>
#include <vector>

#include <SFML/Graphics.hpp>


/**
 * A class representing a cubic Bézier curve
 */
class CubicBezierCurve {
private:
    const int degree = 3;
    sf::Vector2f controlPoints[4];

    // Performs deCasteljau's algorithm to evaluate the curve at `t` across `steps` reduction levels.
    std::array<sf::Vector2f, 4> deCasteljau(float t, int steps) const;

public:
    CubicBezierCurve(const std::vector<sf::Vector2f>& points);
    CubicBezierCurve(const sf::Vector2f& p0, const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& p3);
    sf::Vector2f getControlPoint(int index) const;
    sf::Vector2f eval(float t) const;
    sf::Vector2f evalTangent(float t) const;
    sf::Vector2f evalSecondDerivative(float t) const;
};


/**
 * A class representing a cubic Bézier spline, which is a sequence of connected cubic Bézier curves.
 */
class CubicBezierSpline {
private:
    std::vector<CubicBezierCurve> segments;

    // Maps global parameter `t` in [0, 1] to the corresponding segment index and local parameter within that segment.
    std::pair<size_t, float> getSegmentAndLocalT(float t) const;

public:
    CubicBezierSpline() = default;
    sf::Vector2f eval(float t) const;
    sf::Vector2f evalTangent(float t) const;
    sf::Vector2f evalSecondDerivative(float t) const;
    
    void addSegment(const CubicBezierCurve& segment);
    // Automatically uses the last point of the previous segment as the first point of the new segment
    void addContinuousSegment(const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& p3);

    // Generates a continuous line strip fro SFML rendering
    sf::VertexArray getVertices(int resolutionPerCurve = 20) const;

    [[nodiscard]] bool empty() const { return segments.empty(); }
    [[nodiscard]] size_t getSegmentCount() const { return segments.size(); }
    void clear() { segments.clear(); }
    [[nodiscard]] const std::vector<CubicBezierCurve>& getSegments() const { return segments; }
};