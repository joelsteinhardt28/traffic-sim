#include "RoadViewer.hpp"
#include "constants.hpp"
#include "toolbox.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

RoadViewer::RoadViewer(bool fullscreen, const std::string& title, unsigned int windowedWidth, unsigned int windowedHeight)
    : windowTitle(title), savedWidth(windowedWidth), savedHeight(windowedHeight), isFullscreen(fullscreen) {
    initWindow(isFullscreen);
    tryLoadFont();
    initGUIButtons();

    network.loadSampleNetwork(window.getSize().x, window.getSize().y);

    print::info("RoadViewer Network Editor initialized.");
    print::info("Tools: [1] Select/Move | [2] Straight Road | [3] Curved Road | [4] Intersection | [5] Gateway");
    print::info("Interactive GUI toolbar enabled in the top-left corner.");
}

RoadViewer::RoadViewer(unsigned int width, unsigned int height, const std::string& title, bool fullscreen)
    : windowTitle(title), savedWidth(width), savedHeight(height), isFullscreen(fullscreen) {
    initWindow(isFullscreen);
    tryLoadFont();
    initGUIButtons();
    network.loadSampleNetwork(window.getSize().x, window.getSize().y);
}

void RoadViewer::initWindow(bool fullscreen) {
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    isFullscreen = fullscreen;

    if (isFullscreen) {
        window.create(sf::VideoMode::getDesktopMode(), windowTitle, sf::Style::Fullscreen, settings);
    } else {
        window.create(sf::VideoMode(savedWidth, savedHeight), windowTitle, sf::Style::Default, settings);
    }
    window.setFramerateLimit(60);
}

void RoadViewer::toggleFullscreen() {
    initWindow(!isFullscreen);
}

void RoadViewer::tryLoadFont() {
    const std::vector<std::string> fontPaths = {
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
    };

    for (const auto& path : fontPaths) {
        if (font.loadFromFile(path)) {
            fontLoaded = true;
            return;
        }
    }
    fontLoaded = false;
}

void RoadViewer::initGUIButtons() {
    guiButtons.clear();
    guiButtons.push_back({ToolMode::SelectMove, "Select / Move", "[1/V]", sf::FloatRect(0, 0, 142.f, 32.f)});
    guiButtons.push_back({ToolMode::StraightRoad, "Straight Road", "[2/R]", sf::FloatRect(0, 0, 142.f, 32.f)});
    guiButtons.push_back({ToolMode::CurvedRoad, "Curved Road", "[3/C]", sf::FloatRect(0, 0, 142.f, 32.f)});
    guiButtons.push_back({ToolMode::AddIntersection, "+ Intersection", "[4/I]", sf::FloatRect(0, 0, 142.f, 32.f)});
    guiButtons.push_back({ToolMode::AddGateway, "+ Gateway", "[5/G]", sf::FloatRect(0, 0, 142.f, 32.f)});
}

void RoadViewer::updateButtonLayout(float topY) {
    float startX = 14.0f;
    float gap = 8.0f;
    float btnWidth = 142.0f;
    float btnHeight = 32.0f;

    for (size_t i = 0; i < guiButtons.size(); ++i) {
        guiButtons[i].bounds = sf::FloatRect(startX + i * (btnWidth + gap), topY, btnWidth, btnHeight);
    }
}

bool RoadViewer::handleButtonClick(const sf::Vector2f& mousePos) {
    if (!showHUD) return false;

    for (size_t i = 0; i < guiButtons.size(); ++i) {
        if (guiButtons[i].bounds.contains(mousePos)) {
            setToolMode(guiButtons[i].mode);
            return true;
        }
    }
    return false;
}

void RoadViewer::setToolMode(ToolMode mode) {
    currentTool = mode;
    cancelCurrentAction();

    switch (currentTool) {
        case ToolMode::SelectMove:
            print::info("Active Tool: [Select & Move] - Click/drag nodes, or select curved roads to adjust control handles.");
            break;
        case ToolMode::StraightRoad:
            print::info("Active Tool: [Straight Two-Way Road] - Click start node, then click end node.");
            break;
        case ToolMode::CurvedRoad:
            print::info("Active Tool: [Curved Two-Way Road] - Step 1: Click start node.");
            break;
        case ToolMode::AddIntersection:
            print::info("Active Tool: [Add Intersection Node] - Click empty canvas to create, or click a Gateway to convert it.");
            break;
        case ToolMode::AddGateway:
            print::info("Active Tool: [Add Gateway Node] - Click empty canvas to place a gateway.");
            break;
    }
}

