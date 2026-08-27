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

    print::info("RoadViewer Network Editor initialized with Two-Way and One-Way road support.");
    print::info("Tools: [1] Select | [2] 2-Way Straight | [3] 2-Way Curved | [6] 1-Way Straight | [7] 1-Way Curved | [4] Intersection | [5] Gateway");
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
    settings.antialiasingLevel = roadViewer::dimensions::antialiasingLevel;
    isFullscreen = fullscreen;

    if (isFullscreen) {
        window.create(sf::VideoMode::getDesktopMode(), windowTitle, sf::Style::Fullscreen, settings);
    } else {
        window.create(sf::VideoMode(savedWidth, savedHeight), windowTitle, sf::Style::Default, settings);
    }
    window.setFramerateLimit(roadViewer::dimensions::frameRateLimit);
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
    using namespace roadViewer::dimensions;
    guiButtons.clear();
    guiButtons.push_back({ToolMode::SelectMove, "Select / Move", "[1/V]", sf::FloatRect(0, 0, btnWidth, btnHeight)});
    guiButtons.push_back({ToolMode::StraightRoad, "2-Way Straight", "[2/R]", sf::FloatRect(0, 0, btnWidth, btnHeight)});
    guiButtons.push_back({ToolMode::CurvedRoad, "2-Way Curved", "[3/C]", sf::FloatRect(0, 0, btnWidth, btnHeight)});
    guiButtons.push_back({ToolMode::StraightOneWayRoad, "1-Way Straight", "[6/O]", sf::FloatRect(0, 0, btnWidth, btnHeight)});
    guiButtons.push_back({ToolMode::CurvedOneWayRoad, "1-Way Curved", "[7/U]", sf::FloatRect(0, 0, btnWidth, btnHeight)});
    guiButtons.push_back({ToolMode::AddIntersection, "+ Intersect", "[4/I]", sf::FloatRect(0, 0, btnWidth, btnHeight)});
    guiButtons.push_back({ToolMode::AddGateway, "+ Gateway", "[5/G]", sf::FloatRect(0, 0, btnWidth, btnHeight)});
}

void RoadViewer::updateButtonLayout(float topY) {
    using namespace roadViewer::dimensions;
    float startX = hudPaddingX;

    for (size_t i = 0; i < guiButtons.size(); ++i) {
        guiButtons[i].bounds = sf::FloatRect(startX + i * (btnWidth + btnGap), topY, btnWidth, btnHeight);
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
            print::info("Active Tool: [Select & Move] - Click/drag nodes or road handles.");
            break;
        case ToolMode::StraightRoad:
            print::info("Active Tool: [Straight Two-Way Road] - Click start node, then click end node.");
            break;
        case ToolMode::CurvedRoad:
            print::info("Active Tool: [Curved Two-Way Road] - Click Start -> Handle 1 -> Handle 2 -> End Node.");
            break;
        case ToolMode::StraightOneWayRoad:
            print::info("Active Tool: [Straight One-Way Road] - Click start node (origin), then click target node (destination).");
            break;
        case ToolMode::CurvedOneWayRoad:
            print::info("Active Tool: [Curved One-Way Road] - Click Start -> Handle 1 -> Handle 2 -> End Node.");
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
            handleStraightRoadClick(mousePos, false);
            break;
        case ToolMode::CurvedRoad:
            handleCurvedRoadClick(mousePos, false);
            break;
        case ToolMode::StraightOneWayRoad:
            handleStraightRoadClick(mousePos, true);
            break;
        case ToolMode::CurvedOneWayRoad:
            handleCurvedRoadClick(mousePos, true);
            break;
        case ToolMode::AddIntersection: {
            int existingNode = network.findNodeAt(mousePos, roadViewer::dimensions::nodeClickExtraRadius);
            if (existingNode != -1) {
                auto* n = network.getNode(static_cast<size_t>(existingNode));
                if (n && n->isGateway()) {
                    network.convertGatewayToIntersection(static_cast<size_t>(existingNode));
                }
                selectedNodeId = existingNode;
            } else {
                size_t id = network.createIntersection(mousePos, roadNetwork::defaults::intersectionRadius);
                selectedNodeId = static_cast<int>(id);
            }
            break;
        }
        case ToolMode::AddGateway: {
            int existingNode = network.findNodeAt(mousePos, roadViewer::dimensions::nodeClickExtraRadius);
            if (existingNode != -1) {
                auto* n = network.getNode(static_cast<size_t>(existingNode));
                if (n && n->isIntersection()) {
                    network.convertIntersectionToGateway(static_cast<size_t>(existingNode));
                }
                selectedNodeId = existingNode;
            } else {
                size_t id = network.createGateway(mousePos, roadNetwork::defaults::gatewayRadius);
                selectedNodeId = static_cast<int>(id);
            }
            break;
        }
    }
}

