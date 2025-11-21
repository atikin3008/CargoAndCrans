#include "../include/Port.h"
#include "../include/Types.h"
#include <algorithm>
#include <iostream>

Port::Port(std::string settingsFilename, std::string scheduleFilename) : settings(settingsFilename),
                                                                         schedule(settingsFilename, scheduleFilename) {
    auto cranesAmount = settings.get("cranes_amount");
    for (auto &it: cranesAmount.as<SettingsNode::object_t>()) {
        for (int64_t craneIndex = 0; craneIndex < it.second.as<int64_t>(); ++craneIndex) {
            cranes.emplace_back(new Crane(types::getTypeByString(it.first)));
        }
    }
}


void Port::process() {
    const auto allScheduleEvents = schedule.getEvents();
    const types::time_t lastArrival =
        allScheduleEvents.empty() ? 0 : allScheduleEvents.back().arrival_time;

    types::time_t ticksAmount =
        std::max<int64_t>(settings.get("ticks_amount").as<int64_t>(), lastArrival) + 10;

    auto processTick = [&](types::time_t tick) {
        for (auto &event : schedule.getEvents(tick)) {
            shipsInOrder.push_back(event.ship);
            eventLog.pushEvent(std::make_shared<ArrivalEvent>(event.ship, tick));
        }

        for (auto &crane : cranes) {
            if (crane->shipInCrane() && !crane->isBusy(tick)) {
                eventLog.pushEvent(std::make_shared<DepartureEvent>(crane->getShip(), crane, tick));
                crane->addShip(nullptr, 0);
            }
        }

        for (int shipIndex = 0; shipIndex < static_cast<int>(shipsInOrder.size()); ++shipIndex) {
            for (auto &crane : cranes) {
                if (!crane->isBusy(tick) && crane->getType() == shipsInOrder[shipIndex]->cargo_type) {
                    crane->addShip(shipsInOrder[shipIndex], tick);
                    eventLog.pushEvent(std::make_shared<InCraneEvent>(shipsInOrder[shipIndex], crane, tick));
                    shipsInOrder.erase(shipsInOrder.begin() + shipIndex);
                    --shipIndex;
                    break;
                }
            }
        }
    };

    types::time_t tick = 0;
    for (; tick < ticksAmount; ++tick) {
        processTick(tick);
    }

    auto pendingWork = [&]() {
        if (!shipsInOrder.empty()) {
            return true;
        }
        return std::any_of(cranes.begin(), cranes.end(),
                           [](const std::shared_ptr<Crane> &crane) { return crane->shipInCrane(); });
    };

    while (pendingWork()) {
        processTick(tick++);
    }
}

std::vector<std::shared_ptr<Event>> Port::get(types::time_t time){
    return eventLog.getEvents(time);
}

std::vector<std::shared_ptr<Event>> Port::get(){
    return eventLog.getEvents();
}

const Settings &Port::getSettings() const{
    return settings;
}

const std::vector<std::shared_ptr<Crane>> &Port::getCranes() const{
    return cranes;
}
