#include "../include/GUI.h"
#include "../include/Statistics.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace {
std::map<types::CargoType, std::size_t> readCraneTotals(const Settings &settings) {
    std::map<types::CargoType, std::size_t> totals;
    auto cranesNode = settings.get("cranes_amount");
    for (const auto &[key, value] : cranesNode.as<SettingsNode::object_t>()) {
        totals[types::getTypeByString(key)] = static_cast<std::size_t>(value.as<int64_t>());
    }
    for (types::CargoType type : {types::CargoType::BULK, types::CargoType::LIQUID, types::CargoType::CONTAINER}) {
        if (!totals.contains(type)) {
            totals[type] = 0;
        }  
    }
    return totals;
}

float clampUnit(float value) {
    if (value < 0.f)
        return 0.f;
    if (value > 1.f)
        return 1.f;
    return value;
}
} // namespace

PortGUI::PortGUI(Port &port, const std::string &settingsFile, const std::string &fontPath)
    : port_(port),
      settings_(settingsFile),
      window_(sf::VideoMode(sf::Vector2u{static_cast<unsigned>(windowWidth_),
                                         static_cast<unsigned>(windowHeight_)}),
              "Port visualizer", sf::Style::Close) {
    window_.setFramerateLimit(60);
    if (!font_.openFromFile(fontPath)) {
        throw std::runtime_error("Unable to load font from " + fontPath);
    }

    typeOrder_ = {types::CargoType::BULK, types::CargoType::LIQUID, types::CargoType::CONTAINER};
    typeTotals_ = readCraneTotals(settings_);

    events_ = port_.get();

    targetTicks_ = settings_.get("ticks_amount").as<int64_t>();
    if (!events_.empty()) {
        targetTicks_ = std::max(targetTicks_, events_.back()->getTime());
    }

    for (auto type : typeOrder_) {
        queues_[type] = {};
    }

    buildLayout();
}

void PortGUI::run() {
    sf::Clock clock;
    while (window_.isOpen()) {
        handleWindowEvents();
        float dt = clock.restart().asSeconds();
        dt = std::min(dt, 0.1f);
        processSimulation(dt);
        updateDepartures(dt);
        drawFrame();
    }
}

void PortGUI::buildLayout() {
    laneMetrics_.clear();
    const std::size_t typeCount = typeOrder_.size();
    const float totalWidth = windowWidth_ - 2.f * horizontalMargin_;
    const float columnWidth =
        (totalWidth - columnSpacing_ * static_cast<float>(typeCount - 1)) / static_cast<float>(typeCount);

    const float craneTop = topBarHeight_;
    const float craneHeight = windowHeight_ - craneTop - queueHeight_ - verticalMargin_;
    const float queueTop = craneTop + craneHeight + 24.f;

    for (std::size_t idx = 0; idx < typeCount; ++idx) {
        float left = horizontalMargin_ + idx * (columnWidth + columnSpacing_);
        laneMetrics_[typeOrder_[idx]] = LaneMetrics{left, columnWidth, craneTop, craneHeight, queueTop, queueHeight_};
    }

    craneVisuals_.clear();
    craneIndex_.clear();
    actualTypeCounts_.clear();

    const auto &cranes = port_.getCranes();
    std::map<types::CargoType, std::size_t> counters;
    for (auto type : typeOrder_) {
        counters[type] = 0;
        actualTypeCounts_[type] = 0;
    }

    const float innerOffsetX = 12.f;
    const float innerOffsetY = 18.f;
    const float craneWidth = 134.f;
    const float craneHeightBox = 58.f;
    const float craneSpacingX = 28.f;
    const float craneSpacingY = 20.f;

    for (const auto &crane : cranes) {
        CraneVisual visual;
        visual.crane = crane;
        visual.type = crane->getType();
        visual.orderIndex = counters[visual.type]++;

        const auto laneIt = laneMetrics_.find(visual.type);
        if (laneIt == laneMetrics_.end()) {
            continue;
        }
        const LaneMetrics &lane = laneIt->second;
        const std::size_t row = visual.orderIndex % kMaxCraneRows;
        const std::size_t column = visual.orderIndex / kMaxCraneRows;

        float x = lane.left + innerOffsetX + column * (craneWidth + craneSpacingX);
        float y = lane.craneTop + innerOffsetY + row * (craneHeightBox + craneSpacingY) + verticalMargin_;

        visual.position = {x, y};
        visual.size = {craneWidth, craneHeightBox};

        std::size_t visualIndex = craneVisuals_.size();
        craneVisuals_.push_back(visual);
        craneIndex_[visual.crane] = visualIndex;
        actualTypeCounts_[visual.type]++;
    }
}

