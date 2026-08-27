#pragma once

#include <SFML/Graphics.hpp>
#include "curves.hpp"

/**
 * @brief Represents an autonomous vehicle / traffic agent navigating a Bézier spline.
 *
 * Implements physical arc-length parameterization, lookahead braking behavior
 * when approaching tight curvature, and smooth acceleration/deceleration kinematics.
 */
class Agent : public sf::Drawable {
public:
    /**
     * @brief Constructs a new Agent with specified desired speed, size, and color.
     * @param desiredSpeed Target cruising speed in physical units (px/frame, default 5.0f).
     * @param size Dimensions of the agent rectangle (default 32x16).
     * @param color Fill color of the agent (default red).
     */
    explicit Agent(float desiredSpeed = 5.0f,
                   const sf::Vector2f& size = sf::Vector2f(32.0f, 16.0f),
                   const sf::Color& color = sf::Color(255, 50, 50));

    /**
     * @brief Constructs a new Agent with an initial progress parameter t.
     * @param initialT Initial t parameter along the spline [0.0, 1.0].
     * @param desiredSpeed Target cruising speed in physical units.
     * @param size Dimensions of the agent rectangle.
     * @param color Fill color of the agent.
     */
    Agent(float initialT, float desiredSpeed,
          const sf::Vector2f& size = sf::Vector2f(32.0f, 16.0f),
          const sf::Color& color = sf::Color(255, 50, 50));

    /**
     * @brief Updates the agent's position and orientation along the given spline.
     * Implements lookahead mapping to calculate cornering speed limits,
     * accelerates/decelerates towards v_max, and advances along the curve via arc-length parameterization.
     * @param spline The Bézier spline the agent is traversing.
     */
    void update(const CubicBezierSpline& spline);

    /**
     * @brief Resets the agent's progress t back to 0.0f and resets speed.
     */
    void reset();

    /**
     * @brief Calculates the 2D curvature kappa = |x'y'' - y'x''| / |B'(t)|^3 from 1st and 2nd derivatives.
     */
    [[nodiscard]] float calculateCurvature(const sf::Vector2f& d1, const sf::Vector2f& d2) const;

    /**
     * @brief Calculates maximum cornering speed allowed for a given curvature.
     */
    [[nodiscard]] float calculateMaxCorneringSpeed(float curvature) const;

    // --- Accessors and Mutators ---

    [[nodiscard]] float getT() const;
    void setT(float t);

    [[nodiscard]] float getSpeed() const;
    void setSpeed(float speed);

    [[nodiscard]] float getCurrentSpeed() const;
    void setCurrentSpeed(float speed);

    [[nodiscard]] float getDesiredSpeed() const;
    void setDesiredSpeed(float speed);

    [[nodiscard]] float getLookaheadDistance() const;
    void setLookaheadDistance(float distance);

    [[nodiscard]] float getAcceleration() const;
    void setAcceleration(float accel);

    [[nodiscard]] float getDeceleration() const;
    void setDeceleration(float decel);

    [[nodiscard]] float getFrictionCoeff() const;
    void setFrictionCoeff(float mu);

    [[nodiscard]] float getMinSpeed() const;
    void setMinSpeed(float minSpd);

    [[nodiscard]] const sf::Vector2f& getPosition() const;
    void setPosition(const sf::Vector2f& position);

    [[nodiscard]] const sf::Vector2f& getOrigin() const;
    void setOrigin(const sf::Vector2f& origin);

    [[nodiscard]] float getRotation() const;
    void setRotation(float angle);

    [[nodiscard]] const sf::Color& getFillColor() const;
    void setFillColor(const sf::Color& color);

    [[nodiscard]] sf::Vector2f getSize() const;
    void setSize(const sf::Vector2f& size);

    [[nodiscard]] const sf::RectangleShape& getSprite() const;
    [[nodiscard]] sf::RectangleShape& getSprite();

    /**
     * @brief SFML Drawable interface implementation.
     */
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const override;

private:
    void updateSprite();

    sf::RectangleShape sprite;
    float t = 0.0f;

    // Kinematics and lookahead parameters
    float currentSpeed = 5.0f;        ///< Current physical speed (px/frame)
    float desiredSpeed = 5.0f;        ///< Target cruising speed on straight paths (px/frame)
    float lookaheadDistance = 80.0f;  ///< Physical lookahead distance L (px)
    float acceleration = 0.08f;       ///< Fixed acceleration rate per frame (px/frame^2)
    float deceleration = 0.18f;       ///< Fixed deceleration (braking) rate per frame (px/frame^2)
    float frictionCoeff = 0.1f;       ///< Lateral friction coefficient mu*g for cornering
    float minSpeed = 0.5f;            ///< Minimum speed floor during sharp turns

    sf::Vector2f position = {0.0f, 0.0f};
    sf::Color color = sf::Color(255, 50, 50);
    float angle = 0.0f;
    sf::Vector2f size = {32.0f, 16.0f};
};