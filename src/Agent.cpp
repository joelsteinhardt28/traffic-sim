#include "Agent.hpp"
#include "constants.hpp"

#include <algorithm>
#include <cmath>

Agent::Agent(float desiredSpeed, const sf::Vector2f& size, const sf::Color& color)
    : t(0.0f),
      currentSpeed(desiredSpeed),
      desiredSpeed(desiredSpeed),
      lookaheadDistance(80.0f),
      acceleration(0.08f),
      deceleration(0.18f),
      frictionCoeff(0.1f),
      minSpeed(0.5f),
      position(0.0f, 0.0f),
      color(color),
      angle(0.0f),
      size(size) {
    updateSprite();
}

Agent::Agent(float initialT, float desiredSpeed, const sf::Vector2f& size, const sf::Color& color)
    : t(initialT),
      currentSpeed(desiredSpeed),
      desiredSpeed(desiredSpeed),
      lookaheadDistance(80.0f),
      acceleration(0.08f),
      deceleration(0.18f),
      frictionCoeff(2.0f),
      minSpeed(0.5f),
      position(0.0f, 0.0f),
      color(color),
      angle(0.0f),
      size(size) {
    updateSprite();
}

void Agent::updateSprite() {
    sprite.setSize(size);
    sprite.setOrigin(size.x * 0.5f, size.y * 0.5f);
    sprite.setPosition(position);
    sprite.setRotation(angle);
    sprite.setFillColor(color);
}

float Agent::calculateCurvature(const sf::Vector2f& d1, const sf::Vector2f& d2) const {
    float d1Mag = std::hypot(d1.x, d1.y);
    if (d1Mag < 1e-4f) return 0.0f;
    return std::abs(d1.x * d2.y - d1.y * d2.x) / std::pow(d1Mag, 3.0f);
}

float Agent::calculateMaxCorneringSpeed(float curvature) const {
    if (curvature <= 1e-4f) {
        return desiredSpeed;
    }
    float r = 1.0f / curvature;
    float corneringSpeed = std::sqrt(frictionCoeff * r);
    return std::max(minSpeed, std::min(desiredSpeed, corneringSpeed));
}

void Agent::update(const CubicBezierSpline& spline) {
    if (spline.empty()) return;

    size_t numSegments = spline.getSegmentCount();
    if (numSegments == 0) return;

    // 1. Fetch first derivative (tangent) at current position t
    sf::Vector2f d1 = spline.evalTangent(t);
    float d1Mag = std::hypot(d1.x, d1.y);
    if (d1Mag < 1e-4f) return; // Avoid division by zero

    // 2. Lookahead mapping: \Delta t = L / (|B'(t)| * segments)
    float deltaT = lookaheadDistance / (d1Mag * static_cast<float>(numSegments));
    float tAhead = t + deltaT;
    if (tAhead > 1.0f) {
        tAhead = std::fmod(tAhead, 1.0f);
    } else if (tAhead < 0.0f) {
        tAhead = 1.0f + std::fmod(tAhead, 1.0f);
    }

    // 3. Fetch tangent and second derivative at t_ahead
    sf::Vector2f d1Ahead = spline.evalTangent(tAhead);
    sf::Vector2f d2Ahead = spline.evalSecondDerivative(tAhead);

    // 4. Calculate curvature at t_ahead and current t
    float curvatureAhead = calculateCurvature(d1Ahead, d2Ahead);

    sf::Vector2f d2 = spline.evalSecondDerivative(t);
    float curvatureCurrent = calculateCurvature(d1, d2);

    float effectiveCurvature = std::max(curvatureAhead, curvatureCurrent);

    // 5. Calculate v_max using curvature
    float vMax = calculateMaxCorneringSpeed(effectiveCurvature);

    // 6. Interpolate currentSpeed towards v_max using fixed acceleration / deceleration rate
    if (currentSpeed < vMax) {
        currentSpeed = std::min(vMax, currentSpeed + acceleration);
    } else if (currentSpeed > vMax) {
        currentSpeed = std::max(vMax, currentSpeed - deceleration);
    }

    // 7. Dynamic arc-length step: dt = (currentSpeed / |B'(t)|) / segments
    float dtStep = (currentSpeed / d1Mag) / static_cast<float>(numSegments);
    t += dtStep;
    if (t > 1.0f) {
        t = std::fmod(t, 1.0f);
    } else if (t < 0.0f) {
        t = 1.0f + std::fmod(t, 1.0f);
    }

    // Clamp t to [0, 1]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // 8. Update agent position and heading orientation
    sf::Vector2f pos = spline.eval(t);
    setPosition(pos);

    sf::Vector2f currentTan = spline.evalTangent(t);
    float tanLen = std::hypot(currentTan.x, currentTan.y);
    if (tanLen > 1e-4f) {
        setRotation(std::atan2(currentTan.y, currentTan.x) * mathConstants::RAD_TO_DEG);
    }
}

