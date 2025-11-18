#pragma once

#include "Port.h"
#include "Settings.h"
#include <SFML/Graphics.hpp>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class PortGUI {
  public:
    PortGUI(Port &port, const std::string &settingsFile, const std::string &fontPath);

    void run();

  private:
    struct CraneVisual {
        const Crane *crane = nullptr;
        types::CargoType type{};
        std::size_t orderIndex = 0;
        sf::Vector2f position{};
        sf::Vector2f size{};
        std::shared_ptr<Ship> currentShip;
    };

    struct DepartingShipVisual {
        std::shared_ptr<Ship> ship;
        sf::Vector2f position{};
        sf::Vector2f velocity{};
        sf::Color color{};
        float lifetime = 4.f;
        float elapsed = 0.f;
    };

    struct LaneMetrics {
        float left = 0.f;
        float width = 0.f;
        float craneTop = 0.f;
        float craneHeight = 0.f;
        float queueTop = 0.f;
        float queueHeight = 0.f;
    };

    void buildLayout();
    void handleWindowEvents();
    void processSimulation(float dt);
    void handleSimulationEvent(const std::shared_ptr<Event> &event);
    void updateDepartures(float dt);
    void drawFrame();
    void drawCranes(sf::RenderTarget &target);
    void drawQueues(sf::RenderTarget &target);
    void drawDepartingShips(sf::RenderTarget &target);
    void drawOverlay(sf::RenderTarget &target);
    sf::Color colorForCargo(types::CargoType type, float alpha = 1.f) const;
    std::string cargoToString(types::CargoType type) const;
    sf::Text makeText(const std::string &text, unsigned size) const;
    sf::Vector2f craneCenter(const CraneVisual &visual) const;
    void adjustTimeScale(int direction);

    const float windowWidth_ = 1600.f;
    const float windowHeight_ = 900.f;
    const float horizontalMargin_ = 40.f;
    const float verticalMargin_ = 30.f;
    const float columnSpacing_ = 42.f;
    const float topBarHeight_ = 84.f;
    const float queueHeight_ = 160.f;
    static constexpr std::size_t kMaxCraneRows = 8;

    Port &port_;
    Settings settings_;
    sf::RenderWindow window_;
    sf::Font font_;
    std::vector<std::shared_ptr<Event>> events_;
    std::vector<CraneVisual> craneVisuals_;
    std::map<types::CargoType, LaneMetrics> laneMetrics_;
    std::map<types::CargoType, std::deque<std::shared_ptr<Ship>>> queues_;
    std::unordered_map<const Crane *, std::size_t> craneIndex_;
    std::vector<DepartingShipVisual> departingShips_;
    std::map<types::CargoType, std::size_t> typeTotals_;
    std::map<types::CargoType, std::size_t> actualTypeCounts_;
    std::vector<types::CargoType> typeOrder_;

    float simTime_ = 0.f;
    std::size_t nextEventIndex_ = 0;
    float timeScale_ = 240.f;
    bool paused_ = false;
    types::time_t targetTicks_ = 0;
};
