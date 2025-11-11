#pragma once
#include<cinttypes>

namespace types{
    typedef uint16_t time_t;
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
}