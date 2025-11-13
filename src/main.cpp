#include "../include/Schedule.h"

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    schedule::Schedule schedule("schedule.json");
    for (types::time_t time = 0; time <= 30 * 86400; time++) {
        auto cur_events = schedule.getEvents(time);
        if (cur_events.size() > 0) {
            for (auto event : cur_events) {
                event.print();
            }
        }
    }
}