#pragma once
#include<cinttypes>
#include<string>
#include<exception>
#include <stdexcept>

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
        ON_SHIP_IN_CRANE,
        ON_SHIP_DEPARTURE
    };

    inline std::string getStringByEventType(EventType event) {
        switch(event){
            case EventType::ON_SHIP_ARRIVAL: return "ship arrival";
            case EventType::ON_SHIP_IN_CRANE: return "ship in crane";
            case EventType::ON_SHIP_DEPARTURE: return "ship departure";
            default: throw std::logic_error("incorrect event type");
        }
    }

    inline double getSpeedOfUnloadCargo(CargoType type){
        switch(type){
            case CargoType::BULK: return 10.;
            case CargoType::LIQUID: return 20.;
            case CargoType::CONTAINER: return 30.;
            default: throw std::logic_error("incorrect event type");
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