void RoadViewer::cancelCurrentAction() {
    roadCreationStartNodeId = 0;
    curvedRoadStep = 0;
    draggedNodeId = -1;
    draggedHandleIndex = 0;
}

void RoadViewer::convertSelectedNode() {
    if (selectedNodeId == -1) return;

    auto* node = network.getNode(static_cast<size_t>(selectedNodeId));
    if (!node) return;

    if (node->isGateway()) {
        network.convertGatewayToIntersection(static_cast<size_t>(selectedNodeId));
    } else if (node->isIntersection()) {
        network.convertIntersectionToGateway(static_cast<size_t>(selectedNodeId));
    }
}

void RoadViewer::run() {
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
}

void RoadViewer::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        } else if (event.type == sf::Event::MouseButtonPressed) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
            handleMousePress(event.mouseButton.button, mousePos);
        } else if (event.type == sf::Event::MouseMoved) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
            handleMouseMove(mousePos);
        } else if (event.type == sf::Event::MouseButtonReleased) {
            handleMouseRelease(event.mouseButton.button);
        } else if (event.type == sf::Event::KeyPressed) {
            handleKeyPress(event.key.code);
        }
    }
}

void RoadViewer::handleMousePress(sf::Mouse::Button button, const sf::Vector2f& mousePos) {
    if (button == sf::Mouse::Right) {
        cancelCurrentAction();
        return;
    }

    if (button != sf::Mouse::Left) return;

    // 1. Check if clicked on a GUI Toolbar Button
    if (handleButtonClick(mousePos)) {
        return;
    }

    // 2. Delegate to active tool action
    switch (currentTool) {
        case ToolMode::SelectMove:
            handleSelectMoveClick(mousePos);
            break;
        case ToolMode::StraightRoad:
            handleStraightRoadClick(mousePos);
            break;
        case ToolMode::CurvedRoad:
            handleCurvedRoadClick(mousePos);
            break;
        case ToolMode::AddIntersection: {
            int existingNode = network.findNodeAt(mousePos, 8.0f);
            if (existingNode != -1) {
                auto* n = network.getNode(static_cast<size_t>(existingNode));
                if (n && n->isGateway()) {
                    network.convertGatewayToIntersection(static_cast<size_t>(existingNode));
                }
                selectedNodeId = existingNode;
            } else {
                size_t id = network.createIntersection(mousePos, 26.0f);
                selectedNodeId = static_cast<int>(id);
            }
            break;
        }
        case ToolMode::AddGateway: {
            int existingNode = network.findNodeAt(mousePos, 8.0f);
            if (existingNode != -1) {
                auto* n = network.getNode(static_cast<size_t>(existingNode));
                if (n && n->isIntersection()) {
                    network.convertIntersectionToGateway(static_cast<size_t>(existingNode));
                }
                selectedNodeId = existingNode;
            } else {
                size_t id = network.createGateway(mousePos, 20.0f);
                selectedNodeId = static_cast<int>(id);
            }
            break;
        }
    }
}

void RoadViewer::handleMouseMove(const sf::Vector2f& mousePos) {
    currentMousePos = mousePos;

    // Update hovered GUI button
    hoveredButtonIndex = -1;
    if (showHUD) {
        for (size_t i = 0; i < guiButtons.size(); ++i) {
            if (guiButtons[i].bounds.contains(mousePos)) {
                hoveredButtonIndex = static_cast<int>(i);
                break;
            }
        }
    }

    hoveredNodeId = network.findNodeAt(mousePos, 8.0f);

    if (selectedRoadId != -1) {
        hoveredHandleIndex = network.findRoadControlPointAt(static_cast<size_t>(selectedRoadId), mousePos, 14.0f);
    } else {
        hoveredHandleIndex = 0;
    }

    if (draggedHandleIndex != 0 && selectedRoadId != -1) {
        network.moveRoadControlPoint(static_cast<size_t>(selectedRoadId), draggedHandleIndex, mousePos);
    } else if (currentTool == ToolMode::SelectMove && draggedNodeId != -1) {
        network.moveNode(static_cast<size_t>(draggedNodeId), mousePos);
    }
}

void RoadViewer::handleMouseRelease(sf::Mouse::Button button) {
    if (button == sf::Mouse::Left) {
        draggedNodeId = -1;
        draggedHandleIndex = 0;
    }
}

