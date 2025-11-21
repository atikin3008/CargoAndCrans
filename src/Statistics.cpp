#include"../include/Statistics.h"

Statistics *Statistics::getInstance() {
    static Statistics* singleton_ = nullptr;
    if(singleton_ == nullptr){
        singleton_ = new Statistics();
    }
    return singleton_;
}