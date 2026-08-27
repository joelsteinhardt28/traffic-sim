#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <optional>

#include "NetworkManager.hpp"

/**
 * @brief Interactive visual editor and inspector for creating and modifying road networks.
 *
 * Allows users to place intersection and gateway nodes, convert gateways to intersections,
 * draw straight and curved two-way roads, select roads to inspect and drag intermediate
 * Bézier control points (P1, P2) in real time, view turning lanes, and switch tools via GUI buttons or hotkeys.
 */
class RoadViewer {
public:
    enum class ToolMode {
        SelectMove,       ///< Select and drag nodes or road control points in real time
        StraightRoad,     ///< Click Start Node -> Click End Node to build straight two-way road
        CurvedRoad,       ///< Click Start -> Handle 1 -> Handle 2 -> End Node to build curved two-way road
        AddIntersection,  ///< Click on canvas to spawn an Intersection (or click a Gateway to convert it)
        AddGateway        ///< Click on canvas to spawn a Gateway
    };

    struct GUIButton {
        ToolMode mode;
        std::string label;
        std::string shortcut;
        sf::FloatRect bounds;
    };

    RoadViewer(bool fullscreen = true,
               const std::string& title = "Traffic Network Visualizer & Road Builder",
               unsigned int windowedWidth = 1280, unsigned int windowedHeight = 720);
    RoadViewer(unsigned int width, unsigned int height,
               const std::string& title = "Traffic Network Visualizer & Road Builder",
               bool fullscreen = true);
    ~RoadViewer() = default;

    void run();

    void setToolMode(ToolMode mode);
    [[nodiscard]] ToolMode getToolMode() const { return currentTool; }

    NetworkManager& getNetworkManager() { return network; }
    const NetworkManager& getNetworkManager() const { return network; }

    void convertSelectedNode();
    void toggleFullscreen();

private:
    void initWindow(bool fullscreen);
    void processEvents();
    void update(float dt);
    void render();
    void tryLoadFont();
    void initGUIButtons();
    void updateButtonLayout(float topY);

    // Event handling
    void handleMousePress(sf::Mouse::Button button, const sf::Vector2f& mousePos);
    void handleMouseMove(const sf::Vector2f& mousePos);
    void handleMouseRelease(sf::Mouse::Button button);
    void handleKeyPress(sf::Keyboard::Key key);
    bool handleButtonClick(const sf::Vector2f& mousePos);

    // Tool actions
    void handleSelectMoveClick(const sf::Vector2f& mousePos);
    void handleStraightRoadClick(const sf::Vector2f& mousePos);
    void handleCurvedRoadClick(const sf::Vector2f& mousePos);
    void cancelCurrentAction();

    // Rendering subsystems
    void renderRoadSurfaces();
    void renderLanes();
    void renderTurnLanes();
    void renderNodeBases();
    void renderNodeLabels();
    void renderSelectedRoadOverlay();
    void renderCreationPreview(const sf::Vector2f& mousePos);
    void renderLaneDirectionArrow(const sf::Vector2f& pos, const sf::Vector2f& dir, float size, sf::Color color);
    void renderGUIButtons();
    void drawHUD();

    // Application state
    sf::RenderWindow window;
    std::string windowTitle;
    unsigned int savedWidth = 1280;
    unsigned int savedHeight = 720;
    bool isFullscreen = true;

    sf::Font font;
    bool fontLoaded = false;
    bool showHUD = true;
    bool showTurnLanes = true;
    bool showRoadSurfaces = true;

    NetworkManager network;
    ToolMode currentTool = ToolMode::SelectMove;

    // GUI Buttons
    std::vector<GUIButton> guiButtons;
    int hoveredButtonIndex = -1;

    // Selection & Manipulation
    int selectedNodeId = -1;
    int hoveredNodeId = -1;
    int draggedNodeId = -1;

    int selectedRoadId = -1;
    int hoveredHandleIndex = 0; // 0: None, 1: Control Point 1, 2: Control Point 2
    int draggedHandleIndex = 0; // 0: None, 1: Control Point 1, 2: Control Point 2

    // Road Creation Wizard State
    size_t roadCreationStartNodeId = 0;
    int curvedRoadStep = 0; // 0: Start, 1: Handle1, 2: Handle2, 3: End
    sf::Vector2f curveHandle1 = {0.0f, 0.0f};
    sf::Vector2f curveHandle2 = {0.0f, 0.0f};
    sf::Vector2f currentMousePos = {0.0f, 0.0f};
};