void PortGUI::handleWindowEvents() {
    while (const std::optional<sf::Event> event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
            continue;
        }

        if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            switch (keyPressed->code) {
                case sf::Keyboard::Key::Escape:
                    window_.close();
                    break;
                case sf::Keyboard::Key::Space:
                    paused_ = !paused_;
                    break;
                case sf::Keyboard::Key::Up:
                    adjustTimeScale(+1);
                    break;
                case sf::Keyboard::Key::Down:
                    adjustTimeScale(-1);
                    break;
                default:
                    break;
            }
        }
    }
}

void PortGUI::processSimulation(float dt) {
    if (paused_ || events_.empty()) {
        simTime_ = std::min(simTime_, static_cast<float>(targetTicks_));
        return;
    }

    simTime_ = std::min(simTime_ + dt * timeScale_, static_cast<float>(targetTicks_));

    while (nextEventIndex_ < events_.size() && events_[nextEventIndex_]->getTime() <= simTime_) {
        handleSimulationEvent(events_[nextEventIndex_]);
        ++nextEventIndex_;
    }
}

void PortGUI::handleSimulationEvent(const std::shared_ptr<Event> &event) {
    if (!event) {
        return;
    }
    const auto type = event->getType();
    switch (type) {
        case types::EventType::ON_SHIP_ARRIVAL: {
            auto ship = event->getShip();
            if (ship) {
                queues_[ship->cargo_type].push_back(ship);
            }
            break;
        }
        case types::EventType::ON_SHIP_IN_CRANE: {
            auto ship = event->getShip();
            auto crane = event->getCrane();
            if (!ship || !crane) {
                break;
            }
            auto craneIter = craneIndex_.find(crane);
            if (craneIter != craneIndex_.end()) {
                craneVisuals_[craneIter->second].currentShip = ship;
            }
            auto &queue = queues_[ship->cargo_type];
            auto shipIter =
                std::find_if(queue.begin(), queue.end(), [&](const std::shared_ptr<Ship> &candidate) {
                    return candidate.get() == ship.get();
                });
            if (shipIter != queue.end()) {
                queue.erase(shipIter);
            }
            break;
        }
        case types::EventType::ON_SHIP_DEPARTURE: {
            auto ship = event->getShip();
            auto crane = event->getCrane();
            if (!ship || !crane) {
                break;
            }
            auto craneIter = craneIndex_.find(crane);
            sf::Vector2f startPosition{};
            if (craneIter != craneIndex_.end()) {
                CraneVisual &visual = craneVisuals_[craneIter->second];
                startPosition = craneCenter(visual);
                visual.currentShip.reset();
            }
            DepartingShipVisual departing;
            departing.ship = ship;
            departing.position = startPosition;
            float angle = 0.35f + 0.07f * static_cast<float>(ship->getId() % 3);
            float speed = 120.f + 20.f * static_cast<float>(ship->getId() % 5);
            departing.velocity = {std::cos(angle) * speed, -std::sin(angle) * speed};
            departing.color = colorForCargo(ship->cargo_type);
            departing.lifetime = 4.5f;
            departingShips_.push_back(departing);
            break;
        }
    }
}

void PortGUI::updateDepartures(float dt) {
    if (paused_) {
        return;
    }
    
    for (auto &departing : departingShips_) {
        departing.elapsed += dt;
        departing.position += departing.velocity * dt;
        departing.position.y += dt * 6.f;
    }
    departingShips_.erase(std::remove_if(departingShips_.begin(), departingShips_.end(),
                                         [](const DepartingShipVisual &ship) {
                                             return ship.elapsed >= ship.lifetime;
                                         }),
                          departingShips_.end());
}

void PortGUI::drawFrame() {
    window_.clear(sf::Color(12, 22, 40));
    drawCranes(window_);
    drawQueues(window_);
    drawDepartingShips(window_);
    drawOverlay(window_);
    window_.display();
}

