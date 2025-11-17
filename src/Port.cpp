#include "../include/Port.h"
#include "../include/Types.h"
#include<iostream>

Port::Port(std::string settingsFilename, std::string scheduleFilename) : settings(settingsFilename),
                                                                         schedule(scheduleFilename) {
    auto cransAmount = settings.get("crans_amount");
    for (auto &it: cransAmount.as<SettingsNode::object_t>()) {
        for (int64_t cranIndex = 0; cranIndex < it.second.as<int64_t>(); ++cranIndex) {
            crans.emplace_back(new Cran(types::getTypeByString(it.first)));
        }
    }
}


void Port::process() {
    types::time_t ticksAmount = settings.get("ticks_amount").as<int64_t>();
    for (int tick = 0; tick < ticksAmount; ++tick) {
        for (auto &it: schedule.getEvents(tick)) {
            shipsInOrder.push_back(it.ship);
            eventLog.pushEvent(std::make_shared<ArrivalEvent>(it.ship, tick));
        }
        for (auto &it2: crans) {
            if (it2->shipInCran() && !it2->isBusy(tick)){
                eventLog.pushEvent(std::make_shared<DepatureEvent>(it2->getShip(), it2, tick));
                it2->addShip(nullptr, -1e9);
            }
        }
        for (int shipIndex = 0; shipIndex < shipsInOrder.size(); ++shipIndex) {
            for (auto &it2: crans) {
                if (!it2->isBusy(tick) && it2->getType() == shipsInOrder[shipIndex]->cargo_type) {
                    it2->addShip(shipsInOrder[shipIndex], tick);
                    eventLog.pushEvent(std::make_shared<InCranEvent>(shipsInOrder[shipIndex], it2, tick));
                    shipsInOrder.erase(shipsInOrder.begin() + shipIndex);
                    --shipIndex;
                    break;
                }
            }
        }
    }
}

std::vector<std::shared_ptr<Event>> Port::get(types::time_t time){
    return eventLog.getEvents(time);
}

std::vector<std::shared_ptr<Event>> Port::get(){
    return eventLog.getEvents();
}

