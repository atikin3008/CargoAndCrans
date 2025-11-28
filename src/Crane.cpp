#include "../include/Crane.h"
#include <cmath>
#include"../include/Statistics.h"

Crane::Crane(types::CargoType type) : cargoType_(type), ship_(nullptr), end_(0) {}

void Crane::addShip(const schedule::ScheduleEvent &event, types::time_t time) {
    if (time == -1) {
        ship_ = nullptr;
        end_ = 0;
        return;
    }
    if (event.ship->cargo_type != cargoType_)
        throw std::runtime_error("Types are not equal");
    if (end_ > time)
        throw std::runtime_error("Ship is not unloaded");
    ship_ = event.ship;
    end_ = time +
           (types::time_t) std::ceil(
                   event.ship->cargo_weight_tonnes / types::getSpeedOfUnloadCargo(event.ship->cargo_type)) +
           Deviations::GetInstance()->getDischargeDeviation();
    Statistics::getInstance()->addFine(
            std::max<types::time_t>(0, end_ - (event.arrivalTime + event.plannedStayDays * types::MINS_IN_DAY)));
}

bool Crane::isBusy(types::time_t time) {
    if (!ship_)
        return false;
    if (end_ <= time)
        return false;
    return true;
}

types::CargoType Crane::getType() {
    return cargoType_;
}

std::shared_ptr<Ship> Crane::getShip() {
    return ship_;
}

bool Crane::shipInCrane() {
    return ship_.get();
}
