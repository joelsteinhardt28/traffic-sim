#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

#include "curves.hpp"

/**
 * @brief Interactive viewer for creating and manipulating Bézier curves and splines.
 *
 * Allows users to place control points with mouse clicks.
 * - 4 points define the initial cubic Bézier curve.
 * - Every 3 additional points append a continuous cubic Bézier segment, forming a spline.
 * - Existing control points can be clicked and dragged in real-time.
 */
class CurveViewer {
public:
    CurveViewer(bool fullscreen = true,
                const std::string& title = "Interactive Bezier Curve & Spline Viewer",
                unsigned int windowedWidth = 1280, unsigned int windowedHeight = 720);
    CurveViewer(unsigned int width, unsigned int height,
                const std::string& title = "Interactive Bezier Curve & Spline Viewer",
                bool fullscreen = true);
    ~CurveViewer() = default;

    // Starts the main application loop
    void run();

    // Point manipulation
    void addPoint(const sf::Vector2f& point);
    void removeLastPoint();
    void clearPoints();
    void loadSampleSpline();
    void toggleFullscreen();

private:
    void initWindow(bool fullscreen);
    void processEvents();
    void handleMousePress(sf::Mouse::Button button, const sf::Vector2f& mousePos);
    void handleMouseMove(const sf::Vector2f& mousePos);
    void handleMouseRelease(sf::Mouse::Button button);
    void handleKeyPress(sf::Keyboard::Key key);

    void rebuildSpline();
    int findPointAt(const sf::Vector2f& pos, float radius = 14.0f) const;

    void render();
    void drawControlPolygon();
    void drawControlPoints();
    void drawCurve();
    void drawTangents();
    void drawHUD();

    void tryLoadFont();

    void drawAgent();

    sf::RenderWindow window;
    std::string windowTitle;
    unsigned int savedWidth = 1280;
    unsigned int savedHeight = 720;
    bool isFullscreen = true;

    std::vector<sf::Vector2f> controlPoints;
    CubicBezierSpline spline;
    sf::VertexArray splineVertices;

    int draggedPointIndex = -1;
    int hoveredPointIndex = -1;
    bool hasSpline = false;
    bool showControlPolygon = true;
    bool showTangents = false;
    bool showHelp = true;

    sf::Font font;
    bool fontLoaded = false;

    // Agent simulation
    float agentT = 0.0f;
    float agentSpeed = 0.002f;  // Increment of t per frame
    bool playSimulation = false;
};
