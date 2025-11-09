#pragma once


#include "common.hpp"
#include "game.hpp"

#include <list>

#include <SFML/System/Vector2.hpp>

enum GraphicsEvents {

};

namespace Interface {
    class Graphics
    {
    public:
        constexpr static float Width = 1600.f;
        constexpr static float Height = 900.f;
        constexpr static float SnakePartRadius = 30.f;
        constexpr static float SnakeHeadRadius = 45.f;
        constexpr static float FoodRadius = 10.f;
        constexpr static float SmoothDuration = 10.f;
#ifdef BUILD_CLIENT
        virtual Utils::Event::System<GraphicsEvents> & Events() = 0;

        virtual void RenderMenu() = 0;

        virtual void SetServerFrame(uint32_t frame_) = 0;

        virtual void AddSnake(const Entity::Snake::Shared & snake) = 0;

        virtual void AddFood(const Entity::Food::Shared & food) = 0;

        virtual void AddDebug(const Entity::Snake::Shared & snake) = 0;

        virtual void AddLeaderboard(const std::map<uint32_t, std::string> & scores) = 0;

        virtual sf::Vector2f GetCameraCenter() = 0;

        virtual void SetCameraCenter(const sf::Vector2f & point) = 0;

        virtual void SetZoom(float z) = 0;

        virtual sf::Vector2f GetMousePosition() = 0;
#endif
    };
}

