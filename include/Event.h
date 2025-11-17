#pragma once

#include"Types.h"
#include"Ship.h"
#include"Cran.h"
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

class InCranEvent : public Event {
 public:
    InCranEvent(std::shared_ptr<Ship> ship, std::shared_ptr<Cran> cran, types::time_t time) : ship_(std::move(ship)),
                                                                                              cran_(std::move(cran)),
                                                                                              Event(time) {}

    types::EventType getType() override {
        return types::EventType::ON_SHIP_IN_CRAN;
    }

 private:
    std::shared_ptr<Ship> ship_;
    std::shared_ptr<Cran> cran_;
};


class DepatureEvent : public Event {
 public:
    DepatureEvent(std::shared_ptr<Ship> ship, std::shared_ptr<Cran> cran, types::time_t time) : ship_(std::move(ship)),
                                                                                                cran_(std::move(cran)),
                                                                                                Event(time) {}

    types::EventType getType() override {
        return types::EventType::ON_SHIP_DEPATURE;
    }

 private:
    std::shared_ptr<Ship> ship_;
    std::shared_ptr<Cran> cran_;
};