void RoadViewer::handleSelectMoveClick(const sf::Vector2f& mousePos) {
    // 1. Check if clicked on a curved road control point
    if (selectedRoadId != -1) {
        int clickedHandle = network.findRoadControlPointAt(static_cast<size_t>(selectedRoadId), mousePos, 14.0f);
        if (clickedHandle != 0) {
            draggedHandleIndex = clickedHandle;
            draggedNodeId = -1;
            return;
        }
    }

    // 2. Check if clicked on a node
    int clickedNode = network.findNodeAt(mousePos, 8.0f);
    if (clickedNode != -1) {
        selectedNodeId = clickedNode;
        draggedNodeId = clickedNode;
        selectedRoadId = -1;
        draggedHandleIndex = 0;
        return;
    }

    // 3. Check if clicked on a road
    int clickedRoad = network.findRoadAt(mousePos, 18.0f);
    if (clickedRoad != -1) {
        selectedRoadId = clickedRoad;
        selectedNodeId = -1;
        draggedNodeId = -1;
        draggedHandleIndex = 0;
        print::info("Selected Road R" + std::to_string(clickedRoad));
        return;
    }

    // 4. Clicked empty space
    selectedNodeId = -1;
    selectedRoadId = -1;
    draggedNodeId = -1;
    draggedHandleIndex = 0;
}

void RoadViewer::handleStraightRoadClick(const sf::Vector2f& mousePos) {
    int clickedNode = network.findNodeAt(mousePos, 10.0f);
    size_t nodeId = 0;

    if (clickedNode != -1) {
        nodeId = static_cast<size_t>(clickedNode);
    } else {
        nodeId = network.createIntersection(mousePos, 26.0f);
    }

    if (roadCreationStartNodeId == 0) {
        roadCreationStartNodeId = nodeId;
        selectedNodeId = static_cast<int>(nodeId);
        print::info("Road Start Node set to " + std::to_string(nodeId) + ". Now click target node (or empty space).");
    } else {
        if (roadCreationStartNodeId != nodeId) {
            size_t roadId = network.createStraightTwoWayRoad(roadCreationStartNodeId, nodeId, 5.0f, 14.0f);
            selectedRoadId = static_cast<int>(roadId);
        }
        roadCreationStartNodeId = 0;
    }
}

void RoadViewer::handleCurvedRoadClick(const sf::Vector2f& mousePos) {
    if (curvedRoadStep == 0) {
        int clickedNode = network.findNodeAt(mousePos, 10.0f);
        if (clickedNode != -1) {
            roadCreationStartNodeId = static_cast<size_t>(clickedNode);
        } else {
            roadCreationStartNodeId = network.createIntersection(mousePos, 26.0f);
        }
        curvedRoadStep = 1;
        print::info("Curved Road - Step 2/4: Click to place Curve Control Handle 1.");
    } else if (curvedRoadStep == 1) {
        curveHandle1 = mousePos;
        curvedRoadStep = 2;
        print::info("Curved Road - Step 3/4: Click to place Curve Control Handle 2.");
    } else if (curvedRoadStep == 2) {
        curveHandle2 = mousePos;
        curvedRoadStep = 3;
        print::info("Curved Road - Step 4/4: Click to select or create End Node.");
    } else if (curvedRoadStep == 3) {
        int clickedNode = network.findNodeAt(mousePos, 10.0f);
        size_t endNodeId = 0;
        if (clickedNode != -1) {
            endNodeId = static_cast<size_t>(clickedNode);
        } else {
            endNodeId = network.createIntersection(mousePos, 26.0f);
        }

        if (roadCreationStartNodeId != endNodeId) {
            size_t roadId = network.createCurvedTwoWayRoad(
                roadCreationStartNodeId, endNodeId,
                curveHandle1, curveHandle2,
                5.0f, 14.0f
            );
            selectedRoadId = static_cast<int>(roadId);
        }
        cancelCurrentAction();
    }
}

