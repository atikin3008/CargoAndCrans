#pragma once
#include"Types.h"
#include"Ship.h"
#include"Cran.h"
#include<memory>


class Event{
    public:
        Event();
        virtual types::EventType get_type();

    private:
        types::time_t time;

};

class ArrivalEvent : Event{
    public:
    types::EventType get_type(){
        return types::EventType::ON_SHIP_ARRIVAL;
    }
    private:
    std::shared_ptr<Ship> ship;
};

class InCranEvent : Event
{
public:
    types::EventType get_type()
    {
        return types::EventType::ON_SHIP_IN_CRAN;
    }

private:
    std::shared_ptr<Ship> ship;
    std::shared_ptr<Cran> cran;
};


class DepatureEvent : Event
{
public:
    types::EventType get_type()
    {
        return types::EventType::ON_SHIP_DEPATURE;
    }

private:
    std::shared_ptr<Ship> ship;
    std::shared_ptr<Cran> cran;
};