#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <optional>

#include "NetworkManager.hpp"

namespace roadViewer {
    namespace colors {
        const sf::Color background(20, 22, 28);
        const sf::Color asphalt(34, 36, 44);

        // Nodes
        const sf::Color intersectionFill(32, 36, 46);
        const sf::Color intersectionOutline(80, 100, 130);
        const sf::Color intersectionOutlineHover(100, 220, 255);
        const sf::Color intersectionOutlineSelected(80, 255, 120);

        const sf::Color gatewayFill(55, 32, 36);
        const sf::Color gatewayOutline(180, 80, 90);
        const sf::Color gatewayOutlineHover(255, 180, 100);
        const sf::Color gatewayOutlineSelected(80, 255, 120);

        // Handles & Control Polygon
        const sf::Color controlPolygonLine(255, 190, 40, 160);
        const sf::Color handleFillNormal(255, 200, 50);
        const sf::Color handleFillHover(80, 255, 140);
        const sf::Color handleFillDragged(255, 80, 80);
        const sf::Color handleOutline(255, 255, 255, 230);
        const sf::Color handleLabel(255, 235, 180);

        // Creation Previews
        const sf::Color previewStraightLine(80, 255, 140, 180);
        const sf::Color previewStraightDot(80, 255, 140, 220);
        const sf::Color previewCurvedLine(255, 200, 80, 180);
        const sf::Color previewCurvedSpline(255, 200, 80, 200);
        const sf::Color previewHandleDot(255, 200, 80);

        // HUD & GUI
        const sf::Color hudBg(14, 15, 20, 230);
        const sf::Color hudOutline(60, 140, 220);
        const sf::Color hudTextHeader(230, 230, 240);
        const sf::Color hudTextSelected(80, 255, 120); // Lime Green
        const sf::Color hudTextFooter(170, 180, 200);

        // Toolbar Buttons
        const sf::Color btnBgNormal(24, 27, 36, 225);
        const sf::Color btnOutlineNormal(65, 75, 95);
        const sf::Color btnTextNormal(190, 195, 210);

        const sf::Color btnBgHover(48, 56, 72, 235);
        const sf::Color btnOutlineHover(140, 180, 230);
        const sf::Color btnTextHover(240, 245, 255);

        const sf::Color btnBgActive(32, 85, 140, 245);
        const sf::Color btnOutlineActive(80, 220, 255);
        const sf::Color btnTextActive(255, 255, 255);
    }

    namespace dimensions {
        // Road rendering
        constexpr float asphaltWidthMultiplier = 1.35f;
        constexpr float asphaltOneWayMultiplier = 0.95f;
        constexpr float laneArrowSize = 7.0f;
        constexpr float turnArrowSize = 5.5f;
        constexpr float arrowWingRatio = 0.7f;
        constexpr float arrowSpreadRatio = 0.6f;

        // Node outlines & click tolerances
        constexpr float nodeOutlineNormal = 1.5f;
        constexpr float nodeOutlineHighlight = 2.5f;
        constexpr float nodeClickExtraRadius = 8.0f;
        constexpr float nodeRoadSnapRadius = 10.0f;
        constexpr float roadClickTolerance = 18.0f;
        constexpr float handleClickRadius = 14.0f;

        // Handles
        constexpr float handleRadiusNormal = 6.5f;
        constexpr float handleRadiusHover = 8.0f;
        constexpr float handleRadiusDragged = 9.0f;
        constexpr float handleOutlineThickness = 2.0f;

        // Creation Dots
        constexpr float previewTargetDotRadius = 6.0f;
        constexpr float previewHandleDotRadius = 5.0f;

        // GUI & HUD
        constexpr float hudPaddingX = 14.0f;
        constexpr float hudPaddingY = 14.0f;
        constexpr float hudBorderRadius = 1.5f;
        constexpr unsigned int hudFontSize = 13;
        constexpr unsigned int nodeLabelFontSize = 11;
        constexpr unsigned int handleLabelFontSize = 11;
        constexpr unsigned int btnFontSize = 11;

        constexpr float btnWidth = 120.0f;
        constexpr float btnHeight = 30.0f;
        constexpr float btnGap = 6.0f;
        constexpr float btnOutlineThicknessNormal = 1.0f;
        constexpr float btnOutlineThicknessHover = 1.5f;
        constexpr float btnOutlineThicknessActive = 2.0f;

        constexpr unsigned int defaultWindowWidth = 1280;
        constexpr unsigned int defaultWindowHeight = 720;
        constexpr unsigned int frameRateLimit = 60;
        constexpr unsigned int antialiasingLevel = 8;

        constexpr int roadRenderSamples = 30;
        constexpr int turnRenderSamples = 24;
    }
}

/**
 * @brief Interactive visual editor and inspector for creating and modifying road networks.
 *
 * Supports two-way and one-way roads, straight and curved geometries, intersection turn generation,
 * interactive handle manipulation, node conversion, and dynamic toolbar controls.
 */
class RoadViewer {
public:
    enum class ToolMode {
        SelectMove,          ///< Select and drag nodes or road control points in real time
        StraightRoad,        ///< Click Start -> End Node to build straight two-way road
        CurvedRoad,          ///< Click Start -> Handle 1 -> Handle 2 -> End Node to build curved two-way road
        StraightOneWayRoad,  ///< Click Start -> End Node to build straight one-way road
        CurvedOneWayRoad,    ///< Click Start -> Handle 1 -> Handle 2 -> End Node to build curved one-way road
        AddIntersection,     ///< Click on canvas to spawn an Intersection (or click a Gateway to convert it)
        AddGateway           ///< Click on canvas to spawn a Gateway
    };

    struct GUIButton {
        ToolMode mode;
        std::string label;
        std::string shortcut;
        sf::FloatRect bounds;
    };

    RoadViewer(bool fullscreen = true,
               const std::string& title = "Traffic Network Visualizer & Road Builder",
               unsigned int windowedWidth = roadViewer::dimensions::defaultWindowWidth,
               unsigned int windowedHeight = roadViewer::dimensions::defaultWindowHeight);
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
    void handleStraightRoadClick(const sf::Vector2f& mousePos, bool isOneWay);
    void handleCurvedRoadClick(const sf::Vector2f& mousePos, bool isOneWay);
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
    unsigned int savedWidth = roadViewer::dimensions::defaultWindowWidth;
    unsigned int savedHeight = roadViewer::dimensions::defaultWindowHeight;
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
