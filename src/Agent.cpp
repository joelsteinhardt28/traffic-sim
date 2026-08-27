#include "Agent.hpp"
#include "constants.hpp"

#include <cmath>

Agent::Agent(float speed, const sf::Vector2f& size, const sf::Color& color)
    : t(0.0f), speed(speed), position(0.0f, 0.0f), color(color), angle(0.0f), size(size) {
    updateSprite();
}

Agent::Agent(float initialT, float speed, const sf::Vector2f& size, const sf::Color& color)
    : t(initialT), speed(speed), position(0.0f, 0.0f), color(color), angle(0.0f), size(size) {
    updateSprite();
}

void Agent::updateSprite() {
    sprite.setSize(size);
    sprite.setOrigin(size.x * 0.5f, size.y * 0.5f);
    sprite.setPosition(position);
    sprite.setRotation(angle);
    sprite.setFillColor(color);
}

void Agent::update(const CubicBezierSpline& spline) {
    if (spline.empty()) {
        return;
    }

    t += speed;
    if (t > 1.0f) {
        t = std::fmod(t, 1.0f);
    } else if (t < 0.0f) {
        t = 1.0f + std::fmod(t, 1.0f);
    }

    // Clamp to valid range [0, 1]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // Fetch position from spline
    sf::Vector2f pos = spline.eval(t);
    setPosition(pos);

    // Fetch tangent from spline and compute orientation angle
    sf::Vector2f tan = spline.evalTangent(t);
    float len = std::hypot(tan.x, tan.y);
    if (len > 1e-4f) {
        float rotAngle = std::atan2(tan.y, tan.x) * 180.0f / mathConstants::PI;
        setRotation(rotAngle);
    }
}

void Agent::reset() {
    t = 0.0f;
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
    return speed;
}

void Agent::setSpeed(float newSpeed) {
    speed = newSpeed;
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