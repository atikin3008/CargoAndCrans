#include"../include/Deviations.h"
#include<iostream>

int main() {
    Deviations *deviations = Deviations::GetInstance("../settings.json");
    for(int i = 0; i < 100; ++i)
        std::cout << deviations->getDeviationOfArival() << "\n";
}