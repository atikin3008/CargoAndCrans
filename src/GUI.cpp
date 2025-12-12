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
        targetTicks_ = std::max(targetTicks_, events_.back()->getTime()) + 10;
    }

    for (auto type : typeOrder_) {
        queues_[type] = {};
    }

    buildLayout();
}

void PortGUI::run() {
    sf::Clock clock;
    while (window_.isOpen()) {
        rebuildControlButtons();
        handleWindowEvents();
        float dt = clock.restart().asSeconds();
        dt = std::min(dt, 0.1f);
        processSimulation(dt);
        updateDepartures(dt);
        rebuildControlButtons();
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
    const float craneHeight = windowHeight_ - craneTop - queueHeight_ - verticalMargin_ - 100;
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
                    if (showFinalStats_) {
                        seekTo(0);
                        paused_ = false;
                        finished_ = false;
                        showFinalStats_ = false;
                    } else {
                        paused_ = !paused_;
                    }
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

        if (const auto *mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mousePressed->button == sf::Mouse::Button::Left) {
                handleControlClick(sf::Vector2f{static_cast<float>(mousePressed->position.x),
                                                static_cast<float>(mousePressed->position.y)});
            }
        }
    }
}

void PortGUI::processSimulation(float dt) {
    if (finished_) {
        simTime_ = static_cast<float>(targetTicks_);
        return;
    }

    if (paused_) {
        simTime_ = std::min(simTime_, static_cast<float>(targetTicks_));
        return;
    }

    simTime_ = std::min(simTime_ + dt * timeScale_, static_cast<float>(targetTicks_));

    if (events_.empty()) {
        if (simTime_ >= static_cast<float>(targetTicks_)) {
            finished_ = true;
            paused_ = true;
            showFinalStats_ = true;
        }
        return;
    }

    while (nextEventIndex_ < events_.size() && events_[nextEventIndex_]->getTime() <= simTime_) {
        handleSimulationEvent(events_[nextEventIndex_], simTime_, false);
        ++nextEventIndex_;
    }

    if (simTime_ >= static_cast<float>(targetTicks_) && nextEventIndex_ >= events_.size()) {
        finished_ = true;
        paused_ = true;
        showFinalStats_ = true;
        simTime_ = static_cast<float>(targetTicks_);
    }
}

