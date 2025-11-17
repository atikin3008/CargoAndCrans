#pragma once
#include<cinttypes>
#include<string>
#include<exception>

namespace types{
    typedef uint32_t time_t;
    typedef uint64_t mass_t;
    enum class CargoType{
        BULK,
        LIQUID,
        CONTAINER
    };

    enum class EventType{
        ON_SHIP_ARRIVAL,
        ON_SHIP_IN_CRAN,
        ON_SHIP_DEPATURE
    };

    inline double getSpeedOfUnloadCargo(CargoType type){
        switch(type){
            case CargoType::BULK: return 10.;
            case CargoType::LIQUID: return 20.;
            case CargoType::CONTAINER: return 30.;
        }
    }
    inline CargoType getTypeByString(const std::string &type){
        if(type == "BULK")
            return CargoType::BULK;
        if(type == "LIQUID")
            return CargoType::LIQUID;
        if(type == "CONTAINER")
            return CargoType::CONTAINER;
        throw std::runtime_error("Bad type name");

    }
}