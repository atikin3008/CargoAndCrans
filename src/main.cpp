#include"../include/Port.h"
#include"../include/GUI.h"

int main(){
    Port port("", "");
    port.process();
    gui::GUI gui("");
    gui.generateAnimations(port);
}