void PortGUI::handleSimulationEvent(const std::shared_ptr<Event> &event, float simNow, bool catchUp) {
    if (!event) {
        return;
    }
    const float elapsedSinceEvent = catchUp ? std::max(0.f, simNow - static_cast<float>(event->getTime())) : 0.f;
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
            if (catchUp && elapsedSinceEvent >= departing.lifetime) {
                break;
            }
            departing.elapsed = catchUp ? std::min(elapsedSinceEvent, departing.lifetime) : 0.f;
            departing.position += departing.velocity * departing.elapsed;
            departing.position.y += departing.elapsed * 6.f;
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

void PortGUI::resetVisualState() {
    for (auto &queue : queues_) {
        queue.second.clear();
    }
    for (auto &visual : craneVisuals_) {
        visual.currentShip.reset();
    }
    departingShips_.clear();
}

void PortGUI::seekTo(types::time_t newTime) {
    types::time_t clamped = std::min<types::time_t>(newTime, targetTicks_);
    simTime_ = static_cast<float>(clamped);
    nextEventIndex_ = 0;
    resetVisualState();
    for (; nextEventIndex_ < events_.size() && events_[nextEventIndex_]->getTime() <= clamped;
         ++nextEventIndex_) {
        handleSimulationEvent(events_[nextEventIndex_], simTime_, true);
    }
    paused_ = true;
    finished_ = (clamped >= targetTicks_) && (nextEventIndex_ >= events_.size());
    showFinalStats_ = finished_;
}

void PortGUI::seekBy(int64_t delta) {
    const int64_t current = static_cast<int64_t>(
        std::clamp(simTime_, 0.f, static_cast<float>(targetTicks_)));
    int64_t target = current + delta;
    if (target < 0) {
        target = 0;
    }
    if (target > static_cast<int64_t>(targetTicks_)) {
        target = static_cast<int64_t>(targetTicks_);
    }
    seekTo(static_cast<types::time_t>(target));
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

void PortGUI::rebuildControlButtons() {
    controlButtons_.clear();
    const std::size_t buttonCount = 7;
    const float totalWidth =
        buttonCount * controlButtonWidth_ + (buttonCount - 1) * controlButtonSpacing_;
    float x = (windowWidth_ - totalWidth) / 2.f;
    float y = windowHeight_ - verticalMargin_ - controlButtonHeight_ - 8.f + 10;

    auto addButton = [&](ButtonAction action, const std::string &label) {
        controlButtons_.push_back(ControlButton{
            sf::FloatRect({x, y}, {controlButtonWidth_, controlButtonHeight_}), label, action});
        x += controlButtonWidth_ + controlButtonSpacing_;
    };

    addButton(ButtonAction::Restart, "Restart");
    addButton(ButtonAction::BackDay, "-1 day");
    addButton(ButtonAction::BackHour, "-1 hour");
    addButton(ButtonAction::TogglePause, paused_ ? "Play" : "Pause");
    addButton(ButtonAction::ForwardHour, "+1 hour");
    addButton(ButtonAction::ForwardDay, "+1 day");
    addButton(ButtonAction::JumpToEnd, "To end");
}

void PortGUI::handleControlClick(sf::Vector2f mousePos) {
    for (const auto &button : controlButtons_) {
        if (!button.bounds.contains(mousePos)) {
            continue;
        }
        switch (button.action) {
            case ButtonAction::Restart:
                seekTo(0);
                finished_ = false;
                showFinalStats_ = false;
                paused_ = false;
                break;
            case ButtonAction::BackDay:
                showFinalStats_ = false;
                seekBy(-static_cast<int64_t>(types::MINS_IN_DAY));
                break;
            case ButtonAction::BackHour:
                showFinalStats_ = false;
                seekBy(-static_cast<int64_t>(types::MINS_IN_HOUR));
                break;
            case ButtonAction::TogglePause:
                if (finished_) {
                    seekTo(0);
                    finished_ = false;
                    showFinalStats_ = false;
                    paused_ = false;
                } else {
                    paused_ = !paused_;
                }
                break;
            case ButtonAction::ForwardHour:
                showFinalStats_ = false;
                seekBy(static_cast<int64_t>(types::MINS_IN_HOUR));
                break;
            case ButtonAction::ForwardDay:
                showFinalStats_ = false;
                seekBy(static_cast<int64_t>(types::MINS_IN_DAY));
                break;
            case ButtonAction::JumpToEnd:
                seekTo(targetTicks_);
                finished_ = true;
                showFinalStats_ = true;
                paused_ = true;
                break;
        }
        break;
    }
}

void PortGUI::drawControlButtons(sf::RenderTarget &target) {
    sf::Vector2i mousePixel = sf::Mouse::getPosition(window_);
    sf::Vector2f mousePos{static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y)};
    for (const auto &button : controlButtons_) {
        bool hovered = button.bounds.contains(mousePos);
        sf::RectangleShape rect({button.bounds.size.x, button.bounds.size.y});
        rect.setPosition(button.bounds.position);
        rect.setFillColor(hovered ? sf::Color(70, 110, 150, 220) : sf::Color(35, 70, 110, 190));
        rect.setOutlineThickness(1.6f);
        rect.setOutlineColor(sf::Color(200, 220, 240, hovered ? 230 : 170));
        target.draw(rect);

        sf::Text label = makeText(button.label, 16);
        const auto bounds = label.getGlobalBounds();
        label.setPosition(
            sf::Vector2f{button.bounds.position.x + (button.bounds.size.x - bounds.size.x) / 2.f,
                         button.bounds.position.y + (button.bounds.size.y - bounds.size.y) / 2.f - 4.f});
        label.setFillColor(sf::Color(235, 240, 245));
        target.draw(label);
    }
}

void PortGUI::drawFinalStatistics(sf::RenderTarget &target) {
    sf::RectangleShape overlay({windowWidth_, windowHeight_});
    overlay.setFillColor(sf::Color(8, 12, 24, 235));
    target.draw(overlay);

    sf::Text title = makeText("Simulation finished", 40);
    auto titleBounds = title.getGlobalBounds();
    title.setPosition(
        sf::Vector2f{windowWidth_ / 2.f - titleBounds.size.x / 2.f, 96.f});
    target.draw(title);

    const auto stats = Statistics::getInstance()->getStatistics();
    const auto statValue = [&](const std::string &key) -> std::int64_t {
        const auto it = stats.find(key);
        return it != stats.end() ? static_cast<std::int64_t>(it->second) : 0;
    };

    const std::int64_t ships = statValue("SHIPS");
    const std::int64_t cargo = statValue("MASS");
    const std::int64_t fineMinutes = statValue("FINE");
    const int fineCost = (fineMinutes / types::MINS_IN_DAY) * 2000;

    sf::RectangleShape panel(
        {windowWidth_ - 2.f * horizontalMargin_, 260.f});
    panel.setPosition(sf::Vector2f{horizontalMargin_, 200.f});
    panel.setFillColor(sf::Color(20, 34, 60, 230));
    panel.setOutlineColor(sf::Color(90, 130, 180, 240));
    panel.setOutlineThickness(2.f);
    target.draw(panel);

    sf::Text shipsText = makeText("Ships handled: " + std::to_string(ships), 28);
    shipsText.setPosition(sf::Vector2f{panel.getPosition().x + 24.f, panel.getPosition().y + 26.f});
    target.draw(shipsText);

    sf::Text cargoText = makeText("Cargo handled: " + std::to_string(cargo) + " t", 28);
    cargoText.setPosition(sf::Vector2f{panel.getPosition().x + 24.f, panel.getPosition().y + 86.f});
    target.draw(cargoText);

    std::ostringstream fineStream;
    fineStream << "Penalty time: " << fineMinutes << " min"
               << "   (~" << std::fixed << std::setprecision(2) << fineCost * -1 << " BTC)";
    sf::Text fineText = makeText(fineStream.str(), 28);
    fineText.setPosition(sf::Vector2f{panel.getPosition().x + 24.f, panel.getPosition().y + 146.f});
    target.draw(fineText);

    sf::Text hint = makeText("Use the buttons below to rewind or replay. Press Esc to exit.", 20);
    auto hintBounds = hint.getGlobalBounds();
    hint.setPosition(sf::Vector2f{windowWidth_ / 2.f - hintBounds.size.x / 2.f, panel.getPosition().y + 206.f});
    target.draw(hint);
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

    sf::Text stateText = makeText(paused_ ? "State: paused" : "State: playing", 18);
    stateText.setPosition(sf::Vector2f{horizontalMargin_ + 520.f, 44.f});
    target.draw(stateText);

    float progress = targetTicks_ > 0 ? clampUnit(simTime_ / static_cast<float>(targetTicks_)) : 0.f;
    sf::RectangleShape progressBg({windowWidth_ - 2.f * horizontalMargin_, 10.f});
    progressBg.setPosition(sf::Vector2f{horizontalMargin_, topBarHeight_ - 16.f});
    progressBg.setFillColor(sf::Color(35, 45, 70));
    target.draw(progressBg);

    sf::RectangleShape progressFill({progressBg.getSize().x * progress, 10.f});
    progressFill.setPosition(progressBg.getPosition());
    progressFill.setFillColor(sf::Color(120, 200, 255));
    target.draw(progressFill);

    sf::Text instructionText = makeText(
        "Space: pause/resume   Up/Down: speed   Esc: exit   Buttons: rewind/forward", 16);
    instructionText.setPosition(
        sf::Vector2f{horizontalMargin_, windowHeight_ - verticalMargin_ - controlButtonHeight_ - 24.f});
    instructionText.setFillColor(sf::Color(210, 220, 235));
    target.draw(instructionText);

    if (showFinalStats_) {
        drawFinalStatistics(target);
        drawControlButtons(target);
        return;
    }

    drawControlButtons(target);

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
    timeScale_ = std::clamp(timeScale_ + delta, 30.f, 120000.f);
}
