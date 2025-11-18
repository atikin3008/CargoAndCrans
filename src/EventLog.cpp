#include"../include/EventLog.h"
#include<algorithm>

struct EventTimeLess {
    bool operator()(const std::shared_ptr<Event> &e, types::time_t t) const {
        return e->getTime() < t;   // same as comparing the raw timestamp
    }

    bool operator()(types::time_t t, const std::shared_ptr<Event> &e) const {
        return t < e->getTime();   // same as comparing the raw timestamp
    }
};


void EventLog::pushEvent(std::shared_ptr<Event> event) {
    log.push_back(event);
}

std::vector<std::shared_ptr<Event>> EventLog::getEvents(types::time_t time) {
    EventTimeLess comp;
    auto range = std::equal_range(log.begin(), log.end(), time, comp);
    return std::vector<std::shared_ptr<Event>>(range.first, range.second);
}

std::vector<std::shared_ptr<Event>> EventLog::getEvents() {
    return log;
}