void RoadViewer::handleMouseMove(const sf::Vector2f& mousePos) {
    currentMousePos = mousePos;

    hoveredButtonIndex = -1;
    if (showHUD) {
        for (size_t i = 0; i < guiButtons.size(); ++i) {
            if (guiButtons[i].bounds.contains(mousePos)) {
                hoveredButtonIndex = static_cast<int>(i);
                break;
            }
        }
    }

    hoveredNodeId = network.findNodeAt(mousePos, roadViewer::dimensions::nodeClickExtraRadius);

    if (selectedRoadId != -1) {
        hoveredHandleIndex = network.findRoadControlPointAt(static_cast<size_t>(selectedRoadId), mousePos,
                                                            roadViewer::dimensions::handleClickRadius);
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
        int clickedHandle = network.findRoadControlPointAt(static_cast<size_t>(selectedRoadId), mousePos,
                                                           roadViewer::dimensions::handleClickRadius);
        if (clickedHandle != 0) {
            draggedHandleIndex = clickedHandle;
            draggedNodeId = -1;
            return;
        }
    }

    // 2. Check if clicked on a node
    int clickedNode = network.findNodeAt(mousePos, roadViewer::dimensions::nodeClickExtraRadius);
    if (clickedNode != -1) {
        selectedNodeId = clickedNode;
        draggedNodeId = clickedNode;
        selectedRoadId = -1;
        draggedHandleIndex = 0;
        return;
    }

    // 3. Check if clicked on a road
    int clickedRoad = network.findRoadAt(mousePos, roadViewer::dimensions::roadClickTolerance);
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

void RoadViewer::handleStraightRoadClick(const sf::Vector2f& mousePos, bool isOneWay) {
    int clickedNode = network.findNodeAt(mousePos, roadViewer::dimensions::nodeRoadSnapRadius);
    size_t nodeId = 0;

    if (clickedNode != -1) {
        nodeId = static_cast<size_t>(clickedNode);
    } else {
        nodeId = network.createIntersection(mousePos, roadNetwork::defaults::intersectionRadius);
    }

    if (roadCreationStartNodeId == 0) {
        roadCreationStartNodeId = nodeId;
        selectedNodeId = static_cast<int>(nodeId);
        print::info("Road Start Node set to " + std::to_string(nodeId) + ". Now click target node (or empty space).");
    } else {
        if (roadCreationStartNodeId != nodeId) {
            size_t roadId = 0;
            if (isOneWay) {
                roadId = network.createStraightOneWayRoad(roadCreationStartNodeId, nodeId,
                                                         roadNetwork::defaults::speedLimit,
                                                         roadNetwork::defaults::laneWidth);
            } else {
                roadId = network.createStraightTwoWayRoad(roadCreationStartNodeId, nodeId,
                                                         roadNetwork::defaults::speedLimit,
                                                         roadNetwork::defaults::laneWidth);
            }
            selectedRoadId = static_cast<int>(roadId);
        }
        roadCreationStartNodeId = 0;
    }
}

void RoadViewer::handleCurvedRoadClick(const sf::Vector2f& mousePos, bool isOneWay) {
    if (curvedRoadStep == 0) {
        int clickedNode = network.findNodeAt(mousePos, roadViewer::dimensions::nodeRoadSnapRadius);
        if (clickedNode != -1) {
            roadCreationStartNodeId = static_cast<size_t>(clickedNode);
        } else {
            roadCreationStartNodeId = network.createIntersection(mousePos, roadNetwork::defaults::intersectionRadius);
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
        int clickedNode = network.findNodeAt(mousePos, roadViewer::dimensions::nodeRoadSnapRadius);
        size_t endNodeId = 0;
        if (clickedNode != -1) {
            endNodeId = static_cast<size_t>(clickedNode);
        } else {
            endNodeId = network.createIntersection(mousePos, roadNetwork::defaults::intersectionRadius);
        }

        if (roadCreationStartNodeId != endNodeId) {
            size_t roadId = 0;
            if (isOneWay) {
                roadId = network.createCurvedOneWayRoad(
                    roadCreationStartNodeId, endNodeId,
                    curveHandle1, curveHandle2,
                    roadNetwork::defaults::speedLimit,
                    roadNetwork::defaults::laneWidth
                );
            } else {
                roadId = network.createCurvedTwoWayRoad(
                    roadCreationStartNodeId, endNodeId,
                    curveHandle1, curveHandle2,
                    roadNetwork::defaults::speedLimit,
                    roadNetwork::defaults::laneWidth
                );
            }
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
    } else if (key == sf::Keyboard::Num6 || key == sf::Keyboard::O) {
        setToolMode(ToolMode::StraightOneWayRoad);
    } else if (key == sf::Keyboard::Num7 || key == sf::Keyboard::U) {
        setToolMode(ToolMode::CurvedOneWayRoad);
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
    window.clear(roadViewer::colors::background);

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
    using namespace roadViewer::dimensions;
    for (const auto& [id, road] : network.getRoads()) {
        const auto* segFwd = network.getSegment(road.forwardSegmentId);
        if (!segFwd || segFwd->spline.empty()) continue;

        sf::VertexArray roadStrip(sf::TriangleStrip);
        const int samples = roadRenderSamples;
        sf::Color asphaltColor = roadViewer::colors::asphalt;

        if (road.isOneWay()) {
            // One-Way Road: single lane corridor
            const float halfWidth = road.laneWidth * asphaltOneWayMultiplier;
            for (int i = 0; i <= samples; ++i) {
                float t = static_cast<float>(i) / samples;
                sf::Vector2f center = segFwd->spline.eval(t);
                sf::Vector2f tan = segFwd->spline.evalTangent(t);
                float len = std::hypot(tan.x, tan.y);
                sf::Vector2f n = len > 1e-4f ? sf::Vector2f(-tan.y / len, tan.x / len) : sf::Vector2f(0, 1);

                roadStrip.append(sf::Vertex(center + n * halfWidth, asphaltColor));
                roadStrip.append(sf::Vertex(center - n * halfWidth, asphaltColor));
            }
        } else {
            // Two-Way Road: two lane corridor
            const auto* segBwd = network.getSegment(road.backwardSegmentId);
            if (!segBwd || segBwd->spline.empty()) continue;

            const float halfTotalWidth = road.laneWidth * asphaltWidthMultiplier;
            for (int i = 0; i <= samples; ++i) {
                float t = static_cast<float>(i) / samples;
                sf::Vector2f posFwd = segFwd->spline.eval(t);
                sf::Vector2f posBwd = segBwd->spline.eval(1.0f - t);
                sf::Vector2f center = (posFwd + posBwd) * 0.5f;

                sf::Vector2f tan = segFwd->spline.evalTangent(t);
                float len = std::hypot(tan.x, tan.y);
                sf::Vector2f n = len > 1e-4f ? sf::Vector2f(-tan.y / len, tan.x / len) : sf::Vector2f(0, 1);

                roadStrip.append(sf::Vertex(center + n * halfTotalWidth, asphaltColor));
                roadStrip.append(sf::Vertex(center - n * halfTotalWidth, asphaltColor));
            }
        }
        window.draw(roadStrip);
    }
}

void RoadViewer::renderLanes() {
    using namespace roadViewer::dimensions;
    for (const auto& [id, seg] : network.getSegments()) {
        if (seg.spline.empty()) continue;

        sf::VertexArray vertices = seg.spline.getVertices(roadRenderSamples);
        for (size_t i = 0; i < vertices.getVertexCount(); ++i) {
            vertices[i].color = seg.laneColor;
        }
        window.draw(vertices);

        // Direction arrow
        sf::Vector2f midPos = seg.spline.eval(0.5f);
        sf::Vector2f midTan = seg.spline.evalTangent(0.5f);
        renderLaneDirectionArrow(midPos, midTan, laneArrowSize, seg.laneColor);
    }
}

void RoadViewer::renderTurnLanes() {
    using namespace roadViewer::dimensions;
    for (const auto& turn : network.getTurnLanes()) {
        if (turn.spline.empty()) continue;

        sf::VertexArray vertices = turn.spline.getVertices(turnRenderSamples);
        for (size_t i = 0; i < vertices.getVertexCount(); ++i) {
            vertices[i].color = turn.laneColor;
        }
        window.draw(vertices);

        // Direction arrow for turn lane
        sf::Vector2f midPos = turn.spline.eval(0.5f);
        sf::Vector2f midTan = turn.spline.evalTangent(0.5f);
        renderLaneDirectionArrow(midPos, midTan, turnArrowSize, turn.laneColor);
    }
}

void RoadViewer::renderLaneDirectionArrow(const sf::Vector2f& pos, const sf::Vector2f& dir, float size, sf::Color color) {
    using namespace roadViewer::dimensions;
    float len = std::hypot(dir.x, dir.y);
    if (len < 1e-4f) return;

    sf::Vector2f u = dir / len;
    sf::Vector2f n(-u.y, u.x);

    sf::ConvexShape arrow;
    arrow.setPointCount(3);
    arrow.setPoint(0, pos + u * size);
    arrow.setPoint(1, pos - u * (size * arrowWingRatio) + n * (size * arrowSpreadRatio));
    arrow.setPoint(2, pos - u * (size * arrowWingRatio) - n * (size * arrowSpreadRatio));
    arrow.setFillColor(color);

    window.draw(arrow);
}

void RoadViewer::renderNodeBases() {
    using namespace roadViewer::colors;
    using namespace roadViewer::dimensions;

    for (const auto& [id, node] : network.getNodes()) {
        bool isHovered = (static_cast<int>(id) == hoveredNodeId);
        bool isSelected = (static_cast<int>(id) == selectedNodeId);

        sf::CircleShape circle(node->getRadius());
        circle.setOrigin(node->getRadius(), node->getRadius());
        circle.setPosition(node->getPosition());

        if (node->isIntersection()) {
            circle.setFillColor(intersectionFill);
            circle.setOutlineColor(isSelected ? intersectionOutlineSelected :
                                  (isHovered ? intersectionOutlineHover : intersectionOutline));
            circle.setOutlineThickness(isSelected || isHovered ? nodeOutlineHighlight : nodeOutlineNormal);
        } else {
            circle.setFillColor(gatewayFill);
            circle.setOutlineColor(isSelected ? gatewayOutlineSelected :
                                  (isHovered ? gatewayOutlineHover : gatewayOutline));
            circle.setOutlineThickness(isSelected || isHovered ? nodeOutlineHighlight : nodeOutlineNormal);
        }

        window.draw(circle);
    }
}

void RoadViewer::renderNodeLabels() {
    if (!fontLoaded) return;
    using namespace roadViewer::colors;
    using namespace roadViewer::dimensions;

    for (const auto& [id, node] : network.getNodes()) {
        sf::Text label;
        label.setFont(font);
        label.setCharacterSize(nodeLabelFontSize);
        label.setFillColor(hudTextHeader);

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
        using namespace roadViewer::colors;
        using namespace roadViewer::dimensions;

        // 1. Draw Control Polygon connecting NodeA -> P1 -> P2 -> NodeB
        sf::VertexArray polygon(sf::LineStrip);
        polygon.append(sf::Vertex(nodeA->getPosition(), controlPolygonLine));
        polygon.append(sf::Vertex(road->controlPoint1, controlPolygonLine));
        polygon.append(sf::Vertex(road->controlPoint2, controlPolygonLine));
        polygon.append(sf::Vertex(nodeB->getPosition(), controlPolygonLine));
        window.draw(polygon);

        // 2. Draw Handle 1 (P1)
        float r1 = (draggedHandleIndex == 1) ? handleRadiusDragged :
                   ((hoveredHandleIndex == 1) ? handleRadiusHover : handleRadiusNormal);
        sf::Color c1 = (draggedHandleIndex == 1) ? handleFillDragged :
                      ((hoveredHandleIndex == 1) ? handleFillHover : handleFillNormal);

        sf::CircleShape h1(r1);
        h1.setOrigin(r1, r1);
        h1.setPosition(road->controlPoint1);
        h1.setFillColor(c1);
        h1.setOutlineColor(handleOutline);
        h1.setOutlineThickness(handleOutlineThickness);
        window.draw(h1);

        // 3. Draw Handle 2 (P2)
        float r2 = (draggedHandleIndex == 2) ? handleRadiusDragged :
                   ((hoveredHandleIndex == 2) ? handleRadiusHover : handleRadiusNormal);
        sf::Color c2 = (draggedHandleIndex == 2) ? handleFillDragged :
                      ((hoveredHandleIndex == 2) ? handleFillHover : handleFillNormal);

        sf::CircleShape h2(r2);
        h2.setOrigin(r2, r2);
        h2.setPosition(road->controlPoint2);
        h2.setFillColor(c2);
        h2.setOutlineColor(handleOutline);
        h2.setOutlineThickness(handleOutlineThickness);
        window.draw(h2);

        // 4. Draw labels for P1 and P2
        if (fontLoaded) {
            sf::Text t1, t2;
            t1.setFont(font); t2.setFont(font);
            t1.setCharacterSize(handleLabelFontSize); t2.setCharacterSize(handleLabelFontSize);
            t1.setFillColor(handleLabel);
            t2.setFillColor(handleLabel);

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
    using namespace roadViewer::colors;
    using namespace roadViewer::dimensions;

    bool isStraightMode = (currentTool == ToolMode::StraightRoad || currentTool == ToolMode::StraightOneWayRoad);
    bool isCurvedMode = (currentTool == ToolMode::CurvedRoad || currentTool == ToolMode::CurvedOneWayRoad);

    if (isStraightMode && roadCreationStartNodeId != 0) {
        const auto* startNode = network.getNode(roadCreationStartNodeId);
        if (startNode) {
            sf::VertexArray previewLine(sf::Lines);
            previewLine.append(sf::Vertex(startNode->getPosition(), previewStraightLine));
            previewLine.append(sf::Vertex(mousePos, previewStraightLine));
            window.draw(previewLine);

            sf::CircleShape targetDot(previewTargetDotRadius);
            targetDot.setOrigin(previewTargetDotRadius, previewTargetDotRadius);
            targetDot.setPosition(mousePos);
            targetDot.setFillColor(previewStraightDot);
            window.draw(targetDot);
        }
    } else if (isCurvedMode) {
        const auto* startNode = network.getNode(roadCreationStartNodeId);
        if (curvedRoadStep == 1 && startNode) {
            sf::VertexArray l(sf::Lines);
            l.append(sf::Vertex(startNode->getPosition(), previewCurvedLine));
            l.append(sf::Vertex(mousePos, previewCurvedLine));
            window.draw(l);
        } else if (curvedRoadStep == 2 && startNode) {
            sf::VertexArray l(sf::Lines);
            l.append(sf::Vertex(curveHandle1, previewCurvedLine));
            l.append(sf::Vertex(mousePos, previewCurvedLine));
            window.draw(l);

            sf::CircleShape h1(previewHandleDotRadius);
            h1.setOrigin(previewHandleDotRadius, previewHandleDotRadius);
            h1.setPosition(curveHandle1);
            h1.setFillColor(previewHandleDot);
            window.draw(h1);
        } else if (curvedRoadStep == 3 && startNode) {
            CubicBezierCurve previewCurve(startNode->getPosition(), curveHandle1, curveHandle2, mousePos);
            CubicBezierSpline previewSpline;
            previewSpline.addSegment(previewCurve);
            sf::VertexArray curveVerts = previewSpline.getVertices(roadRenderSamples);
            for (size_t i = 0; i < curveVerts.getVertexCount(); ++i) {
                curveVerts[i].color = previewCurvedSpline;
            }
            window.draw(curveVerts);

            sf::CircleShape h1(previewHandleDotRadius), h2(previewHandleDotRadius);
            h1.setOrigin(previewHandleDotRadius, previewHandleDotRadius);
            h1.setPosition(curveHandle1);
            h1.setFillColor(previewHandleDot);
            h2.setOrigin(previewHandleDotRadius, previewHandleDotRadius);
            h2.setPosition(curveHandle2);
            h2.setFillColor(previewHandleDot);
            window.draw(h1);
            window.draw(h2);
        }
    }
}

void RoadViewer::renderGUIButtons() {
    if (!fontLoaded) return;
    using namespace roadViewer::colors;
    using namespace roadViewer::dimensions;

    for (size_t i = 0; i < guiButtons.size(); ++i) {
        const auto& btn = guiButtons[i];
        bool isActive = (btn.mode == currentTool);
        bool isHovered = (static_cast<int>(i) == hoveredButtonIndex);

        sf::RectangleShape rect(sf::Vector2f(btn.bounds.width, btn.bounds.height));
        rect.setPosition(btn.bounds.left, btn.bounds.top);

        if (isActive) {
            rect.setFillColor(btnBgActive);
            rect.setOutlineColor(btnOutlineActive);
            rect.setOutlineThickness(btnOutlineThicknessActive);
        } else if (isHovered) {
            rect.setFillColor(btnBgHover);
            rect.setOutlineColor(btnOutlineHover);
            rect.setOutlineThickness(btnOutlineThicknessHover);
        } else {
            rect.setFillColor(btnBgNormal);
            rect.setOutlineColor(btnOutlineNormal);
            rect.setOutlineThickness(btnOutlineThicknessNormal);
        }

        window.draw(rect);

        sf::Text label;
        label.setFont(font);
        label.setCharacterSize(btnFontSize);
        label.setFillColor(isActive ? btnTextActive : (isHovered ? btnTextHover : btnTextNormal));
        label.setString(btn.label);

        sf::FloatRect textBounds = label.getLocalBounds();
        label.setOrigin(textBounds.left + textBounds.width * 0.5f, textBounds.top + textBounds.height * 0.5f);
        label.setPosition(btn.bounds.left + btn.bounds.width * 0.5f, btn.bounds.top + btn.bounds.height * 0.5f);

        window.draw(label);
    }
}

void RoadViewer::drawHUD() {
    if (!fontLoaded) return;
    using namespace roadViewer::colors;
    using namespace roadViewer::dimensions;

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
        case ToolMode::StraightOneWayRoad:
            ssHeader << "[ TOOL: STRAIGHT ONE-WAY ROAD"
                     << (roadCreationStartNodeId != 0 ? " (Step 2/2: Select Target Node)" : " (Step 1/2: Select Start Node)")
                     << " ]\n";
            break;
        case ToolMode::CurvedOneWayRoad:
            ssHeader << "[ TOOL: CURVED ONE-WAY ROAD (Step " << (curvedRoadStep + 1) << "/4) ]\n";
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

    size_t twoWayCount = 0;
    size_t oneWayCount = 0;
    for (const auto& [id, road] : network.getRoads()) {
        if (road.isOneWay()) oneWayCount++;
        else twoWayCount++;
    }

    ssHeader << "Network: " << network.getNodes().size() << " Nodes (" << intersectionCount << " Intersections, "
             << gatewayCount << " Gateways) | " << network.getRoads().size() << " Roads (" << twoWayCount << " 2-Way, "
             << oneWayCount << " 1-Way) | " << network.getSegments().size() << " Drive Lanes | "
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
            std::string roadTypeStr = r->isOneWay() ? (r->isCurved ? "Curved One-Way" : "Straight One-Way") :
                                                      (r->isCurved ? "Curved Two-Way" : "Straight Two-Way");
            ssSelected << ">>> Selected " << roadTypeStr << " Road R" << r->id
                       << " (" << (r->isOneWay() ? "Node " + std::to_string(r->nodeA) + " -> Node " + std::to_string(r->nodeB) :
                                                   "Node " + std::to_string(r->nodeA) + " <-> Node " + std::to_string(r->nodeB)) << ")";
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
    textHeader.setFont(font); textHeader.setCharacterSize(hudFontSize); textHeader.setFillColor(hudTextHeader);
    textSelected.setFont(font); textSelected.setCharacterSize(hudFontSize); textSelected.setFillColor(hudTextSelected);
    textFooter.setFont(font); textFooter.setCharacterSize(hudFontSize); textFooter.setFillColor(hudTextFooter);

    textHeader.setString(ssHeader.str());
    if (hasSelection) textSelected.setString(ssSelected.str());
    textFooter.setString(ssFooter.str());

    float startX = hudPaddingX;
    float currentY = hudPaddingY;

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
                               870.0f});

    float totalHeight = currentY - hudPaddingY;
    sf::RectangleShape bg(sf::Vector2f(maxWidth + 20.f, totalHeight + 16.f));
    bg.setPosition(startX - 9.f, hudPaddingY - 7.f);
    bg.setFillColor(hudBg);
    bg.setOutlineColor(hudOutline);
    bg.setOutlineThickness(hudBorderRadius);

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