void RoadViewer::handleKeyPress(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Escape) {
        if (roadCreationStartNodeId != 0 || curvedRoadStep != 0) {
            cancelCurrentAction();
            print::info("Cancelled road creation.");
        } else if (selectedRoadId != -1 || selectedNodeId != -1) {
            selectedRoadId = -1;
            selectedNodeId = -1;
            draggedHandleIndex = 0;
        } else {
            window.close();
        }
    } else if (key == sf::Keyboard::Num1 || key == sf::Keyboard::V || key == sf::Keyboard::S) {
        setToolMode(ToolMode::SelectMove);
    } else if (key == sf::Keyboard::Num2 || key == sf::Keyboard::R) {
        setToolMode(ToolMode::StraightRoad);
    } else if (key == sf::Keyboard::Num3 || key == sf::Keyboard::C) {
        setToolMode(ToolMode::CurvedRoad);
    } else if (key == sf::Keyboard::Num4) {
        setToolMode(ToolMode::AddIntersection);
    } else if (key == sf::Keyboard::Num5) {
        setToolMode(ToolMode::AddGateway);
    } else if (key == sf::Keyboard::I || key == sf::Keyboard::Return) {
        if (selectedNodeId != -1) {
            auto* node = network.getNode(static_cast<size_t>(selectedNodeId));
            if (node && node->isGateway()) {
                network.convertGatewayToIntersection(static_cast<size_t>(selectedNodeId));
            } else if (key == sf::Keyboard::Return && node && node->isIntersection()) {
                network.convertIntersectionToGateway(static_cast<size_t>(selectedNodeId));
            }
        } else {
            setToolMode(ToolMode::AddIntersection);
        }
    } else if (key == sf::Keyboard::G) {
        if (selectedNodeId != -1) {
            auto* node = network.getNode(static_cast<size_t>(selectedNodeId));
            if (node && node->isIntersection()) {
                network.convertIntersectionToGateway(static_cast<size_t>(selectedNodeId));
            }
        } else {
            setToolMode(ToolMode::AddGateway);
        }
    } else if (key == sf::Keyboard::D) {
        network.loadSampleNetwork(window.getSize().x, window.getSize().y);
        selectedRoadId = -1;
        selectedNodeId = -1;
    } else if (key == sf::Keyboard::T) {
        showTurnLanes = !showTurnLanes;
        print::info(std::string("Intersection Turn Lanes: ") + (showTurnLanes ? "Visible" : "Hidden"));
    } else if (key == sf::Keyboard::L) {
        showRoadSurfaces = !showRoadSurfaces;
        print::info(std::string("Road Asphalt Surfaces: ") + (showRoadSurfaces ? "Visible" : "Hidden"));
    } else if (key == sf::Keyboard::H || key == sf::Keyboard::F1) {
        showHUD = !showHUD;
    } else if (key == sf::Keyboard::F11 || key == sf::Keyboard::F) {
        toggleFullscreen();
    } else if (key == sf::Keyboard::Delete || key == sf::Keyboard::X) {
        if (selectedNodeId != -1) {
            network.removeNode(static_cast<size_t>(selectedNodeId));
            selectedNodeId = -1;
        } else if (selectedRoadId != -1) {
            network.removeRoad(static_cast<size_t>(selectedRoadId));
            selectedRoadId = -1;
            draggedHandleIndex = 0;
        }
    } else if (key == sf::Keyboard::Z || key == sf::Keyboard::BackSpace) {
        cancelCurrentAction();
    }
}

void RoadViewer::update(float /* dt */) {
}

void RoadViewer::render() {
    window.clear(sf::Color(20, 22, 28));

    // 1. Render dark asphalt road corridor base
    if (showRoadSurfaces) {
        renderRoadSurfaces();
    }

    // 2. Render normal road driving lanes
    renderLanes();

    // 3. Render node junction background circles
    renderNodeBases();

    // 4. Render intersection turn lanes ON TOP of node junction bases
    if (showTurnLanes) {
        renderTurnLanes();
    }

    // 5. Render node ID tags and selection outlines
    renderNodeLabels();

    // 6. Render selected road control polygon & handles (P1, P2)
    renderSelectedRoadOverlay();

    // 7. Render live road creation wizard preview
    renderCreationPreview(currentMousePos);

    // 8. Render HUD overlay and GUI Buttons
    if (showHUD) {
        drawHUD();
    }

    window.display();
}

