#include "../include/Deviations.h"
#include <algorithm>

Deviations *Deviations::singleton_ = nullptr;

Deviations *Deviations::GetInstance() {

    if (singleton_ == nullptr) {
        singleton_ = new Deviations("settings.json");
    }
    return singleton_;
}

int64_t Deviations::getDeviationOfArival() {
    double mean = (minimumDeviationOfArrival + maximumDeviationOfArrival) / 2.0;
    double stddev = (maximumDeviationOfArrival - minimumDeviationOfArrival) / 6.0;
    std::normal_distribution<double> dist(mean, stddev);
    int64_t value = static_cast<int>(std::round(dist(deviationOfArrival)));
    return std::clamp(value, minimumDeviationOfArrival, maximumDeviationOfArrival);
}

int64_t Deviations::getDischargeDeviation() {
    double mean = (minimumDischargeDeviation + maximumDischargeDeviation) / 2.0;
    double stddev = (maximumDischargeDeviation - minimumDischargeDeviation) / 6.0;
    std::normal_distribution<double> dist(mean, stddev);
    int64_t value = static_cast<int>(std::round(dist(deviationOfArrival)));
    return std::clamp(value, minimumDischargeDeviation, maximumDischargeDeviation);
}

