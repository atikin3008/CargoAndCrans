#include "../include/Port.h"
#include "../include/Schedule.h"
#include<iostream>

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    Port port("../settings.json", "../schedule.json");
    port.process();
    auto events = port.get();
    for (auto event : events) {
        std::cout << "event_type " << types::getStringByEventType(event->getType()) << " with time: " << event->getTime() << '\n';
    }
}