void RoadViewer::renderRoadSurfaces() {
    for (const auto& [id, road] : network.getRoads()) {
        const auto* segFwd = network.getSegment(road.forwardSegmentId);
        const auto* segBwd = network.getSegment(road.backwardSegmentId);
        if (!segFwd || !segBwd || segFwd->spline.empty()) continue;

        sf::VertexArray roadStrip(sf::TriangleStrip);
        const int samples = 30;
        const float halfTotalWidth = road.laneWidth * 1.35f;

        for (int i = 0; i <= samples; ++i) {
            float t = static_cast<float>(i) / samples;
            sf::Vector2f posFwd = segFwd->spline.eval(t);
            sf::Vector2f posBwd = segBwd->spline.eval(1.0f - t);
            sf::Vector2f center = (posFwd + posBwd) * 0.5f;

            sf::Vector2f tan = segFwd->spline.evalTangent(t);
            float len = std::hypot(tan.x, tan.y);
            sf::Vector2f n = len > 1e-4f ? sf::Vector2f(-tan.y / len, tan.x / len) : sf::Vector2f(0, 1);

            sf::Color asphaltColor(34, 36, 44);
            roadStrip.append(sf::Vertex(center + n * halfTotalWidth, asphaltColor));
            roadStrip.append(sf::Vertex(center - n * halfTotalWidth, asphaltColor));
        }
        window.draw(roadStrip);
    }
}

void RoadViewer::renderLanes() {
    for (const auto& [id, seg] : network.getSegments()) {
        if (seg.spline.empty()) continue;

        sf::VertexArray vertices = seg.spline.getVertices(30);
        for (size_t i = 0; i < vertices.getVertexCount(); ++i) {
            vertices[i].color = seg.laneColor;
        }
        window.draw(vertices);

        // Direction arrow
        sf::Vector2f midPos = seg.spline.eval(0.5f);
        sf::Vector2f midTan = seg.spline.evalTangent(0.5f);
        renderLaneDirectionArrow(midPos, midTan, 7.0f, seg.laneColor);
    }
}

void RoadViewer::renderTurnLanes() {
    for (const auto& turn : network.getTurnLanes()) {
        if (turn.spline.empty()) continue;

        sf::VertexArray vertices = turn.spline.getVertices(24);
        for (size_t i = 0; i < vertices.getVertexCount(); ++i) {
            vertices[i].color = turn.laneColor;
        }
        window.draw(vertices);

        // Direction arrow for turn lane
        sf::Vector2f midPos = turn.spline.eval(0.5f);
        sf::Vector2f midTan = turn.spline.evalTangent(0.5f);
        renderLaneDirectionArrow(midPos, midTan, 5.5f, turn.laneColor);
    }
}

void RoadViewer::renderLaneDirectionArrow(const sf::Vector2f& pos, const sf::Vector2f& dir, float size, sf::Color color) {
    float len = std::hypot(dir.x, dir.y);
    if (len < 1e-4f) return;

    sf::Vector2f u = dir / len;
    sf::Vector2f n(-u.y, u.x);

    sf::ConvexShape arrow;
    arrow.setPointCount(3);
    arrow.setPoint(0, pos + u * size);
    arrow.setPoint(1, pos - u * (size * 0.7f) + n * (size * 0.6f));
    arrow.setPoint(2, pos - u * (size * 0.7f) - n * (size * 0.6f));
    arrow.setFillColor(color);

    window.draw(arrow);
}

void RoadViewer::renderNodeBases() {
    for (const auto& [id, node] : network.getNodes()) {
        bool isHovered = (static_cast<int>(id) == hoveredNodeId);
        bool isSelected = (static_cast<int>(id) == selectedNodeId);

        sf::CircleShape circle(node->getRadius());
        circle.setOrigin(node->getRadius(), node->getRadius());
        circle.setPosition(node->getPosition());

        if (node->isIntersection()) {
            circle.setFillColor(sf::Color(32, 36, 46));
            circle.setOutlineColor(isSelected ? sf::Color(80, 255, 120) :
                                  (isHovered ? sf::Color(100, 220, 255) : sf::Color(80, 100, 130)));
            circle.setOutlineThickness(isSelected || isHovered ? 2.5f : 1.5f);
        } else {
            circle.setFillColor(sf::Color(55, 32, 36));
            circle.setOutlineColor(isSelected ? sf::Color(80, 255, 120) :
                                  (isHovered ? sf::Color(255, 180, 100) : sf::Color(180, 80, 90)));
            circle.setOutlineThickness(isSelected || isHovered ? 2.5f : 1.5f);
        }

        window.draw(circle);
    }
}

void RoadViewer::renderNodeLabels() {
    if (!fontLoaded) return;

    for (const auto& [id, node] : network.getNodes()) {
        sf::Text label;
        label.setFont(font);
        label.setCharacterSize(11);
        label.setFillColor(sf::Color(240, 240, 245));

        label.setString(node->getShortLabel());

        sf::FloatRect bounds = label.getLocalBounds();
        label.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
        label.setPosition(node->getPosition());
        window.draw(label);
    }
}

