#include "../include/Settings.h"

#include <iostream>

int main() {
    Settings settings("../test.json");
    auto items = settings.get<SettingsNode::array_t>("xuila");

    for (auto &item : items) {
        std::cout << "a=" << item.at("a").as<std::int64_t>() << ", b="
                  << item.at("b").as<std::int64_t>() << '\n';
    }

    return 0;
}
