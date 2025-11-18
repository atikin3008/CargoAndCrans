#pragma once

#include "Event.h"
#include "Port.h"
#include "Settings.h"
#include "Ship.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace gui {

enum class AnimationPhase {
    Arrival,
    Docking,
    Unloading,
    Departure
};

class AnimationPosition {
  public:
    AnimationPosition(types::time_t timestamp,
                      std::uint16_t subframe,
                      types::EventType type,
                      std::shared_ptr<Event> eventPtr);

    types::time_t timestamp() const noexcept { return timestamp_; }
    std::uint16_t subframe() const noexcept { return subframe_; }
    types::EventType eventType() const noexcept { return eventType_; }
    AnimationPhase phase() const noexcept { return phase_; }
    const std::shared_ptr<Event> &event() const noexcept { return event_; }

    void setCoordinates(int x, int y) noexcept;
    std::pair<int, int> coordinates() const noexcept { return {x_, y_}; }

    void markForUnloading(std::uint16_t frames) noexcept;
    bool hasUnloadPlan() const noexcept { return unloadFrames_ > 0; }
    std::uint16_t unloadFrames() const noexcept { return unloadFrames_; }

  private:
    types::time_t timestamp_;
    std::uint16_t subframe_;
    types::EventType eventType_;
    std::shared_ptr<Event> event_;
    AnimationPhase phase_;
    int x_{0};
    int y_{0};
    std::uint16_t unloadFrames_{0};
};

struct Coordinate {
    int x{0};
    int y{0};
};

struct LaneLayout {
    Coordinate arrivalEntry;
    Coordinate dockPoint;
    Coordinate departureExit;
};

struct AnimationLayout {
    types::time_t arrivalDuration{1};
    types::time_t unloadingDuration{1};
    types::time_t departureDuration{1};
    std::vector<LaneLayout> lanes;
};

class GUI {
  public:
    explicit GUI(const std::string &settingsFilename);
    void generateAnimations(Port &port);

    const std::vector<std::shared_ptr<AnimationPosition>> &animations() const noexcept { return timeline_; }
    std::vector<std::shared_ptr<const AnimationPosition>> stateAt(types::time_t moment,
                                                                  std::uint16_t subframe = 0) const;
    types::time_t simulationTicks() const noexcept { return simulationTicks_; }
    std::uint16_t framesPerTick() const noexcept { return framesPerTick_; }
    std::size_t frameCount() const { return totalFrameCount(); }
    const AnimationLayout &layout() const noexcept { return layout_; }

  private:
    types::time_t readSimulationTicks() const;
    std::uint16_t readUnloadFrames() const;
    std::uint16_t readFramesPerTick() const;
    void appendAnimationsForTime(Port &port, types::time_t timePoint);
    AnimationLayout loadLayoutFromSettings() const;
    Coordinate readCoordinateNode(const SettingsNode &node, const std::string &name) const;
    types::time_t readTicksValue(const SettingsNode &node, const std::string &name) const;
    void createTravelFrames(const Coordinate &from,
                            const Coordinate &to,
                            types::time_t startTick,
                            types::time_t duration,
                            types::EventType eventType,
                            const std::shared_ptr<Event> &event);
    void createHoldFrames(const Coordinate &point,
                          types::time_t startTick,
                          types::time_t duration,
                          const std::shared_ptr<Event> &event);
    void registerFrame(std::size_t frameIndex, std::shared_ptr<AnimationPosition> frame);
    std::size_t totalFrameCount() const;
    std::size_t clampFrameIndex(types::time_t tick, std::uint16_t subframe) const;
    void resetLaneCursors();
    const LaneLayout &laneForIndex(std::size_t laneIndex) const;

    Settings settings_;
    types::time_t simulationTicks_{0};
    std::uint16_t unloadFramesPerEvent_{0};
    std::uint16_t framesPerTick_{1};
    AnimationLayout layout_;
    std::vector<std::shared_ptr<AnimationPosition>> timeline_;
    std::vector<std::vector<std::weak_ptr<AnimationPosition>>> framesByFrame_;
    std::size_t nextArrivalLane_{0};
    std::size_t nextDockLane_{0};
    std::size_t nextDepartureLane_{0};
};

} // namespace gui
