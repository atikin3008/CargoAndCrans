#include "../include/Schedule.h"
#include "../include/Settings.h"

using json = nlohmann::json;


namespace schedule {
    types::CargoType Schedule::stringToCargoType(const std::string& typeStr) {
        if (typeStr == "bulk")      return types::CargoType::BULK;
        if (typeStr == "liquid")    return types::CargoType::LIQUID;
        if (typeStr == "container") return types::CargoType::CONTAINER;
        throw std::invalid_argument("Unknown cargo_type: " + typeStr);
    }

    types::time_t Schedule::parseTime(const std::string& timeStr) {
        size_t colonPos = timeStr.find(':');
        if (colonPos == std::string::npos) {
            throw std::invalid_argument("Invalid time format: " + timeStr);
        }
        
        int hours = std::stoi(timeStr.substr(0, colonPos));
        int minutes = std::stoi(timeStr.substr(colonPos + 1));
        
        return hours * types::MINS_IN_HOUR + minutes;
    }

    Schedule::Schedule(const std::string& settingsFilename, const std::string& scheduleFilename) {
        Settings scheduleSettingsFile(scheduleFilename), settings(settingsFilename);
        auto scheduleSettings = scheduleSettingsFile.get("schedule");
        
        for (auto shipSettings : scheduleSettings.as<SettingsNode::array_t>()) {
            auto ship = std::make_shared<Ship>(
                shipSettings.at("ship_id").as<int64_t>(),
                shipSettings.at("ship_name").as<std::string>(),
                shipSettings.at("cargo_weight_tonnes").as<types::mass_t>(),
                stringToCargoType(shipSettings.at("cargo_type").as<std::string>())
            );
            
            ScheduleEvent se {
                std::move(ship),
                (shipSettings.at("arrival_date").as<int>()-1) * types::MINS_IN_DAY +
                parseTime(shipSettings.at("arrival_time").as<std::string>()),
                shipSettings.at("planned_stay_days").as<int>()
            };

            events.push_back(std::move(se));
        }

        std::sort(events.begin(), events.end(),
                [](const ScheduleEvent& a, const ScheduleEvent& b) {
                    
                    return a.arrivalTime < b.arrivalTime;
                });
    }

    std::vector<ScheduleEvent> Schedule::getEvents(types::time_t time) const {
        std::vector<ScheduleEvent> answer;
        for (int i = 0; i < events.size(); i++) {
            if (events[i].arrivalTime == time) {
                answer.push_back(events[i]);
            }
        }
        return answer;
    }
    std::vector<ScheduleEvent> Schedule::getEvents() const
    {
        return events;
    }

} // namespace schedule
