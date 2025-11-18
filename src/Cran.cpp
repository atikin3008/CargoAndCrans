#include "../include/Crane.h"
#include <cmath>

Crane::Crane(types::CargoType type) : cargoType_(type), ship_(nullptr), end_(0) {}

void Crane::addShip(const std::shared_ptr<Ship> &ship, types::time_t time) {
    if(!ship){
        ship_ = nullptr;
        end_ = 0;
        return;
    }
    if (ship->cargo_type != cargoType_)
        throw std::runtime_error("Types are not equal");
    if (end_ > time)
        throw std::runtime_error("Ship is not unloaded");
    ship_ = ship;
    end_ = time + (types::time_t) std::ceil(ship->cargo_weight_tonnes / types::getSpeedOfUnloadCargo(ship->cargo_type));
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

std::shared_ptr<Ship> Crane::getShip(){
    return ship_;
}

bool Crane::shipInCran(){
    return ship_.get();
}