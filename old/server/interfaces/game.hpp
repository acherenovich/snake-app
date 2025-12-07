#pragma once

#include "event_system2.hpp"

enum GameEvents {

};

namespace Interface {
    class Game
    {
    public:
        constexpr static float AreaWidth = 10000.0;
        constexpr static float AreaHeight = 10000.0;

        virtual Utils::Event::System<GameEvents> & Events() = 0;
    };
}