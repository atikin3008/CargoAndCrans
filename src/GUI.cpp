#include "GUI.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

constexpr const char *kSimulationTicksKey = "simulation_ticks";
constexpr const char *kUnloadFramesKey = "unload_animation_frames";
constexpr const char *kFramesPerTickKey = "animation_frames_per_tick";
constexpr const char *kAnimationKey = "animation";
constexpr const char *kCoordinatesKey = "coordinates";
constexpr const char *kDurationsKey = "durations";
constexpr const char *kLanesKey = "lanes";
constexpr const char *kArrivalEntryKey = "arrival_entry";
constexpr const char *kDockKey = "dock";
constexpr const char *kDepartureExitKey = "departure_exit";
constexpr const char *kArrivalDurationKey = "arrival_travel_ticks";
constexpr const char *kUnloadingDurationKey = "unloading_ticks";
constexpr const char *kDepartureDurationKey = "departure_travel_ticks";

template <typename Target>
Target clampToUnsigned(std::int64_t value, const char *keyName) {
    if (value < 0) {
        throw std::runtime_error(std::string("Settings value for '") + keyName + "' must be non-negative");
    }
    if (static_cast<unsigned long long>(value) > std::numeric_limits<Target>::max()) {
        throw std::runtime_error(std::string("Settings value for '") + keyName + "' exceeds supported range");
    }
    return static_cast<Target>(value);
}

AnimationPhase detectPhase(types::EventType type) {
    switch (type) {
        case types::EventType::ON_SHIP_ARRIVAL:
            return AnimationPhase::Arrival;
        case types::EventType::ON_SHIP_IN_CRAN:
            return AnimationPhase::Docking;
        case types::EventType::ON_SHIP_DEPATURE:
            return AnimationPhase::Departure;
        default:
            return AnimationPhase::Arrival;
    }
}

} // namespace