void RoadViewer::renderSelectedRoadOverlay() {
    if (selectedRoadId == -1) return;

    const auto* road = network.getRoad(static_cast<size_t>(selectedRoadId));
    if (!road) return;

    const auto* nodeA = network.getNode(road->nodeA);
    const auto* nodeB = network.getNode(road->nodeB);

    if (road->isCurved && nodeA && nodeB) {
        // 1. Draw Control Polygon connecting NodeA -> P1 -> P2 -> NodeB
        sf::VertexArray polygon(sf::LineStrip);
        polygon.append(sf::Vertex(nodeA->getPosition(), sf::Color(255, 190, 40, 150)));
        polygon.append(sf::Vertex(road->controlPoint1, sf::Color(255, 190, 40, 220)));
        polygon.append(sf::Vertex(road->controlPoint2, sf::Color(255, 190, 40, 220)));
        polygon.append(sf::Vertex(nodeB->getPosition(), sf::Color(255, 190, 40, 150)));
        window.draw(polygon);

        // 2. Draw Handle 1 (P1)
        float r1 = (draggedHandleIndex == 1) ? 9.0f : ((hoveredHandleIndex == 1) ? 8.0f : 6.5f);
        sf::Color c1 = (draggedHandleIndex == 1) ? sf::Color(255, 80, 80) :
                      ((hoveredHandleIndex == 1) ? sf::Color(80, 255, 140) : sf::Color(255, 200, 50));

        sf::CircleShape h1(r1);
        h1.setOrigin(r1, r1);
        h1.setPosition(road->controlPoint1);
        h1.setFillColor(c1);
        h1.setOutlineColor(sf::Color(255, 255, 255, 230));
        h1.setOutlineThickness(2.0f);
        window.draw(h1);

        // 3. Draw Handle 2 (P2)
        float r2 = (draggedHandleIndex == 2) ? 9.0f : ((hoveredHandleIndex == 2) ? 8.0f : 6.5f);
        sf::Color c2 = (draggedHandleIndex == 2) ? sf::Color(255, 80, 80) :
                      ((hoveredHandleIndex == 2) ? sf::Color(80, 255, 140) : sf::Color(255, 200, 50));

        sf::CircleShape h2(r2);
        h2.setOrigin(r2, r2);
        h2.setPosition(road->controlPoint2);
        h2.setFillColor(c2);
        h2.setOutlineColor(sf::Color(255, 255, 255, 230));
        h2.setOutlineThickness(2.0f);
        window.draw(h2);

        // 4. Draw labels for P1 and P2
        if (fontLoaded) {
            sf::Text t1, t2;
            t1.setFont(font); t2.setFont(font);
            t1.setCharacterSize(11); t2.setCharacterSize(11);
            t1.setFillColor(sf::Color(255, 235, 180));
            t2.setFillColor(sf::Color(255, 235, 180));

            t1.setString("P1");
            t1.setPosition(road->controlPoint1.x + 10.0f, road->controlPoint1.y - 14.0f);
            window.draw(t1);

            t2.setString("P2");
            t2.setPosition(road->controlPoint2.x + 10.0f, road->controlPoint2.y - 14.0f);
            window.draw(t2);
        }
    }
}

