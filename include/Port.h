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
    private:
    EventLog eventLog;
    schedule::Schedule Schedule;
    Settings settings;
};