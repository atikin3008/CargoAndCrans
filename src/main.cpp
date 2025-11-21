#include "../include/GUI.h"
#include "../include/Port.h"
#include <iostream>

int main() {
    try {
        const std::string settingsFile = "settings.json";
        const std::string scheduleFile = "schedule.json";
        const std::string fontPath = "fonts/DejaVuSans.ttf";

        Port port(settingsFile, scheduleFile);
        port.process();
        for(auto &it : port.get()){
            if(it->getShip()->getName().starts_with("Aurora Spirit")){
                std::cout << it->getShip()->getId()  <<"  "<< types::getStringByEventType(it->getType()) << " " << it->getTime() << "\n";
            }
        }

        PortGUI gui(port, settingsFile, fontPath);
        gui.run();
    } catch (const std::exception &ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
