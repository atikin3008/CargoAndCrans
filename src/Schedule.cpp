#include "../include/Schedule.h"
#include "../include/Settings.h"

using json = nlohmann::json;


namespace schedule {
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
        
        return hours * types::MINS_IN_HOUR + minutes;
    }

    Schedule::Schedule(const std::string& settingsFilename, const std::string& scheduleFilename) {
        Settings scheduleSettingsFile(scheduleFilename), settings(settingsFilename);
        auto scheduleSettings = scheduleSettingsFile.get("schedule");

        int minimum_deviation_of_arrival = settings.get("minimum_deviation_of_arrival").as<int64_t>();
        int maximum_deviation_of_arrival = settings.get("maximum_deviation_of_arrival").as<int64_t>();
        int minimum_discharge_deviation = settings.get("minimum_discharge_deviation").as<int64_t>();
        int maximum_discharge_deviation = settings.get("maximum_discharge_deviation").as<int64_t>();
        
        
        for (auto ship_settings : scheduleSettings.as<SettingsNode::array_t>()) {
            auto ship = std::make_shared<Ship>(
                ship_settings.at("ship_id").as<int64_t>(),
                ship_settings.at("ship_name").as<std::string>(),
                ship_settings.at("cargo_weight_tonnes").as<types::mass_t>(),
                stringToCargoType(ship_settings.at("cargo_type").as<std::string>())
            );
            
            ScheduleEvent se {
                std::move(ship),
                (ship_settings.at("arrival_date").as<int>()-1) * types::MINS_IN_DAY +
                parseTime(ship_settings.at("arrival_time").as<std::string>()),
                ship_settings.at("planned_stay_days").as<int>()
            };

            events.push_back(std::move(se));
        }

        std::sort(events.begin(), events.end(),
                [](const ScheduleEvent& a, const ScheduleEvent& b) {
                    
                    return a.arrival_time < b.arrival_time;
                });
    }

    std::vector<ScheduleEvent> Schedule::getEvents(types::time_t time) const {
        std::vector<ScheduleEvent> answer;
        for (int i = 0; i < events.size(); i++) {
            if (events[i].arrival_time == time) {
                answer.push_back(events[i]);
            }
        }
        return answer;
    }

} // namespace schedule
