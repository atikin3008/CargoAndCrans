#include"../include/Statistics.h"

Statistics *Statistics::getInstance() {
    static Statistics *singleton_ = nullptr;
    if (singleton_ == nullptr) {
        singleton_ = new Statistics();
    }
    return singleton_;
}

Statistics::Statistics() : statistics({{"MASS",  0},
                                       {"SHIPS", 0},
                                       {"FINE",  0}}) {}

void Statistics::addMass(types::mass_t mass) {
    statistics["MASS"] += mass;
}

void Statistics::addShip() {
    ++statistics["SHIPS"];
}

void Statistics::addFine(int64_t fine){
    statistics["FINE"] += fine;
}

std::map<std::string, int> Statistics::getStatistics() const{
    return statistics;
}