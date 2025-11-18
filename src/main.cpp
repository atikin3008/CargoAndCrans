#include "../include/Port.h"
#include "../include/Schedule.h"
#include "../include/GUI.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string resolvePath(const std::string &filename) {
    namespace fs = std::filesystem;
    try {
        const fs::path direct = fs::path(filename);
        if (fs::exists(direct)) {
            return direct.string();
        }

        const fs::path parent = fs::path("..") / filename;
        if (fs::exists(parent)) {
            return parent.string();
        }
    } catch (const std::exception &) {
    }

    return filename;
}

struct LayoutBounds {
    float minX{0.f};
    float maxX{100.f};
    float minY{0.f};
    float maxY{100.f};
};

LayoutBounds calculateBounds(const gui::AnimationLayout &layout) {
    LayoutBounds bounds;
    bounds.minX = std::numeric_limits<float>::max();
    bounds.maxX = std::numeric_limits<float>::lowest();
    bounds.minY = std::numeric_limits<float>::max();
    bounds.maxY = std::numeric_limits<float>::lowest();

    auto extend = [&bounds](const gui::Coordinate &coord) {
        bounds.minX = std::min(bounds.minX, static_cast<float>(coord.x));
        bounds.maxX = std::max(bounds.maxX, static_cast<float>(coord.x));
        bounds.minY = std::min(bounds.minY, static_cast<float>(coord.y));
        bounds.maxY = std::max(bounds.maxY, static_cast<float>(coord.y));
    };

    for (const auto &lane : layout.lanes) {
        extend(lane.arrivalEntry);
        extend(lane.dockPoint);
        extend(lane.departureExit);
    }

    if (bounds.minX == std::numeric_limits<float>::max()) {
        bounds = LayoutBounds{};
    }
    if (bounds.maxX - bounds.minX < 1.f) {
        bounds.maxX = bounds.minX + 1.f;
    }
    if (bounds.maxY - bounds.minY < 1.f) {
        bounds.maxY = bounds.minY + 1.f;
    }

    return bounds;
}

sf::Vector2f mapToScreen(const gui::Coordinate &coord,
                         const LayoutBounds &bounds,
                         const sf::Vector2f &padding,
                         const sf::Vector2f &areaSize) {
    const float normalizedX = (static_cast<float>(coord.x) - bounds.minX) / (bounds.maxX - bounds.minX);
    const float normalizedY = (static_cast<float>(coord.y) - bounds.minY) / (bounds.maxY - bounds.minY);
    const float x = padding.x + normalizedX * areaSize.x;
    const float y = padding.y + normalizedY * areaSize.y;
    return {x, y};
}

sf::Color colorForPhase(gui::AnimationPhase phase) {
    switch (phase) {
        case gui::AnimationPhase::Arrival:
            return sf::Color(52, 152, 219); // blue
        case gui::AnimationPhase::Docking:
            return sf::Color(241, 196, 15); // yellow
        case gui::AnimationPhase::Unloading:
            return sf::Color(230, 126, 34); // orange
        case gui::AnimationPhase::Departure:
            return sf::Color(46, 204, 113); // green
    }
    return sf::Color::White;
}

struct FontStorage {
    sf::Font font;
    bool loaded{false};
};

FontStorage loadFont() {
    FontStorage storage;
    const std::array<const char *, 8> candidates = {
        "fonts/DejaVuSans.ttf",
        "../fonts/DejaVuSans.ttf",
        "assets/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf"};

    for (const auto *path : candidates) {
        if (path && storage.font.openFromFile(std::filesystem::path(path))) {
            storage.loaded = true;
            break;
        }
    }
    return storage;
}

void drawLanes(sf::RenderTarget &target,
               const gui::AnimationLayout &layout,
               const LayoutBounds &bounds,
               const sf::Vector2f &padding,
               const sf::Vector2f &areaSize) {
    for (const auto &lane : layout.lanes) {
        sf::VertexArray lines(sf::PrimitiveType::LineStrip);
        lines.resize(3);
        lines[0].position = mapToScreen(lane.arrivalEntry, bounds, padding, areaSize);
        lines[1].position = mapToScreen(lane.dockPoint, bounds, padding, areaSize);
        lines[2].position = mapToScreen(lane.departureExit, bounds, padding, areaSize);

        for (std::size_t i = 0; i < lines.getVertexCount(); ++i) {
            lines[i].color = sf::Color(100, 110, 120);
        }

        target.draw(lines);

        constexpr float markerRadius = 6.f;
        const sf::Color markerColor(149, 165, 166);
        std::array<gui::Coordinate, 3> points{lane.arrivalEntry, lane.dockPoint, lane.departureExit};
        for (const auto &point : points) {
            sf::CircleShape marker(markerRadius);
            marker.setOrigin(sf::Vector2f(markerRadius, markerRadius));
            marker.setFillColor(markerColor);
            marker.setPosition(mapToScreen(point, bounds, padding, areaSize));
            target.draw(marker);
        }
    }
}

