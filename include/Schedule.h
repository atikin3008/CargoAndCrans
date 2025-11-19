#pragma once
#include <memory>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include "Ship.h"
#include "nlohmann/json.hpp"
#include "../include/Types.h"

namespace schedule {
    class ScheduleEvent {
    public:
        std::shared_ptr<Ship> ship;
        types::time_t arrival_time = 0;
        int         planned_stay_days = 0;

        ScheduleEvent() = default;

        ScheduleEvent(std::shared_ptr<Ship> s, types::time_t time, int stay_days)
            : ship(std::move(s)),
            arrival_time(time),
            planned_stay_days(stay_days) {}
    };

    class Schedule {
     public:
        Schedule(const std::string& filename);
        std::vector<ScheduleEvent> getEvents(types::time_t time) const;
        
     private:
        std::vector<ScheduleEvent> events;

        static types::CargoType stringToCargoType(const std::string& type_str);
        static types::time_t parseTime(const std::string& time_str);
    };

} // namespace schedule
