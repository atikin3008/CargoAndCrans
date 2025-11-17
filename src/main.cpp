#include "../include/Port.h"
#include "../include/Schedule.h"
#include<iostream>

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    Port port("../settings.json", "../schedule.json");
    port.process();
    schedule::Schedule s("../schedule.json");
    for(int i = 0; i < 20000; ++i)
        std::cout << s.getEvents(i).size() << "\n";
}