void drawShips(sf::RenderTarget &target,
               const std::vector<std::shared_ptr<const gui::AnimationPosition>> &state,
               const LayoutBounds &bounds,
               const sf::Vector2f &padding,
               const sf::Vector2f &areaSize,
               std::array<int, 4> &phaseCounters) {
    constexpr float shipRadius = 10.f;
    for (const auto &frame : state) {
        if (!frame) {
            continue;
        }
        const auto coords = frame->coordinates();
        const auto position = mapToScreen(gui::Coordinate{coords.first, coords.second}, bounds, padding, areaSize);
        sf::CircleShape ship(shipRadius);
        ship.setOrigin(sf::Vector2f(shipRadius, shipRadius));
        const auto phase = frame->phase();
        ship.setFillColor(colorForPhase(phase));
        ship.setOutlineColor(sf::Color::Black);
        ship.setOutlineThickness(1.5f);
        ship.setPosition(position);
        target.draw(ship);

        switch (phase) {
            case gui::AnimationPhase::Arrival:
                ++phaseCounters[0];
                break;
            case gui::AnimationPhase::Docking:
                ++phaseCounters[1];
                break;
            case gui::AnimationPhase::Unloading:
                ++phaseCounters[2];
                break;
            case gui::AnimationPhase::Departure:
                ++phaseCounters[3];
                break;
        }
    }
}

void drawHud(sf::RenderTarget &target,
             const FontStorage &fontStorage,
             std::size_t frameIndex,
             std::size_t totalFrames,
             types::time_t tick,
             std::uint16_t framesPerTick,
             double speedMultiplier,
             bool paused,
             const std::array<int, 4> &phaseCounters) {
    const float barWidth = 400.f;
    const float barHeight = 8.f;
    const float margin = 20.f;

    sf::RectangleShape background(sf::Vector2f(barWidth, barHeight));
    background.setPosition(sf::Vector2f(margin, margin));
    background.setFillColor(sf::Color(44, 62, 80));
    target.draw(background);

    const float progress =
        totalFrames > 1 ? static_cast<float>(frameIndex) / static_cast<float>(totalFrames - 1) : 1.0f;
    sf::RectangleShape fill(sf::Vector2f(barWidth * progress, barHeight));
    fill.setPosition(sf::Vector2f(margin, margin));
    fill.setFillColor(sf::Color(52, 152, 219));
    target.draw(fill);

    if (!fontStorage.loaded) {
        return;
    }

    auto makeText = [&](const std::string &value, float x, float y, unsigned size = 16U) {
        sf::Text text(fontStorage.font);
        text.setString(value);
        text.setCharacterSize(size);
        text.setFillColor(sf::Color::White);
        text.setPosition(sf::Vector2f(x, y));
        return text;
    };

    std::ostringstream status;
    status << "Frame " << frameIndex + 1 << " / " << totalFrames << " | Tick " << tick << " ("
           << framesPerTick << " frames)";
    target.draw(makeText(status.str(), margin, margin + barHeight + 6.f));

    std::ostringstream mode;
    mode << (paused ? "Paused" : "Playing") << " | Speed: x" << std::fixed << std::setprecision(1)
         << speedMultiplier;
    target.draw(makeText(mode.str(), margin, margin + barHeight + 26.f));

    const std::array<std::pair<std::string, sf::Color>, 4> legend = {{
        {"Queue", sf::Color(52, 152, 219)},
        {"Docking", sf::Color(241, 196, 15)},
        {"Unloading", sf::Color(230, 126, 34)},
        {"Departure", sf::Color(46, 204, 113)},
    }};

    float legendX = margin;
    float legendY = margin + barHeight + 50.f;
    for (std::size_t i = 0; i < legend.size(); ++i) {
        sf::RectangleShape rect(sf::Vector2f(18.f, 18.f));
        rect.setFillColor(legend[i].second);
        rect.setPosition(sf::Vector2f(legendX, legendY));
        target.draw(rect);

        std::ostringstream text;
        text << legend[i].first << ": " << phaseCounters[i];
        target.draw(makeText(text.str(), legendX + 24.f, legendY - 2.f));
        legendY += 24.f;
    }

    std::vector<std::string> instructions = {
        "Space - pause/resume",
        "Left/Right arrows - frame step",
        "Up/Down arrows - speed",
        "R - restart",
        "Esc - exit"};

    float instX = margin;
    const auto viewSize = target.getView().getSize();
    float instY = viewSize.y - margin - static_cast<float>(instructions.size()) * 22.f;
    for (const auto &line : instructions) {
        target.draw(makeText(line, instX, instY));
        instY += 20.f;
    }
}

