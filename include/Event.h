#pragma once

#include"Types.h"
#include"Ship.h"
#include"Crane.h"
#include<memory>
#include <utility>


class Event {
 public:
    Event(types::time_t time) : time_(time) {}

    virtual types::EventType getType() = 0;

    types::time_t getTime();

    bool operator>(types::time_t time) const;

    bool operator==(types::time_t time) const;

    bool operator<(types::time_t time) const;

    virtual ~Event() = default;

 private:
    types::time_t time_;

};

class ArrivalEvent : public Event {
 public:
    ArrivalEvent(std::shared_ptr<Ship> ship, types::time_t time) : ship_(std::move(ship)), Event(time) {}

    types::EventType getType() override {
        return types::EventType::ON_SHIP_ARRIVAL;
    }

 private:
    std::shared_ptr<Ship> ship_;
};

class InCraneEvent : public Event {
 public:
    InCraneEvent(std::shared_ptr<Ship> ship, std::shared_ptr<Crane> crane, types::time_t time) : ship_(std::move(ship)),
                                                                                              crane_(std::move(crane)),
                                                                                              Event(time) {}

    types::EventType getType() override {
        return types::EventType::ON_SHIP_IN_CRANE;
    }

 private:
    std::shared_ptr<Ship> ship_;
    std::shared_ptr<Crane> crane_;
};


class DepartureEvent : public Event {
 public:
    DepartureEvent(std::shared_ptr<Ship> ship, std::shared_ptr<Crane> crane, types::time_t time) : ship_(std::move(ship)),
                                                                                                crane_(std::move(crane)),
                                                                                                Event(time) {}

    types::EventType getType() override {
        return types::EventType::ON_SHIP_DEPARTURE;
    }

 private:
    std::shared_ptr<Ship> ship_;
    std::shared_ptr<Crane> crane_;
};