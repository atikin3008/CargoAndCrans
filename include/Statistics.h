#pragma once
#include<map>
#include"Types.h"

class Statistics{
 public:
    static Statistics* getInstance();
    void addMass(types::mass_t mass);
    void addShip();
    void addFine(int64_t fine);


 private:
    Statistics() = default;
    std::map<std::string, int> statistics;

};