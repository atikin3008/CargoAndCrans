#pragma once

#include "Types.h"
#include "Ship.h"
#include<memory>

class Cran {
 public:
 private:
    types::CargoType cargoType;
    std::shared_ptr<Ship> ship;
};