void PortGUI::drawCranes(sf::RenderTarget &target) {
    std::map<types::CargoType, std::size_t> busyCount;
    for (auto type : typeOrder_) {
        busyCount[type] = 0;
    }
    for (const auto &visual : craneVisuals_) {
        if (visual.currentShip) {
            busyCount[visual.type]++;
        }
    }

    for (auto type : typeOrder_) {
        const auto laneIt = laneMetrics_.find(type);
        if (laneIt == laneMetrics_.end()) {
            continue;
        }
        const LaneMetrics &lane = laneIt->second;
        sf::RectangleShape panel({lane.width, lane.craneHeight});
        panel.setPosition(sf::Vector2f{lane.left, lane.craneTop});
        panel.setFillColor(sf::Color(20, 34, 60, 180));
        panel.setOutlineThickness(1.5f);
        panel.setOutlineColor(sf::Color(50, 70, 90, 220));
        target.draw(panel);

        std::ostringstream label;
        const auto totalConfigured = typeTotals_.contains(type) ? typeTotals_.at(type) : 0;
        const auto totalActive = actualTypeCounts_.contains(type) ? actualTypeCounts_.at(type) : 0;
        label << types::cargoToString(type) << " cranes  busy " << busyCount[type] << "/" << totalActive
              << "  configured " << totalConfigured;
        sf::Text labelText = makeText(label.str(), 20);
        labelText.setFillColor(sf::Color::White);
        labelText.setPosition(sf::Vector2f{lane.left + 6.f, lane.craneTop + 2.f});
        target.draw(labelText);
    }

    for (const CraneVisual &visual : craneVisuals_) {
        sf::RectangleShape craneRect(visual.size);
        craneRect.setPosition(visual.position);
        auto craneColor = colorForCargo(visual.type, 0.35f);
        craneRect.setFillColor(craneColor);
        craneRect.setOutlineThickness(2.f);
        craneRect.setOutlineColor(colorForCargo(visual.type, 0.85f));
        target.draw(craneRect);

        sf::Text craneIdText = makeText("#" + std::to_string((visual.orderIndex % kMaxCraneRows) + 1 +
                                                             static_cast<int>(visual.orderIndex / kMaxCraneRows) *
                                                                 static_cast<int>(kMaxCraneRows)),
                                        14);
        craneIdText.setPosition(sf::Vector2f{visual.position.x + 6.f, visual.position.y - 18.f});
        craneIdText.setFillColor(sf::Color(210, 220, 235));
        target.draw(craneIdText);

        if (visual.currentShip) {
            sf::RectangleShape shipRect({visual.size.x - 16.f, visual.size.y - 18.f});
            shipRect.setPosition(sf::Vector2f{visual.position.x + 8.f, visual.position.y + 9.f});
            auto shipColor = colorForCargo(visual.currentShip->cargo_type, 0.95f);
            shipRect.setFillColor(shipColor);
            shipRect.setOutlineColor(sf::Color::White);
            shipRect.setOutlineThickness(1.2f);
            target.draw(shipRect);

            std::string shipName = visual.currentShip->getName();
            if (shipName.size() > 14) {
                shipName = shipName.substr(0, 13);
                shipName.erase(std::remove(shipName.begin(), shipName.end(), ' '), shipName.end());
                shipName += "...";
            }
            sf::Text shipText = makeText(shipName, 14);
            shipText.setFillColor(sf::Color::Black);
            shipText.setPosition(sf::Vector2f{shipRect.getPosition().x + 6.f,
                                              shipRect.getPosition().y + 8.f});
            target.draw(shipText);
        }
    }
}

