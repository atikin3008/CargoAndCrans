#pragma once

#include<map>
#include"Types.h"

class Statistics {
 public:
    static Statistics *getInstance();

    void addMass(types::mass_t mass);

    void addShip();

    void addFine(int64_t fine);

    std::map<std::string, int> getStatistics() const;


 private:
    Statistics();

    std::map<std::string, int> statistics;

};