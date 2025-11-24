#pragma once

#include "Types.h"
#include "Ship.h"
#include<memory>
#include "Deviations.h"
#include"Schedule.h"


class Crane {
 public:
    Crane(types::CargoType cargoType);
    void addShip(const schedule::ScheduleEvent & event, types::time_t time);
    bool isBusy(types::time_t time);
    std::shared_ptr<Ship> getShip();
    types::CargoType getType();
    bool shipInCrane();

 private:
    types::CargoType cargoType_;
    std::shared_ptr<Ship> ship_;
    types::time_t end_;
};

