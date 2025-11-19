#pragma once

#include<string>
#include<random>
#include"Settings.h"


class Deviations {
 protected:
    Deviations(const std::string value) : settings(value), minimumDeviationOfArrival(
            settings.get("minimum_deviation_of_arrival").as<int64_t>()),
                                          maximumDeviationOfArrival(
                                                  settings.get("maximum_deviation_of_arrival").as<int64_t>()),
                                          minimumDischargeDeviation(
                                                  settings.get("minimum_discharge_deviation").as<int64_t>()),
                                          maximumDischargeDeviation(
                                                  settings.get("maximum_discharge_deviation").as<int64_t>()),
                                          deviationOfArrival(rd()), dischargeOfArrival(rd()) {}

    static Deviations *singleton_;


    Settings settings;
    int64_t minimumDeviationOfArrival;
    int64_t maximumDeviationOfArrival;
    int64_t minimumDischargeDeviation;
    int64_t maximumDischargeDeviation;
    std::random_device rd;
    std::mt19937 deviationOfArrival;
    std::mt19937 dischargeOfArrival;


 public:
    Deviations(Deviations &other) = delete;

    void operator=(const Deviations &) = delete;

    static Deviations *GetInstance(const std::string &settingsFilename);

    int64_t getDeviationOfArival();

    int64_t getDischargeDeviation();

};
