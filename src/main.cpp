#include "../include/Settings.h"
#include "../include/GUI.h"
#include "../include/Port.h"

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    Port port("", "");
    port.process();
    gui::GUI gui("");
    gui.generateAnimations(port);
}