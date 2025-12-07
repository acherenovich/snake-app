#pragma once

#include "game.hpp"
#include "server.hpp"
#include "game_state.hpp"

#include "common.hpp"

class Logic: public Interface::Game {
    GameState gameState = GameIdle;
    Utils::Event::System<GameEvents> events;

    Interface::Server * server = {};

    Initializer initializer;
public:
    Logic(Initializer  init);

    void Think();

    Utils::Event::System<GameEvents> & Events() override { return events; }

    Utils::Log::Shared & Log() { return initializer.log; }
};