void runVisualizer(const gui::GUI &guiData) {
    const auto totalFrames = guiData.frameCount();
    if (totalFrames == 0) {
        std::cout << "No data to visualize\n";
        return;
    }

    const auto &layout = guiData.layout();
    const LayoutBounds bounds = calculateBounds(layout);
    sf::RenderWindow window(sf::VideoMode({1280u, 720u}), "Port Visualizer", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    FontStorage fontStorage = loadFont();
    if (!fontStorage.loaded) {
        std::cout << "Warning: font not found. Tips are hidden (drop DejaVuSans.ttf into fonts/)\n";
    }

    sf::Clock clock;
    double frameAccumulator = 0.0;
    double playbackSpeed = 30.0;
    bool paused = false;
    std::size_t frameIndex = 0;

    const sf::Vector2f padding(120.f, 80.f);
    const auto winSize = window.getSize();
    const sf::Vector2f areaSize(static_cast<float>(winSize.x) - padding.x * 2.f,
                                static_cast<float>(winSize.y) - padding.y * 2.f);

    std::cout << "Legend: blue=queue, yellow=docking, orange=unloading, green=departure\n";

    while (window.isOpen()) {
        while (auto eventOpt = window.pollEvent()) {
            const sf::Event &event = *eventOpt;
            if (event.is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                switch (keyPressed->code) {
                    case sf::Keyboard::Key::Escape:
                        window.close();
                        break;
                    case sf::Keyboard::Key::Space:
                        paused = !paused;
                        break;
                    case sf::Keyboard::Key::R:
                        frameIndex = 0;
                        frameAccumulator = 0.0;
                        paused = false;
                        break;
                    case sf::Keyboard::Key::Right:
                        if (frameIndex + 1 < totalFrames) {
                            ++frameIndex;
                        }
                        break;
                    case sf::Keyboard::Key::Left:
                        if (frameIndex > 0) {
                            --frameIndex;
                        }
                        break;
                    case sf::Keyboard::Key::Up:
                        playbackSpeed = std::min(playbackSpeed + 5.0, 240.0);
                        break;
                    case sf::Keyboard::Key::Down:
                        playbackSpeed = std::max(playbackSpeed - 5.0, 5.0);
                        break;
                    default:
                        break;
                }
            }
        }

        const double delta = clock.restart().asSeconds();
        if (!paused) {
            frameAccumulator += delta * playbackSpeed;
            while (frameAccumulator >= 1.0) {
                if (frameIndex + 1 < totalFrames) {
                    ++frameIndex;
                } else {
                    paused = true;
                    frameAccumulator = 0.0;
                    break;
                }
                frameAccumulator -= 1.0;
            }
        }

        const std::uint16_t framesPerTick = std::max<std::uint16_t>(guiData.framesPerTick(), 1);
        const types::time_t tick = static_cast<types::time_t>(frameIndex / framesPerTick);
        const std::uint16_t subframe = static_cast<std::uint16_t>(frameIndex % framesPerTick);
        const auto state = guiData.stateAt(tick, subframe);

        std::array<int, 4> counts{0, 0, 0, 0};

        window.clear(sf::Color(22, 30, 42));
        drawLanes(window, layout, bounds, padding, areaSize);
        drawShips(window, state, bounds, padding, areaSize, counts);
        drawHud(window, fontStorage, frameIndex, totalFrames, tick, framesPerTick, playbackSpeed / 30.0, paused, counts);
        window.display();
    }
}

} // namespace

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    const auto settingsPath = resolvePath("settings.json");
    const auto schedulePath = resolvePath("schedule.json");

    try {
        Port port(settingsPath, schedulePath);
        port.process();

        gui::GUI gui(settingsPath);
        gui.generateAnimations(port);
        runVisualizer(gui);
    } catch (const std::exception &err) {
        std::cerr << "Error: " << err.what() << '\n';
        return 1;
    }

    return 0;
}