void RoadViewer::renderCreationPreview(const sf::Vector2f& mousePos) {
    if (currentTool == ToolMode::StraightRoad && roadCreationStartNodeId != 0) {
        const auto* startNode = network.getNode(roadCreationStartNodeId);
        if (startNode) {
            sf::VertexArray previewLine(sf::Lines);
            previewLine.append(sf::Vertex(startNode->getPosition(), sf::Color(80, 255, 140, 180)));
            previewLine.append(sf::Vertex(mousePos, sf::Color(80, 255, 140, 180)));
            window.draw(previewLine);

            sf::CircleShape targetDot(6.0f);
            targetDot.setOrigin(6.0f, 6.0f);
            targetDot.setPosition(mousePos);
            targetDot.setFillColor(sf::Color(80, 255, 140, 220));
            window.draw(targetDot);
        }
    } else if (currentTool == ToolMode::CurvedRoad) {
        const auto* startNode = network.getNode(roadCreationStartNodeId);
        if (curvedRoadStep == 1 && startNode) {
            sf::VertexArray l(sf::Lines);
            l.append(sf::Vertex(startNode->getPosition(), sf::Color(255, 200, 80, 180)));
            l.append(sf::Vertex(mousePos, sf::Color(255, 200, 80, 180)));
            window.draw(l);
        } else if (curvedRoadStep == 2 && startNode) {
            sf::VertexArray l(sf::Lines);
            l.append(sf::Vertex(curveHandle1, sf::Color(255, 200, 80, 180)));
            l.append(sf::Vertex(mousePos, sf::Color(255, 200, 80, 180)));
            window.draw(l);

            sf::CircleShape h1(5.0f);
            h1.setOrigin(5.0f, 5.0f);
            h1.setPosition(curveHandle1);
            h1.setFillColor(sf::Color(255, 200, 80));
            window.draw(h1);
        } else if (curvedRoadStep == 3 && startNode) {
            CubicBezierCurve previewCurve(startNode->getPosition(), curveHandle1, curveHandle2, mousePos);
            CubicBezierSpline previewSpline;
            previewSpline.addSegment(previewCurve);
            sf::VertexArray curveVerts = previewSpline.getVertices(30);
            for (size_t i = 0; i < curveVerts.getVertexCount(); ++i) {
                curveVerts[i].color = sf::Color(255, 200, 80, 200);
            }
            window.draw(curveVerts);

            sf::CircleShape h1(5.0f), h2(5.0f);
            h1.setOrigin(5.0f, 5.0f);
            h1.setPosition(curveHandle1);
            h1.setFillColor(sf::Color(255, 200, 80));
            h2.setOrigin(5.0f, 5.0f);
            h2.setPosition(curveHandle2);
            h2.setFillColor(sf::Color(255, 200, 80));
            window.draw(h1);
            window.draw(h2);
        }
    }
}

void RoadViewer::renderGUIButtons() {
    if (!fontLoaded) return;

    for (size_t i = 0; i < guiButtons.size(); ++i) {
        const auto& btn = guiButtons[i];
        bool isActive = (btn.mode == currentTool);
        bool isHovered = (static_cast<int>(i) == hoveredButtonIndex);

        sf::RectangleShape rect(sf::Vector2f(btn.bounds.width, btn.bounds.height));
        rect.setPosition(btn.bounds.left, btn.bounds.top);

        if (isActive) {
            rect.setFillColor(sf::Color(32, 85, 140, 245));
            rect.setOutlineColor(sf::Color(80, 220, 255));
            rect.setOutlineThickness(2.0f);
        } else if (isHovered) {
            rect.setFillColor(sf::Color(48, 56, 72, 235));
            rect.setOutlineColor(sf::Color(140, 180, 230));
            rect.setOutlineThickness(1.5f);
        } else {
            rect.setFillColor(sf::Color(24, 27, 36, 225));
            rect.setOutlineColor(sf::Color(65, 75, 95));
            rect.setOutlineThickness(1.0f);
        }

        window.draw(rect);

        // Draw Button Label
        sf::Text label;
        label.setFont(font);
        label.setCharacterSize(12);
        label.setFillColor(isActive ? sf::Color(255, 255, 255) : (isHovered ? sf::Color(240, 245, 255) : sf::Color(190, 195, 210)));
        label.setString(btn.label);

        sf::FloatRect textBounds = label.getLocalBounds();
        label.setOrigin(textBounds.left + textBounds.width * 0.5f, textBounds.top + textBounds.height * 0.5f);
        label.setPosition(btn.bounds.left + btn.bounds.width * 0.5f, btn.bounds.top + btn.bounds.height * 0.5f);

        window.draw(label);
    }
}

