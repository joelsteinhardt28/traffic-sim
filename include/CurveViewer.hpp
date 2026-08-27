#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

#include "Agent.hpp"
#include "curves.hpp"

/**
 * @brief Represents the active operational mode of the CurveViewer.
 */
enum class ViewerMode {
    CurveCreation,  ///< Interactive creation, modification, and dragging of Bézier spline control points
    Simulation      ///< Agent traffic simulation actively navigating along the constructed spline
};

/**
 * @brief Interactive viewer and simulator for Bézier curves, splines, and traffic agents.
 *
 * Provides a clear separation between:
 *  1. Curve Creation / Editing Subsystem: Placing, manipulating, and visualizing Bézier splines.
 *  2. Agent Simulation Subsystem: Autonomous agents traversing splines with kinematics and orientation.
 */
class CurveViewer {
public:
    CurveViewer(bool fullscreen = true,
                const std::string& title = "Interactive Bezier Curve & Traffic Agent Viewer",
                unsigned int windowedWidth = 1280, unsigned int windowedHeight = 720);
    CurveViewer(unsigned int width, unsigned int height,
                const std::string& title = "Interactive Bezier Curve & Traffic Agent Viewer",
                bool fullscreen = true);
    ~CurveViewer() = default;

    // Starts the main application loop
    void run();

    // Mode management
    [[nodiscard]] ViewerMode getMode() const { return currentMode; }
    void setMode(ViewerMode mode);
    void toggleMode();

    // ------------------------------------------------------------------------
    // Curve Creation & Manipulation Interface
    // ------------------------------------------------------------------------
    void addPoint(const sf::Vector2f& point);
    void removeLastPoint();
    void clearPoints();
    void loadSampleSpline();
    void rebuildSpline();
    [[nodiscard]] int findPointAt(const sf::Vector2f& pos, float radius = 14.0f) const;

    // ------------------------------------------------------------------------
    // Agent Simulation Interface
    // ------------------------------------------------------------------------
    void startSimulation();
    void pauseSimulation();
    void toggleSimulation();
    void resetSimulation();
    [[nodiscard]] bool isSimulationRunning() const { return simulationRunning; }
    [[nodiscard]] Agent& getAgent() { return agent; }
    [[nodiscard]] const Agent& getAgent() const { return agent; }

    // Window controls
    void toggleFullscreen();

private:
    // Window & Application Lifecycle
    void initWindow(bool fullscreen);
    void processEvents();
    void update(float dt);
    void render();
    void tryLoadFont();

    // Event Handling Subsystems
    void handleMousePress(sf::Mouse::Button button, const sf::Vector2f& mousePos);
    void handleMouseMove(const sf::Vector2f& mousePos);
    void handleMouseRelease(sf::Mouse::Button button);
    void handleKeyPress(sf::Keyboard::Key key);

    void handleCurveMousePress(sf::Mouse::Button button, const sf::Vector2f& mousePos);
    void handleCurveMouseMove(const sf::Vector2f& mousePos);
    void handleCurveMouseRelease(sf::Mouse::Button button);
    void handleCurveKeyPress(sf::Keyboard::Key key);

    void handleSimulationKeyPress(sf::Keyboard::Key key);

    // Simulation Subsystem
    void updateSimulation(float dt);

    // Rendering Subsystems
    void renderCurveElements();
    void renderSimulationElements();
    void drawControlPolygon();
    void drawControlPoints();
    void drawCurve();
    void drawTangents();
    void drawHUD();

    // Window properties
    sf::RenderWindow window;
    std::string windowTitle;
    unsigned int savedWidth = 1280;
    unsigned int savedHeight = 720;
    bool isFullscreen = true;

    // Active Mode
    ViewerMode currentMode = ViewerMode::CurveCreation;

    // --- Curve Subsystem State ---
    std::vector<sf::Vector2f> controlPoints;
    CubicBezierSpline spline;
    sf::VertexArray splineVertices;
    int draggedPointIndex = -1;
    int hoveredPointIndex = -1;
    bool hasSpline = false;
    bool showControlPolygon = true;
    bool showTangents = false;

    // --- Agent Simulation Subsystem State ---
    Agent agent;
    bool simulationRunning = false;

    // UI / Overlay State
    bool showHelp = true;
    sf::Font font;
    bool fontLoaded = false;
};