namespace gui {

AnimationPosition::AnimationPosition(types::time_t timestamp,
                                     std::uint16_t subframe,
                                     types::EventType type,
                                     std::shared_ptr<Event> eventPtr)
    : timestamp_(timestamp),
      subframe_(subframe),
      eventType_(type),
      event_(std::move(eventPtr)),
      phase_(detectPhase(type)) {}

void AnimationPosition::setCoordinates(int x, int y) noexcept {
    x_ = x;
    y_ = y;
}

void AnimationPosition::markForUnloading(std::uint16_t frames) noexcept {
    unloadFrames_ = frames;
    if (frames > 0) {
        phase_ = AnimationPhase::Unloading;
    }
}

GUI::GUI(const std::string &settingsFilename) : settings_(settingsFilename) {
    simulationTicks_ = readSimulationTicks();
    unloadFramesPerEvent_ = readUnloadFrames();
    framesPerTick_ = readFramesPerTick();
    layout_ = loadLayoutFromSettings();
}

void GUI::generateAnimations(Port &port) {
    timeline_.clear();
    const std::size_t baseFrames = totalFrameCount();
    const std::size_t laneFactor = std::max<std::size_t>(1, layout_.lanes.size());
    timeline_.reserve(baseFrames ? baseFrames * laneFactor : laneFactor);
    framesByFrame_.assign(baseFrames, {});
    resetLaneCursors();

    for (types::time_t tick = 0; tick < simulationTicks_; ++tick) {
        appendAnimationsForTime(port, tick);
    }
}

types::time_t GUI::readSimulationTicks() const {
    const auto rawTicks = settings_.get<std::int64_t>(kSimulationTicksKey);
    return clampToUnsigned<types::time_t>(rawTicks, kSimulationTicksKey);
}

std::uint16_t GUI::readUnloadFrames() const {
    try {
        const auto rawFrames = settings_.get<std::int64_t>(kUnloadFramesKey);
        return clampToUnsigned<std::uint16_t>(rawFrames, kUnloadFramesKey);
    } catch (const std::exception &) {
        return 0;
    }
}

std::uint16_t GUI::readFramesPerTick() const {
    try {
        const auto raw = settings_.get<std::int64_t>(kFramesPerTickKey);
        const auto clamped = clampToUnsigned<std::uint16_t>(raw, kFramesPerTickKey);
        return static_cast<std::uint16_t>(std::max<std::uint16_t>(clamped, 1));
    } catch (const std::exception &) {
        return 1;
    }
}

void GUI::appendAnimationsForTime(Port &port, types::time_t timePoint) {
    auto events = port.get(timePoint);
    if (events.empty()) {
        return;
    }

    for (const auto &event : events) {
        if (!event) {
            continue;
        }

        switch (event->get_type()) {
            case types::EventType::ON_SHIP_ARRIVAL: {
                const auto &lane = laneForIndex(nextArrivalLane_++);
                createTravelFrames(lane.arrivalEntry,
                                   lane.dockPoint,
                                   timePoint,
                                   layout_.arrivalDuration,
                                   types::EventType::ON_SHIP_ARRIVAL,
                                   event);
                break;
            }
            case types::EventType::ON_SHIP_IN_CRAN: {
                const auto &lane = laneForIndex(nextDockLane_++);
                types::time_t duration = layout_.unloadingDuration;
                if (duration == 0) {
                    duration = static_cast<types::time_t>(std::max<std::uint16_t>(unloadFramesPerEvent_, 1));
                }
                createHoldFrames(lane.dockPoint, timePoint, duration, event);
                break;
            }
            case types::EventType::ON_SHIP_DEPATURE: {
                const auto &lane = laneForIndex(nextDepartureLane_++);
                createTravelFrames(lane.dockPoint,
                                   lane.departureExit,
                                   timePoint,
                                   layout_.departureDuration,
                                   types::EventType::ON_SHIP_DEPATURE,
                                   event);
                break;
            }
        }
    }
}

std::vector<std::shared_ptr<const AnimationPosition>>
GUI::stateAt(types::time_t moment, std::uint16_t subframe) const {
    std::vector<std::shared_ptr<const AnimationPosition>> result;
    if (framesPerTick_ == 0 || framesByFrame_.empty()) {
        return result;
    }

    const std::size_t index = clampFrameIndex(moment, subframe);
    if (index >= framesByFrame_.size()) {
        return result;
    }

    const auto &weakList = framesByFrame_[index];
    result.reserve(weakList.size());
    for (const auto &weakPtr : weakList) {
        if (auto locked = weakPtr.lock()) {
            result.emplace_back(std::const_pointer_cast<const AnimationPosition>(locked));
        }
    }

    return result;
}

AnimationLayout GUI::loadLayoutFromSettings() const {
    AnimationLayout layout;
    try {
        const auto &root = settings_.root();
        const auto &animationNode = root.at(kAnimationKey);
        const auto &durations = animationNode.at(kDurationsKey);

        layout.arrivalDuration =
            readTicksValue(durations.at(kArrivalDurationKey), std::string(kArrivalDurationKey));
        layout.unloadingDuration =
            readTicksValue(durations.at(kUnloadingDurationKey), std::string(kUnloadingDurationKey));
        layout.departureDuration =
            readTicksValue(durations.at(kDepartureDurationKey), std::string(kDepartureDurationKey));

        bool lanesLoaded = false;
        try {
            const auto lanesArray = animationNode.at(kLanesKey).as<SettingsNode::array_t>();
            for (std::size_t i = 0; i < lanesArray.size(); ++i) {
                const auto &laneNode = lanesArray[i];
                LaneLayout lane;
                const std::string baseName = "lanes[" + std::to_string(i) + "]";
                lane.arrivalEntry =
                    readCoordinateNode(laneNode.at(kArrivalEntryKey), baseName + ".arrival_entry");
                lane.dockPoint = readCoordinateNode(laneNode.at(kDockKey), baseName + ".dock");
                lane.departureExit =
                    readCoordinateNode(laneNode.at(kDepartureExitKey), baseName + ".departure_exit");
                layout.lanes.push_back(std::move(lane));
            }
            lanesLoaded = !layout.lanes.empty();
        } catch (const std::exception &) {
            lanesLoaded = false;
        }

        if (!lanesLoaded) {
            const auto &coords = animationNode.at(kCoordinatesKey);
            LaneLayout lane;
            lane.arrivalEntry = readCoordinateNode(coords.at(kArrivalEntryKey), std::string(kArrivalEntryKey));
            lane.dockPoint = readCoordinateNode(coords.at(kDockKey), std::string(kDockKey));
            lane.departureExit = readCoordinateNode(coords.at(kDepartureExitKey), std::string(kDepartureExitKey));
            layout.lanes.push_back(std::move(lane));
        }
    } catch (const std::exception &err) {
        throw std::runtime_error(std::string("Failed to load animation layout: ") + err.what());
    }

    if (layout.lanes.empty()) {
        layout.lanes.push_back({});
    }

    return layout;
}

Coordinate GUI::readCoordinateNode(const SettingsNode &node, const std::string &name) const {
    try {
        const auto xValue = node.at("x").as<std::int64_t>();
        const auto yValue = node.at("y").as<std::int64_t>();
        return Coordinate{static_cast<int>(xValue), static_cast<int>(yValue)};
    } catch (const std::exception &err) {
        throw std::runtime_error(std::string("Invalid coordinate '") + name + "': " + err.what());
    }
}

types::time_t GUI::readTicksValue(const SettingsNode &node, const std::string &name) const {
    const auto raw = node.as<std::int64_t>();
    if (raw <= 0) {
        throw std::runtime_error(std::string("Duration '") + name + "' must be positive");
    }
    return clampToUnsigned<types::time_t>(raw, name.c_str());
}

void GUI::createTravelFrames(const Coordinate &from,
                             const Coordinate &to,
                             types::time_t startTick,
                             types::time_t duration,
                             types::EventType eventType,
                             const std::shared_ptr<Event> &event) {
    if (duration == 0) {
        duration = 1;
    }

    const std::size_t startFrame = static_cast<std::size_t>(startTick) * framesPerTick_;
    if (startFrame >= framesByFrame_.size()) {
        return;
    }

    const std::size_t durationFrames =
        std::max<std::size_t>(1, static_cast<std::size_t>(duration) * framesPerTick_);

    for (std::size_t frameOffset = 0; frameOffset < durationFrames; ++frameOffset) {
        const std::size_t frameIndex = startFrame + frameOffset;
        if (frameIndex >= framesByFrame_.size()) {
            break;
        }

        const double factor =
            durationFrames == 1 ? 1.0 : static_cast<double>(frameOffset) / static_cast<double>(durationFrames - 1);
        const int x = static_cast<int>(std::lround(from.x + (to.x - from.x) * factor));
        const int y = static_cast<int>(std::lround(from.y + (to.y - from.y) * factor));

        const types::time_t tick = static_cast<types::time_t>(frameIndex / framesPerTick_);
        const std::uint16_t sub = static_cast<std::uint16_t>(frameIndex % framesPerTick_);

        auto frame = std::make_shared<AnimationPosition>(tick, sub, eventType, event);
        frame->setCoordinates(x, y);
        if (eventType == types::EventType::ON_SHIP_IN_CRAN && unloadFramesPerEvent_ > 0) {
            frame->markForUnloading(unloadFramesPerEvent_);
        }
        registerFrame(frameIndex, std::move(frame));
    }
}

void GUI::createHoldFrames(const Coordinate &point,
                           types::time_t startTick,
                           types::time_t duration,
                           const std::shared_ptr<Event> &event) {
    if (duration == 0) {
        duration = 1;
    }

    const std::size_t startFrame = static_cast<std::size_t>(startTick) * framesPerTick_;
    if (startFrame >= framesByFrame_.size()) {
        return;
    }

    const std::size_t durationFrames =
        std::max<std::size_t>(1, static_cast<std::size_t>(duration) * framesPerTick_);

    for (std::size_t frameOffset = 0; frameOffset < durationFrames; ++frameOffset) {
        const std::size_t frameIndex = startFrame + frameOffset;
        if (frameIndex >= framesByFrame_.size()) {
            break;
        }

        const types::time_t tick = static_cast<types::time_t>(frameIndex / framesPerTick_);
        const std::uint16_t sub = static_cast<std::uint16_t>(frameIndex % framesPerTick_);

        auto frame = std::make_shared<AnimationPosition>(tick, sub, event->get_type(), event);
        frame->setCoordinates(point.x, point.y);
        if (unloadFramesPerEvent_ > 0) {
            frame->markForUnloading(unloadFramesPerEvent_);
        }
        registerFrame(frameIndex, std::move(frame));
    }
}

void GUI::registerFrame(std::size_t frameIndex, std::shared_ptr<AnimationPosition> frame) {
    timeline_.push_back(frame);
    if (frameIndex >= framesByFrame_.size()) {
        framesByFrame_.resize(frameIndex + 1);
    }
    framesByFrame_[frameIndex].push_back(frame);
}

std::size_t GUI::totalFrameCount() const {
    if (framesPerTick_ == 0) {
        return 0;
    }
    return static_cast<std::size_t>(simulationTicks_) * framesPerTick_;
}

std::size_t GUI::clampFrameIndex(types::time_t tick, std::uint16_t subframe) const {
    if (framesPerTick_ == 0) {
        return 0;
    }
    const std::uint16_t maxSub = static_cast<std::uint16_t>(framesPerTick_ - 1);
    const std::uint16_t clamped = std::min<std::uint16_t>(subframe, maxSub);
    const std::size_t base = static_cast<std::size_t>(tick) * framesPerTick_;
    return base + clamped;
}

void GUI::resetLaneCursors() {
    nextArrivalLane_ = 0;
    nextDockLane_ = 0;
    nextDepartureLane_ = 0;
}

const LaneLayout &GUI::laneForIndex(std::size_t laneIndex) const {
    if (layout_.lanes.empty()) {
        static const LaneLayout fallback{};
        return fallback;
    }
    return layout_.lanes[laneIndex % layout_.lanes.size()];
}

} // namespace gui
