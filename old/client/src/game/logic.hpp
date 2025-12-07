#pragma once

#include "game.hpp"
#include "server.hpp"
#include "graphics.hpp"

#include "entities/snake.hpp"
#include "entities/food.hpp"

#include "common.hpp"

#include <set>


class Logic: public Interface::Game {
    Interface::Graphics * graphics = {};
    Interface::Server * server = {};

    std::map<uint32_t, std::string> leaderboard;

    std::map<uint64_t, Entity::Snake::Shared> snakes;
    std::vector<Entity::Food::Shared> foods;

    GameState gameState = GameMenu;
    Utils::Event::System<GameEvents> events;

    Initializer initializer;

    uint32_t frame = 0;
public:
    Logic(Initializer  init);

    void OnAllLoaded();

    void Think();

private:
    void GenerateLeaderboard();
    void GenerateFoods();
    void CheckCollision(std::set<Entity::Snake::Shared> & killList);
    void KillSnakes(const std::set<Entity::Snake::Shared> & list);
    Entity::Snake::Shared AddSnake(const Interface::Session::Shared & session);
    void SetDestination(const Interface::Session::Shared & session, const sf::Vector2f & dest);
    bool MoveSnake(const Entity::Snake::Shared & snake);
    void SendFullSessionUpdate(const Interface::Session::Shared & session);
    void ReceiveFullSessionUpdate(const Interface::Session::Shared & session, Message::DataUpdate & update);
    void SendMoveDestination(const Interface::Session::Shared & session, const sf::Vector2f & dest);
public:
    void StartGame(const std::string & name) override;

    GameState State() override;

    Utils::Event::System<GameEvents> & Events() override { return events; }

    Utils::Log::Shared & Log() { return initializer.log; }
};