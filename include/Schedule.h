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
        int         arrival_date = 0;  
        types::time_t arrival_time = 0;      
        int         planned_stay_days = 0;

        ScheduleEvent() = default;

        ScheduleEvent(std::shared_ptr<Ship> s, int date, types::time_t time, int stay_days)
            : ship(std::move(s)),
            arrival_date(date),
            arrival_time(time),
            planned_stay_days(stay_days) {}

        void print();

    private:
        std::string cargoTypeToString(types::CargoType type);
        std::string formatDate(int day);
        std::string formatTime(int hours, int minutes);
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
