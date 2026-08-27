#include "CurveViewer.hpp"
#include "constants.hpp"
#include "toolbox.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

CurveViewer::CurveViewer(bool fullscreen, const std::string& title, unsigned int windowedWidth, unsigned int windowedHeight)
    : windowTitle(title), savedWidth(windowedWidth), savedHeight(windowedHeight), isFullscreen(fullscreen) {
    initWindow(isFullscreen);
    tryLoadFont();

    print::info("Interactive Bezier Spline & Traffic Simulation initialized.");
    print::info("Controls: [Tab/M] Switch Mode | [Space/S] Sim Play/Pause | [Left-Click] Add/Drag | [H] HUD Help");
}

CurveViewer::CurveViewer(unsigned int width, unsigned int height, const std::string& title, bool fullscreen)
    : windowTitle(title), savedWidth(width), savedHeight(height), isFullscreen(fullscreen) {
    initWindow(isFullscreen);
    tryLoadFont();
}

void CurveViewer::initWindow(bool fullscreen) {
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

void CurveViewer::toggleFullscreen() {
    initWindow(!isFullscreen);
}

void CurveViewer::setMode(ViewerMode mode) {
    currentMode = mode;
    if (currentMode == ViewerMode::Simulation) {
        print::info("Switched to Agent Simulation Mode.");
        if (!hasSpline) {
            print::warning("No active spline. Press 'D' to load demo or [Tab] to return to Curve Creation.");
        }
    } else {
        print::info("Switched to Curve Creation Mode.");
    }
}

void CurveViewer::toggleMode() {
    if (currentMode == ViewerMode::CurveCreation) {
        setMode(ViewerMode::Simulation);
    } else {
        setMode(ViewerMode::CurveCreation);
    }
}

void CurveViewer::tryLoadFont() {
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

void CurveViewer::run() {
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
}

// ----------------------------------------------------------------------------
// Event Handling
// ----------------------------------------------------------------------------

void CurveViewer::processEvents() {
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

void CurveViewer::handleMousePress(sf::Mouse::Button button, const sf::Vector2f& mousePos) {
    if (currentMode == ViewerMode::CurveCreation) {
        handleCurveMousePress(button, mousePos);
    } else {
        // In simulation mode, allow moving existing control points
        if (button == sf::Mouse::Left) {
            int clickedIdx = findPointAt(mousePos);
            if (clickedIdx != -1) {
                draggedPointIndex = clickedIdx;
            }
        }
    }
}

void CurveViewer::handleMouseMove(const sf::Vector2f& mousePos) {
    handleCurveMouseMove(mousePos);
}

void CurveViewer::handleMouseRelease(sf::Mouse::Button button) {
    handleCurveMouseRelease(button);
}

void CurveViewer::handleKeyPress(sf::Keyboard::Key key) {
    // Global shortcut keys
    if (key == sf::Keyboard::Escape) {
        window.close();
    } else if (key == sf::Keyboard::F11 || key == sf::Keyboard::F) {
        toggleFullscreen();
    } else if (key == sf::Keyboard::H || key == sf::Keyboard::F1) {
        showHelp = !showHelp;
    } else if (key == sf::Keyboard::Tab || key == sf::Keyboard::M) {
        toggleMode();
    } else if (key == sf::Keyboard::S) {
        toggleSimulation();
    } else {
        // Dispatch to active mode subsystem
        if (currentMode == ViewerMode::CurveCreation) {
            handleCurveKeyPress(key);
        } else {
            handleSimulationKeyPress(key);
        }
    }
}

// ----------------------------------------------------------------------------
// Curve Creation & Manipulation Subsystem
// ----------------------------------------------------------------------------

void CurveViewer::handleCurveMousePress(sf::Mouse::Button button, const sf::Vector2f& mousePos) {
    if (button == sf::Mouse::Left) {
        int clickedIdx = findPointAt(mousePos);
        if (clickedIdx != -1) {
            draggedPointIndex = clickedIdx;
        } else {
            addPoint(mousePos);
        }
    }
}

void CurveViewer::handleCurveMouseMove(const sf::Vector2f& mousePos) {
    if (draggedPointIndex >= 0 && draggedPointIndex < static_cast<int>(controlPoints.size())) {
        controlPoints[draggedPointIndex] = mousePos;
        rebuildSpline();
    }
    hoveredPointIndex = findPointAt(mousePos);
}

void CurveViewer::handleCurveMouseRelease(sf::Mouse::Button button) {
    if (button == sf::Mouse::Left) {
        draggedPointIndex = -1;
    }
}

void CurveViewer::handleCurveKeyPress(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Z || key == sf::Keyboard::BackSpace) {
        removeLastPoint();
    } else if (key == sf::Keyboard::C || key == sf::Keyboard::Delete) {
        clearPoints();
    } else if (key == sf::Keyboard::D) {
        loadSampleSpline();
    } else if (key == sf::Keyboard::T) {
        showTangents = !showTangents;
    } else if (key == sf::Keyboard::P) {
        showControlPolygon = !showControlPolygon;
    }
}

void CurveViewer::addPoint(const sf::Vector2f& point) {
    controlPoints.push_back(point);
    rebuildSpline();

    size_t count = controlPoints.size();
    if (count < 4) {
        print::info("Added point P" + std::to_string(count - 1) + " (" + std::to_string(static_cast<int>(point.x)) + ", " +
                    std::to_string(static_cast<int>(point.y)) + "). Need " + std::to_string(4 - count) +
                    " more for the first Bezier curve.");
    } else if (count == 4) {
        print::info("4 points reached! Created initial Bezier curve.");
    } else {
        size_t extra = (count - 4) % 3;
        if (extra == 0) {
            size_t segCount = 1 + (count - 4) / 3;
            print::info("Added segment! Spline now has " + std::to_string(segCount) + " segments (" +
                        std::to_string(count) + " points).");
        } else {
            print::info("Added point P" + std::to_string(count - 1) + ". Need " + std::to_string(3 - extra) +
                        " more point(s) to complete next segment.");
        }
    }
}

void CurveViewer::removeLastPoint() {
    if (!controlPoints.empty()) {
        controlPoints.pop_back();
        rebuildSpline();
        print::info("Removed last point. " + std::to_string(controlPoints.size()) + " point(s) remaining.");
    }
}

void CurveViewer::clearPoints() {
    controlPoints.clear();
    rebuildSpline();
    print::info("Cleared all control points.");
}

void CurveViewer::loadSampleSpline() {
    sf::Vector2u winSize = window.getSize();
    float cx = winSize.x * 0.5f;
    float cy = winSize.y * 0.5f;
    float scale = std::min(winSize.x, winSize.y) * 0.32f;

    controlPoints = {
        sf::Vector2f(cx - scale, cy + scale * 0.4f), // P0: Start (bottom left)
        sf::Vector2f(cx - scale, cy - scale * 0.8f), // P1: Control 1 (pulls up)
        sf::Vector2f(cx + scale, cy - scale * 0.8f), // P2: Control 2 (pulls right)
        sf::Vector2f(cx + scale, cy + scale * 0.4f), // P3: Segment 1 End / Segment 2 Start (bottom right)
        sf::Vector2f(cx + scale, cy + scale * 1.1f), // P4: Control 1 (Seg 2)
        sf::Vector2f(cx - scale, cy + scale * 1.1f), // P5: Control 2 (Seg 2)
        sf::Vector2f(cx - scale, cy + scale * 0.4f)  // P6: End (closes loop)
    };
    rebuildSpline();
    print::info("Loaded sample spline with 2 segments (7 points).");
}

void CurveViewer::rebuildSpline() {
    spline = CubicBezierSpline();
    hasSpline = false;
    splineVertices.clear();

    if (controlPoints.size() < 4) {
        return;
    }

    // First segment: P0, P1, P2, P3
    CubicBezierCurve firstSegment(
        controlPoints[0],
        controlPoints[1],
        controlPoints[2],
        controlPoints[3]
    );
    spline.addSegment(firstSegment);

    // Continuous segments: each uses previous segment's end point and 3 new points
    for (size_t i = 4; i + 2 < controlPoints.size(); i += 3) {
        spline.addContinuousSegment(
            controlPoints[i],
            controlPoints[i + 1],
            controlPoints[i + 2]
        );
    }

    hasSpline = true;
    splineVertices = spline.getVertices(60);

    // Align agent to current t position on updated spline
    agent.update(spline);
}

int CurveViewer::findPointAt(const sf::Vector2f& pos, float radius) const {
    float rSquared = radius * radius;
    for (int i = static_cast<int>(controlPoints.size()) - 1; i >= 0; --i) {
        float dx = controlPoints[i].x - pos.x;
        float dy = controlPoints[i].y - pos.y;
        if ((dx * dx + dy * dy) <= rSquared) {
            return i;
        }
    }
    return -1;
}

// ----------------------------------------------------------------------------
// Agent Simulation Subsystem
// ----------------------------------------------------------------------------

void CurveViewer::handleSimulationKeyPress(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Space) {
        toggleSimulation();
    } else if (key == sf::Keyboard::R) {
        resetSimulation();
    } else if (key == sf::Keyboard::Up || key == sf::Keyboard::Equal || key == sf::Keyboard::Add) {
        float newSpeed = agent.getSpeed() + 0.0005f;
        agent.setSpeed(newSpeed);
        print::info("Agent speed increased to: " + std::to_string(agent.getSpeed()));
    } else if (key == sf::Keyboard::Down || key == sf::Keyboard::Dash || key == sf::Keyboard::Subtract) {
        float newSpeed = std::max(0.0002f, agent.getSpeed() - 0.0005f);
        agent.setSpeed(newSpeed);
        print::info("Agent speed decreased to: " + std::to_string(agent.getSpeed()));
    } else if (key == sf::Keyboard::D) {
        loadSampleSpline();
    }
}

void CurveViewer::startSimulation() {
    if (!hasSpline) {
        print::warning("Cannot start simulation: need at least 4 points for a valid Bézier spline.");
        return;
    }
    simulationRunning = true;
    print::info("Simulation started.");
}

void CurveViewer::pauseSimulation() {
    simulationRunning = false;
    print::info("Simulation paused.");
}

void CurveViewer::toggleSimulation() {
    if (simulationRunning) {
        pauseSimulation();
    } else {
        startSimulation();
    }
}

void CurveViewer::resetSimulation() {
    agent.reset();
    if (hasSpline) {
        agent.update(spline);
    }
    print::info("Simulation reset to spline origin.");
}

void CurveViewer::update(float dt) {
    if (simulationRunning && hasSpline) {
        updateSimulation(dt);
    }
}

void CurveViewer::updateSimulation(float /* dt */) {
    agent.update(spline);
}

// ----------------------------------------------------------------------------
// Rendering Pipeline
// ----------------------------------------------------------------------------

void CurveViewer::render() {
    window.clear(sf::Color(25, 25, 30));

    // 1. Render Curve Creation Elements (Spline track, polygon, tangents, control points)
    renderCurveElements();

    // 2. Render Agent Simulation Elements (Traffic agent)
    renderSimulationElements();

    // 3. Render HUD & UI Overlay
    if (showHelp) {
        drawHUD();
    }

    window.display();
}

void CurveViewer::renderCurveElements() {
    if (showControlPolygon) {
        drawControlPolygon();
    }

    drawCurve();

    if (showTangents && hasSpline) {
        drawTangents();
    }

    drawControlPoints();
}

void CurveViewer::renderSimulationElements() {
    if (hasSpline) {
        window.draw(agent);
    }
}

void CurveViewer::drawControlPolygon() {
    if (controlPoints.size() < 2) return;

    sf::VertexArray lines(sf::Lines);
    for (size_t i = 0; i + 1 < controlPoints.size(); ++i) {
        sf::Color lineColor(120, 120, 130, 140);
        // Dim pending handle connections if segment is incomplete
        if (i >= 3 && (i - 3) % 3 != 0 && i + 1 >= controlPoints.size()) {
            lineColor = sf::Color(180, 140, 80, 120);
        }
        lines.append(sf::Vertex(controlPoints[i], lineColor));
        lines.append(sf::Vertex(controlPoints[i + 1], lineColor));
    }
    window.draw(lines);
}

void CurveViewer::drawCurve() {
    if (!hasSpline || splineVertices.getVertexCount() == 0) return;

    // Draw the curve roadway with a clean bright cyan color
    for (size_t i = 0; i < splineVertices.getVertexCount(); ++i) {
        splineVertices[i].color = sf::Color(100, 220, 255);
    }
    window.draw(splineVertices);
}

void CurveViewer::drawTangents() {
    if (!hasSpline) return;

    sf::VertexArray tangentLines(sf::Lines);
    const int samples = 40;
    const float tangentLength = 24.0f;

    for (int i = 0; i <= samples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(samples);
        sf::Vector2f pos = spline.eval(t);
        sf::Vector2f tan = spline.evalTangent(t);

        float len = std::hypot(tan.x, tan.y);
        if (len > 1e-4f) {
            sf::Vector2f unitTan = (tan / len) * tangentLength;
            tangentLines.append(sf::Vertex(pos, sf::Color(255, 120, 180, 220)));
            tangentLines.append(sf::Vertex(pos + unitTan, sf::Color(255, 50, 120, 220)));
        }
    }
    window.draw(tangentLines);
}

void CurveViewer::drawControlPoints() {
    sf::CircleShape circle;

    for (size_t i = 0; i < controlPoints.size(); ++i) {
        bool isAnchor = (i % 3 == 0);
        bool isHovered = (static_cast<int>(i) == hoveredPointIndex);
        bool isDragged = (static_cast<int>(i) == draggedPointIndex);

        float radius = isAnchor ? 7.0f : 5.0f;
        if (isDragged) radius += 3.0f;
        else if (isHovered) radius += 2.0f;

        circle.setRadius(radius);
        circle.setOrigin(radius, radius);
        circle.setPosition(controlPoints[i]);

        if (isDragged) {
            circle.setFillColor(sf::Color(255, 70, 70));
            circle.setOutlineColor(sf::Color::White);
            circle.setOutlineThickness(2.0f);
        } else if (isHovered) {
            circle.setFillColor(sf::Color(80, 255, 120));
            circle.setOutlineColor(sf::Color::White);
            circle.setOutlineThickness(2.0f);
        } else if (isAnchor) {
            circle.setFillColor(sf::Color(50, 180, 255));
            circle.setOutlineColor(sf::Color::White);
            circle.setOutlineThickness(1.5f);
        } else {
            circle.setFillColor(sf::Color(255, 215, 60));
            circle.setOutlineColor(sf::Color(40, 40, 40));
            circle.setOutlineThickness(1.0f);
        }

        window.draw(circle);

        if (fontLoaded) {
            sf::Text label;
            label.setFont(font);
            label.setCharacterSize(12);
            label.setFillColor(sf::Color(220, 220, 220));
            label.setString("P" + std::to_string(i));
            label.setPosition(controlPoints[i].x + radius + 3.f, controlPoints[i].y - radius - 3.f);
            window.draw(label);
        }
    }
}

void CurveViewer::drawHUD() {
    if (!fontLoaded) return;

    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(14);
    text.setFillColor(sf::Color(220, 220, 230));

    std::ostringstream ss;

    // Mode Banner Header
    if (currentMode == ViewerMode::CurveCreation) {
        ss << "[ MODE: CURVE CREATION ]\n";
    } else {
        ss << "[ MODE: AGENT SIMULATION - " << (simulationRunning ? "RUNNING" : "PAUSED") << " ]\n";
    }

    // Spline & Curve Status
    size_t count = controlPoints.size();
    if (count == 0) {
        ss << "Status: Click to place P0 (Start Point)\n";
    } else if (count == 1) {
        ss << "Status: Click to place P1 (Tangent Handle 1)\n";
    } else if (count == 2) {
        ss << "Status: Click to place P2 (Tangent Handle 2)\n";
    } else if (count == 3) {
        ss << "Status: Click to place P3 (End Point of 1st Curve)\n";
    } else {
        size_t segCount = 1 + (count - 4) / 3;
        size_t pending = (count - 4) % 3;
        if (pending == 0) {
            if (segCount == 1) {
                ss << "Status: Bezier Curve active (4 points). Click 3 more points for a Spline!\n";
            } else {
                ss << "Status: Bezier Spline active (" << segCount << " segments, " << count << " points)\n";
            }
        } else {
            ss << "Status: Building segment " << (segCount + 1) << " (" << pending << "/3 points placed)\n";
        }
    }

    // Simulation Stats
    if (hasSpline) {
        std::ostringstream simStream;
        simStream.precision(3);
        simStream << std::fixed;
        simStream << "Agent: t=" << agent.getT() << " | Speed=" << agent.getSpeed()
                  << " | Heading=" << static_cast<int>(agent.getRotation()) << " deg | Sim: "
                  << (simulationRunning ? "Running" : "Paused") << "\n";
        ss << simStream.str();
    }

    // Contextual Controls
    if (currentMode == ViewerMode::CurveCreation) {
        ss << "Curve: [Left-Click] Add/Drag  [Z] Undo  [C] Clear  [D] Demo  [T] Tangents  [P] Polygon\n";
        ss << "Global: [Tab/M] Switch Mode  [S] Start/Pause Sim  [F11] Fullscreen  [H] Help  [Esc] Exit";
    } else {
        ss << "Sim: [Space/S] Play/Pause  [R] Reset Agent  [Up/Down] Speed (+/-)\n";
        ss << "Global: [Tab/M] Switch Mode  [D] Load Demo Spline  [F11] Fullscreen  [H] Help  [Esc] Exit";
    }

    text.setString(ss.str());
    text.setPosition(14.0f, 14.0f);

    // Background panel with mode-colored outline for readability
    sf::FloatRect bounds = text.getGlobalBounds();
    sf::RectangleShape bg(sf::Vector2f(bounds.width + 16.f, bounds.height + 14.f));
    bg.setPosition(bounds.left - 8.f, bounds.top - 6.f);
    bg.setFillColor(sf::Color(15, 15, 20, 215));
    bg.setOutlineColor(currentMode == ViewerMode::CurveCreation ? sf::Color(60, 140, 220) : sf::Color(50, 200, 100));
    bg.setOutlineThickness(1.5f);

    window.draw(bg);
    window.draw(text);
}