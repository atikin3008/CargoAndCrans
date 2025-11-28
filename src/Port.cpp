#include "../include/Port.h"
#include "../include/Types.h"
#include<iostream>
#include"../include/Statistics.h"

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
    types::time_t ticksAmount = settings.get("ticks_amount").as<int64_t>();
    for (int tick = 0; tick < ticksAmount; ++tick) {
        for (auto &it: schedule.getEvents(tick)) {
            shipsInOrder.push_back(it);
            eventLog.pushEvent(std::make_shared<ArrivalEvent>(it.ship, tick));
        }
        for (auto &it2: cranes) {
            if (it2->shipInCrane() && !it2->isBusy(tick)){
                eventLog.pushEvent(std::make_shared<DepartureEvent>(it2->getShip(), it2, tick));
                Statistics::getInstance()->addMass(it2->getShip()->cargo_weight_tonnes);
                Statistics::getInstance()->addShip();

                it2->addShip({}, -1);
            }
        }
        for (int shipIndex = 0; shipIndex < shipsInOrder.size(); ++shipIndex) {
            for (auto &it2: cranes) {
                if (!it2->isBusy(tick) && it2->getType() == shipsInOrder[shipIndex].ship->cargo_type) {
                    it2->addShip(shipsInOrder[shipIndex], tick);
                    eventLog.pushEvent(std::make_shared<InCraneEvent>(shipsInOrder[shipIndex].ship, it2, tick));
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

const Settings &Port::getSettings() const{
    return settings;
}

const std::vector<std::shared_ptr<Crane>> &Port::getCranes() const{
    return cranes;
}