void Agent::reset() {
    t = 0.0f;
    currentSpeed = desiredSpeed;
    position = {0.0f, 0.0f};
    angle = 0.0f;
    updateSprite();
}

float Agent::getT() const {
    return t;
}

void Agent::setT(float newT) {
    t = newT;
}

float Agent::getSpeed() const {
    return currentSpeed;
}

void Agent::setSpeed(float newSpeed) {
    desiredSpeed = newSpeed;
    currentSpeed = newSpeed;
}

float Agent::getCurrentSpeed() const {
    return currentSpeed;
}

void Agent::setCurrentSpeed(float speed) {
    currentSpeed = speed;
}

float Agent::getDesiredSpeed() const {
    return desiredSpeed;
}

void Agent::setDesiredSpeed(float speed) {
    desiredSpeed = speed;
}

float Agent::getLookaheadDistance() const {
    return lookaheadDistance;
}

void Agent::setLookaheadDistance(float distance) {
    lookaheadDistance = distance;
}

float Agent::getAcceleration() const {
    return acceleration;
}

void Agent::setAcceleration(float accel) {
    acceleration = accel;
}

float Agent::getDeceleration() const {
    return deceleration;
}

void Agent::setDeceleration(float decel) {
    deceleration = decel;
}

float Agent::getFrictionCoeff() const {
    return frictionCoeff;
}

void Agent::setFrictionCoeff(float mu) {
    frictionCoeff = mu;
}

float Agent::getMinSpeed() const {
    return minSpeed;
}

void Agent::setMinSpeed(float minSpd) {
    minSpeed = minSpd;
}

const sf::Vector2f& Agent::getPosition() const {
    return position;
}

void Agent::setPosition(const sf::Vector2f& newPosition) {
    position = newPosition;
    sprite.setPosition(position);
}

const sf::Vector2f& Agent::getOrigin() const {
    return sprite.getOrigin();
}

void Agent::setOrigin(const sf::Vector2f& origin) {
    sprite.setOrigin(origin);
}

float Agent::getRotation() const {
    return angle;
}

void Agent::setRotation(float newAngle) {
    angle = newAngle;
    sprite.setRotation(angle);
}

const sf::Color& Agent::getFillColor() const {
    return color;
}

void Agent::setFillColor(const sf::Color& newColor) {
    color = newColor;
    sprite.setFillColor(color);
}

sf::Vector2f Agent::getSize() const {
    return size;
}

void Agent::setSize(const sf::Vector2f& newSize) {
    size = newSize;
    sprite.setSize(size);
    sprite.setOrigin(size.x * 0.5f, size.y * 0.5f);
}

const sf::RectangleShape& Agent::getSprite() const {
    return sprite;
}

sf::RectangleShape& Agent::getSprite() {
    return sprite;
}

void Agent::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(sprite, states);
}