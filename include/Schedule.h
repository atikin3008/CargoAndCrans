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
#include "Deviations.h"

namespace schedule {
    class ScheduleEvent {
     public:
        std::shared_ptr<Ship> ship;

        types::time_t arrivalTime;
        types::time_t plannedStayDays;


        ScheduleEvent() = default;

        ScheduleEvent(std::shared_ptr<Ship> s, types::time_t time, int stayDays)
                : ship(std::move(s)),
                  arrivalTime(time + Deviations::GetInstance()->getDeviationOfArival()),
                  plannedStayDays(stayDays) {}
    };

    class Schedule {
     public:
        Schedule(const std::string &settingsFilename, const std::string &scheduleFilename);

        std::vector<ScheduleEvent> getEvents(types::time_t time) const;

     private:
        std::vector<ScheduleEvent> events;

        static types::CargoType stringToCargoType(const std::string &typeStr);

        static types::time_t parseTime(const std::string &timeStr);
    };

} // namespace schedule
