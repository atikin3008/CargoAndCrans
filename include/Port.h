#pragma once
#include"EventLog.h"
#include "Schedule.h"
#include"Types.h"
#include"Event.h"
#include "Settings.h"
#include<vector>
#include<memory>
#include<string>

class Port{
    public:
    Port(std::string settingsFilename, std::string scheduleFilename);
    void process();
    std::vector<std::shared_ptr<Event>> get(types::time_t time);
    std::vector<std::shared_ptr<Event>> get();
 private:
    EventLog eventLog;
    schedule::Schedule schedule;
    Settings settings;
    std::vector<std::shared_ptr<Ship>> shipsInOrder;
    std::vector<std::shared_ptr<Crane>> crans;
};