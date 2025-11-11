#pragma once
#include"Settings.h"
#include"Ship.h"
#include"Port.h"
#include<memory>
#include<vector>



namespace gui{
    class AnimationPosition{
        public:

        private:
        std::shared_ptr<Ship> ship;
    };
    class GUI{
        public:
        GUI(std::string settingsFilename);
        void generateAnimations(Port port);
        private:
        std::vector<std::shared_ptr<AnimationPosition>> all;
    };
}