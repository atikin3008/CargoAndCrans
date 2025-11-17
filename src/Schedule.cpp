#include "../include/Schedule.h"
#include "../include/Settings.h"

using json = nlohmann::json;

namespace schedule {
    void ScheduleEvent::print() {
        int hours = arrival_time / 60;
        int minutes = arrival_time % 60;
        
        int departure_date = arrival_date + planned_stay_days;
        
        std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                       SCHEDULE EVENT                        ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
        std::cout << "║  Ship ID:     " << std::setw(42) << std::left << ship->getId() << "║\n";
        std::cout << "║  Ship Name:   " << std::setw(42) << std::left << ship->getName() << "║\n";
        std::cout << "║  Cargo Type:  " << std::setw(42) << std::left 
                << cargoTypeToString(ship->getCargoType()) << "║\n";
        std::cout << "║  Cargo Weight:" << std::setw(41) << std::left 
                << std::fixed << std::setprecision(1) << ship->getCargoWeight() << " tonnes║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
        std::cout << "║  Arrival Date:  " << std::setw(38) << std::left 
                << formatDate(arrival_date) << "║\n";
        std::cout << "║  Arrival Time:  " << std::setw(38) << std::left 
                << formatTime(hours, minutes) << "║\n";
        std::cout << "║  Stay Duration: " << std::setw(37) << std::left 
                << std::to_string(planned_stay_days) + " days" << "║\n";
        std::cout << "║  Departure:     " << std::setw(38) << std::left 
                << formatDate(departure_date) << "║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
        std::cout << std::endl;
    }

    std::string ScheduleEvent::cargoTypeToString(types::CargoType type) {
        switch(type) {
            case types::CargoType::BULK: return "Bulk";
            case types::CargoType::CONTAINER: return "Container";
            case types::CargoType::LIQUID: return "Liquid";
            default: return "Unknown";
        }
    }

    std::string ScheduleEvent::formatDate(int day) {
        return std::to_string(day);
    }

    std::string ScheduleEvent::formatTime(int hours, int minutes) {
        std::string period = (hours < 12) ? "AM" : "PM";
        int display_hours = (hours == 0 || hours == 12) ? 12 : hours % 12;
        
        return (display_hours < 10 ? "0" : "") + std::to_string(display_hours) + ":" +
            (minutes < 10 ? "0" : "") + std::to_string(minutes) + " " + period;
    }




    types::CargoType Schedule::stringToCargoType(const std::string& type_str) {
        if (type_str == "bulk")      return types::CargoType::BULK;
        if (type_str == "liquid")    return types::CargoType::LIQUID;
        if (type_str == "container") return types::CargoType::CONTAINER;
        throw std::invalid_argument("Unknown cargo_type: " + type_str);
    }

    types::time_t Schedule::parseTime(const std::string& time_str) {
        size_t colon_pos = time_str.find(':');
        if (colon_pos == std::string::npos) {
            throw std::invalid_argument("Invalid time format: " + time_str);
        }
        
        int hours = std::stoi(time_str.substr(0, colon_pos));
        int minutes = std::stoi(time_str.substr(colon_pos + 1));
        
        return hours * 60 + minutes;
    }

    Schedule::Schedule(const std::string& filename) {
        Settings settingsFile(filename);
        auto settings = settingsFile.get("schedule");
        
        for (auto ship_settings : settings.as<SettingsNode::array_t>()) {
            auto ship = std::make_shared<Ship>(
                ship_settings.at("ship_id").as<int64_t>(),
                ship_settings.at("ship_name").as<std::string>(),
                ship_settings.at("cargo_weight_tonnes").as<types::mass_t>(),
                stringToCargoType(ship_settings.at("cargo_type").as<std::string>())
            );
            
            ScheduleEvent se {
                std::move(ship),
                ship_settings.at("arrival_date").as<int>(),
                parseTime(ship_settings.at("arrival_time").as<std::string>()),
                ship_settings.at("planned_stay_days").as<int>()
            };

            events.push_back(std::move(se));
        }

        std::sort(events.begin(), events.end(),
                [](const ScheduleEvent& a, const ScheduleEvent& b) {
                    if (a.arrival_date == b.arrival_date) {
                        return a.arrival_time < b.arrival_time;
                    }
                    return a.arrival_date < b.arrival_date;
                });
    }

    std::vector<ScheduleEvent> Schedule::getEvents(types::time_t time) const {
        std::vector<ScheduleEvent> answer;
        for (int i = 0; i < events.size(); i++) {
            types::time_t seconds = (events[i].arrival_date-1) * 86400 + events[i].arrival_time;
            if (seconds == time) {
                answer.push_back(events[i]);
            }
        }
        return answer;
    }

} // namespace schedule