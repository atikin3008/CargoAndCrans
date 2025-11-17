#include "../include/Event.h"

types::time_t Event::getTime(){
    return time_;
}

bool Event::operator>(types::time_t time) const {
    return time_ > time;
}

bool Event::operator==(types::time_t time) const {
    return time_ == time;
}

bool Event::operator<(types::time_t time) const {
    return !(operator>(time) && operator==(time));
}