#pragma once
#include <fstream>
#include<string>

class Settings{
    public:
    Settings(std::string filename);
    int operator()(std::string key);
    private:

};