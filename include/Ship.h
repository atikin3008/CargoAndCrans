#pragma once

#include <string>
#include "../include/Types.h"

class Ship {
public:
    int         id = 0; 
    std::string name;
    double      cargo_weight_tonnes = 0.0;
    types::CargoType   cargo_type = types::CargoType::BULK;

    Ship() = default;

    Ship(int id_, std::string name_, double weight_, types::CargoType type_)
        : id(id_), name(std::move(name_)), cargo_weight_tonnes(weight_), cargo_type(type_) {}

    int getId() const {
        return id;
    }

    std::string getName() const {
        return name;
    }

    double getCargoWeight() const {
        return cargo_weight_tonnes;
    }

    types::CargoType getCargoType() const {
        return cargo_type;
    }
};