void PortGUI::drawQueues(sf::RenderTarget &target) {
    const std::size_t maxSlotsPerRow = 5;
    const float slotWidth = 78.f;
    const float slotHeight = 28.f;
    const float slotSpacingX = 12.f;
    const float slotSpacingY = 12.f;

    for (auto type : typeOrder_) {
        const auto laneIt = laneMetrics_.find(type);
        if (laneIt == laneMetrics_.end()) {
            continue;
        }
        const LaneMetrics &lane = laneIt->second;
        sf::RectangleShape queuePanel({lane.width, lane.queueHeight});
        queuePanel.setPosition(sf::Vector2f{lane.left, lane.queueTop});
        queuePanel.setFillColor(sf::Color(16, 28, 48, 200));
        queuePanel.setOutlineColor(sf::Color(45, 60, 80, 220));
        queuePanel.setOutlineThickness(1.5f);
        target.draw(queuePanel);

        const auto queueSize = queues_[type].size();
        std::ostringstream queueLabel;
        queueLabel << "Queue: " << queueSize << " ship" << (queueSize == 1 ? "" : "s");
        sf::Text queueText = makeText(queueLabel.str(), 18);
        queueText.setFillColor(sf::Color(220, 230, 245));
        queueText.setPosition(sf::Vector2f{lane.left + 6.f, lane.queueTop + 6.f});
        target.draw(queueText);

        const auto &queue = queues_[type];
        for (std::size_t i = 0; i < queue.size() && i < maxSlotsPerRow * 3; ++i) {
            const std::size_t row = i / maxSlotsPerRow;
            const std::size_t column = i % maxSlotsPerRow;
            float x = lane.left + 18.f + column * (slotWidth + slotSpacingX);
            float y = lane.queueTop + 34.f + row * (slotHeight + slotSpacingY);
            sf::RectangleShape slot({slotWidth, slotHeight});
            slot.setPosition(sf::Vector2f{x, y});
            auto color = colorForCargo(type, 0.8f);
            slot.setFillColor(color);
            slot.setOutlineThickness(1.f);
            slot.setOutlineColor(sf::Color(230, 234, 240));
            target.draw(slot);

            std::string label = queue[i]->getName();
            if (label.size() > 8) {
                label = label.substr(0, 7);
                label.erase(std::remove(label.begin(), label.end(), ' '), label.end());
                label += "...";
            }
            sf::Text slotText = makeText(label, 13);
            slotText.setFillColor(sf::Color::Black);
            slotText.setPosition(sf::Vector2f{x + 4.f, y + 4.f});
            target.draw(slotText);
        }
    }
}

void PortGUI::drawDepartingShips(sf::RenderTarget &target) {
    for (const auto &ship : departingShips_) {
        float lifeRatio = clampUnit(ship.elapsed / ship.lifetime);
        float alpha = 1.f - lifeRatio;
        sf::RectangleShape hull({78.f, 24.f});
        hull.setPosition(ship.position);
        auto color = ship.color;
        color.a = static_cast<std::uint8_t>(alpha * 255);
        hull.setFillColor(color);
        hull.setOutlineColor(sf::Color(255, 255, 255,
                                       static_cast<std::uint8_t>(alpha * 200.f)));
        hull.setOutlineThickness(1.4f);
        target.draw(hull);

        if (ship.ship) {
            std::string label = ship.ship->getName();
            
            sf::Text shipText = makeText(label, 14);
            shipText.setFillColor(
                sf::Color(240, 240, 240, static_cast<std::uint8_t>(alpha * 255)));
            shipText.setPosition(sf::Vector2f{ship.position.x + 6.f, ship.position.y - 18.f});
            target.draw(shipText);
        }
    }
}

