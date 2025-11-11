#pragma once
#include "Event.h"
#include "Types.h"
#include<fstream>
#include<vector>
#include<memory>
#include <string>

class Schedule{
    public:
    Schedule(std::string file_name);
    std::vector<std::shared_ptr<ArrivalEvent>> get(types::time_t);
private:
    std::vector<std::shared_ptr<ArrivalEvent>> schedule;
};