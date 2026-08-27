#pragma once

#include <SFML/Graphics.hpp>
#include "curves.hpp"

/**
 * @brief Represents an autonomous vehicle / traffic agent navigating a Bézier spline.
 *
 * Encapsulates position along the spline (t parameter), velocity/speed,
 * physical orientation (heading angle), visual representation, and update logic.
 */
class Agent : public sf::Drawable {
public:
    /**
     * @brief Constructs a new Agent with specified speed, size, and color.
     * @param speed Speed increment of parameter t per simulation step (default 0.002f).
     * @param size Dimensions of the agent rectangle (default 32x16).
     * @param color Fill color of the agent (default red).
     */
    explicit Agent(float speed = 0.002f,
                   const sf::Vector2f& size = sf::Vector2f(32.0f, 16.0f),
                   const sf::Color& color = sf::Color(255, 50, 50));

    /**
     * @brief Constructs a new Agent with an initial progress parameter t.
     * @param initialT Initial t parameter along the spline [0.0, 1.0].
     * @param speed Speed increment of parameter t per simulation step.
     * @param size Dimensions of the agent rectangle.
     * @param color Fill color of the agent.
     */
    Agent(float initialT, float speed,
          const sf::Vector2f& size = sf::Vector2f(32.0f, 16.0f),
          const sf::Color& color = sf::Color(255, 50, 50));

    /**
     * @brief Updates the agent's position and orientation along the given spline.
     * Advances parameter t by speed, evaluates position and tangent, and updates the sprite.
     * @param spline The Bézier spline the agent is traversing.
     */
    void update(const CubicBezierSpline& spline);

    /**
     * @brief Resets the agent's progress t back to 0.0f and clears orientation/position.
     */
    void reset();

    // --- Accessors and Mutators ---

    [[nodiscard]] float getT() const;
    void setT(float t);

    [[nodiscard]] float getSpeed() const;
    void setSpeed(float speed);

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
    float speed = 0.002f;
    sf::Vector2f position = {0.0f, 0.0f};
    sf::Color color = sf::Color(255, 50, 50);
    float angle = 0.0f;
    sf::Vector2f size = {32.0f, 16.0f};
};