void PortGUI::drawOverlay(sf::RenderTarget &target) {
    sf::Text title = makeText("Port operation visualizer", 26);
    title.setPosition(sf::Vector2f{horizontalMargin_, 10.f});
    target.draw(title);

    auto ticks = static_cast<types::time_t>(simTime_);
    std::size_t day = ticks / 1440 + 1;
    std::size_t minutes = ticks % 1440;
    std::size_t hours = minutes / 60;
    std::size_t minute = minutes % 60;

    std::ostringstream timeStream;
    timeStream << "Sim day " << day << "  "
               << std::setw(2) << std::setfill('0') << hours << ":" << std::setw(2) << minute;
    sf::Text timeText = makeText(timeStream.str(), 20);
    timeText.setPosition(sf::Vector2f{horizontalMargin_, 44.f});
    target.draw(timeText);

    std::ostringstream speedStream;
    speedStream << "Speed: " << static_cast<int>(timeScale_) << " ticks/sec";
    sf::Text speedText = makeText(speedStream.str(), 20);
    speedText.setPosition(sf::Vector2f{horizontalMargin_ + 280.f, 44.f});
    target.draw(speedText);

    const auto stats = Statistics::getInstance()->getStatistics();
    const auto statValue = [&](const std::string &key) -> std::int64_t {
        const auto it = stats.find(key);
        return it != stats.end() ? static_cast<std::int64_t>(it->second) : 0;
    };

    const float statsWidth = 380.f;
    const float statsHeight = 56.f;
    const float statsLeft = windowWidth_ - horizontalMargin_ - statsWidth;
    const float statsTop = 10.f;

    sf::RectangleShape statsPanel({statsWidth, statsHeight});
    statsPanel.setPosition(sf::Vector2f{statsLeft, statsTop});
    statsPanel.setFillColor(sf::Color(16, 28, 48, 190));
    statsPanel.setOutlineColor(sf::Color(45, 60, 80, 220));
    statsPanel.setOutlineThickness(1.4f);
    target.draw(statsPanel);

    std::ostringstream cargoStats;
    cargoStats << "Ships: " << statValue("SHIPS") << "    Cargo: " << statValue("MASS") << " t";
    sf::Text cargoText = makeText(cargoStats.str(), 16);
    cargoText.setPosition(sf::Vector2f{statsLeft + 10.f, statsTop + 12.f});
    cargoText.setFillColor(sf::Color(220, 230, 245));
    target.draw(cargoText);

    std::ostringstream fineStats;
    fineStats << "Penalty time: " << statValue("FINE") << " min";
    sf::Text fineText = makeText(fineStats.str(), 16);
    fineText.setPosition(sf::Vector2f{statsLeft + 10.f, statsTop + 32.f});
    fineText.setFillColor(sf::Color(220, 230, 245));
    target.draw(fineText);

    sf::Text instructionText =
        makeText("Space: pause/resume   Up/Down: change speed   Esc: exit", 16);
    instructionText.setPosition(
        sf::Vector2f{horizontalMargin_, windowHeight_ - verticalMargin_ - 24.f});
    instructionText.setFillColor(sf::Color(210, 220, 235));
    target.draw(instructionText);

    float progress = targetTicks_ > 0 ? clampUnit(simTime_ / static_cast<float>(targetTicks_)) : 0.f;
    sf::RectangleShape progressBg({windowWidth_ - 2.f * horizontalMargin_, 10.f});
    progressBg.setPosition(sf::Vector2f{horizontalMargin_, topBarHeight_ - 16.f});
    progressBg.setFillColor(sf::Color(35, 45, 70));
    target.draw(progressBg);

    sf::RectangleShape progressFill({progressBg.getSize().x * progress, 10.f});
    progressFill.setPosition(progressBg.getPosition());
    progressFill.setFillColor(sf::Color(120, 200, 255));
    target.draw(progressFill);

    if (paused_) {
        sf::RectangleShape overlay({windowWidth_, windowHeight_});
        overlay.setFillColor(sf::Color(0, 0, 0, 120));
        target.draw(overlay);
        sf::Text pausedText = makeText("PAUSED", 48);
        const auto bounds = pausedText.getGlobalBounds();
        pausedText.setPosition(
            sf::Vector2f{windowWidth_ / 2.f - bounds.size.x / 2.f,
                         windowHeight_ / 2.f - bounds.size.y / 2.f});
        target.draw(pausedText);
    }
}

sf::Color PortGUI::colorForCargo(types::CargoType type, float alpha) const {
    alpha = clampUnit(alpha);
    sf::Color base;
    switch (type) {
        case types::CargoType::BULK:
            base = sf::Color(210, 170, 90);
            break;
        case types::CargoType::LIQUID:
            base = sf::Color(90, 170, 230);
            break;
        case types::CargoType::CONTAINER:
            base = sf::Color(120, 210, 150);
            break;
    }
    base.a = static_cast<std::uint8_t>(alpha * 255);
    return base;
}

sf::Text PortGUI::makeText(const std::string &text, unsigned size) const {
    sf::Text drawable(font_, text, size);
    drawable.setFillColor(sf::Color::White);
    drawable.setOutlineColor(sf::Color::Black);
    drawable.setOutlineThickness(0.4f);
    return drawable;
}

sf::Vector2f PortGUI::craneCenter(const CraneVisual &visual) const {
    return {visual.position.x + visual.size.x * 0.5f, visual.position.y + visual.size.y * 0.5f};
}

void PortGUI::adjustTimeScale(int direction) {
    if (direction == 0) {
        return;
    }
    const float delta = direction > 0 ? 60.f : -60.f;
    timeScale_ = std::clamp(timeScale_ + delta, 30.f, 1200.f);
}