void RoadViewer::drawHUD() {
    if (!fontLoaded) return;

    std::ostringstream ssHeader;
    std::ostringstream ssSelected;
    std::ostringstream ssFooter;

    // 1. Tool Banner
    switch (currentTool) {
        case ToolMode::SelectMove:
            ssHeader << "[ TOOL: SELECT & MOVE ]\n";
            break;
        case ToolMode::StraightRoad:
            ssHeader << "[ TOOL: STRAIGHT TWO-WAY ROAD"
                     << (roadCreationStartNodeId != 0 ? " (Step 2/2: Select End Node)" : " (Step 1/2: Select Start Node)")
                     << " ]\n";
            break;
        case ToolMode::CurvedRoad:
            ssHeader << "[ TOOL: CURVED TWO-WAY ROAD (Step " << (curvedRoadStep + 1) << "/4) ]\n";
            break;
        case ToolMode::AddIntersection:
            ssHeader << "[ TOOL: ADD INTERSECTION (or click Gateway to convert) ]\n";
            break;
        case ToolMode::AddGateway:
            ssHeader << "[ TOOL: ADD GATEWAY (Dead-End) ]\n";
            break;
    }

    // 2. Network Telemetry
    size_t intersectionCount = 0;
    size_t gatewayCount = 0;
    for (const auto& [id, node] : network.getNodes()) {
        if (node->isIntersection()) intersectionCount++;
        else gatewayCount++;
    }

    ssHeader << "Network: " << network.getNodes().size() << " Nodes (" << intersectionCount << " Intersections, "
             << gatewayCount << " Gateways) | " << network.getRoads().size() << " Two-Way Roads | "
             << network.getSegments().size() << " Drive Lanes | "
             << (showTurnLanes ? std::to_string(network.getTurnLanes().size()) + " Turn Lanes [VISIBLE]" : "Turn Lanes [HIDDEN]");

    // 3. Selected Entity Info (Lime Green)
    bool hasSelection = false;
    if (selectedNodeId != -1) {
        const auto* n = network.getNode(static_cast<size_t>(selectedNodeId));
        if (n) {
            hasSelection = true;
            ssSelected << ">>> Selected Node " << n->getShortLabel() << " (" << n->getTypeName() << ") - "
                       << n->getConnectedRoadIds().size() << " Roads Connected, "
                       << n->getIncomingEdgeIds().size() << " Inbound, " << n->getOutgoingEdgeIds().size() << " Outbound";
            if (n->isGateway()) {
                ssSelected << " | [Press I / Enter to Convert to Intersection]";
            } else {
                ssSelected << " | [Press G to Convert to Gateway]";
            }
        }
    } else if (selectedRoadId != -1) {
        const auto* r = network.getRoad(static_cast<size_t>(selectedRoadId));
        if (r) {
            hasSelection = true;
            ssSelected << ">>> Selected " << (r->isCurved ? "Curved" : "Straight") << " Road R" << r->id
                       << " (Node " << r->nodeA << " <-> Node " << r->nodeB << ")";
            if (r->isCurved) {
                ssSelected << " | [Drag P1 / P2 to reshape curve] | [Del] Remove Road";
            } else {
                ssSelected << " | [Del] Remove Road";
            }
        }
    }

    // 4. Footer Actions
    ssFooter << "Actions: [I/Enter] Convert Gateway  [D] Demo  [T] Toggle Turns  [L] Surfaces  [Del] Delete  [H] HUD";

    // Setup Text Objects
    sf::Text textHeader, textSelected, textFooter;
    textHeader.setFont(font); textHeader.setCharacterSize(13); textHeader.setFillColor(sf::Color(230, 230, 240));
    textSelected.setFont(font); textSelected.setCharacterSize(13); textSelected.setFillColor(sf::Color(80, 255, 120)); // Lime Green!
    textFooter.setFont(font); textFooter.setCharacterSize(13); textFooter.setFillColor(sf::Color(170, 180, 200));

    textHeader.setString(ssHeader.str());
    if (hasSelection) textSelected.setString(ssSelected.str());
    textFooter.setString(ssFooter.str());

    float startX = 14.0f;
    float currentY = 14.0f;

    textHeader.setPosition(startX, currentY);
    currentY += textHeader.getGlobalBounds().height + 6.0f;

    if (hasSelection) {
        textSelected.setPosition(startX, currentY);
        currentY += textSelected.getGlobalBounds().height + 6.0f;
    }

    textFooter.setPosition(startX, currentY);
    currentY += textFooter.getGlobalBounds().height;

    // Background Panel
    float maxWidth = std::max({textHeader.getGlobalBounds().width,
                               hasSelection ? textSelected.getGlobalBounds().width : 0.0f,
                               textFooter.getGlobalBounds().width,
                               750.0f});

    float totalHeight = currentY - 14.0f;
    sf::RectangleShape bg(sf::Vector2f(maxWidth + 20.f, totalHeight + 16.f));
    bg.setPosition(startX - 9.f, 14.f - 7.f);
    bg.setFillColor(sf::Color(14, 15, 20, 230));
    bg.setOutlineColor(sf::Color(60, 140, 220));
    bg.setOutlineThickness(1.5f);

    window.draw(bg);
    window.draw(textHeader);
    if (hasSelection) {
        window.draw(textSelected);
    }
    window.draw(textFooter);

    // Update Toolbar Button Layout right below the HUD Box
    float buttonTopY = bg.getPosition().y + bg.getSize().y + 8.0f;
    updateButtonLayout(buttonTopY);

    // Draw the interactive buttons
    renderGUIButtons();
}
