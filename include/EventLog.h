#pragma once
#include"Event.h"
#include"Types.h"
#include<vector>
#include<memory>


class EventLog{
    public:
    void pushEvent(std::shared_ptr<Event> event);
    std::vector<std::shared_ptr<Event>> getEvents(types::time_t);
private:
    std::vector<std::shared_ptr<Event>> log;
};