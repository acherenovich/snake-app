#pragma once

#include "event_system2.hpp"

#include <list>
#include <SFML/System/Vector2.hpp>

enum GameEvents {

};

enum GameState {
    GameIdle,
    GameMenu,
    GameLoading,
    GamePlaying,
};

namespace Interface {
    namespace Entity {
        class BaseEntity {
        public:
            using Shared = std::shared_ptr<BaseEntity>;

            struct Color {
                uint8_t r = 255;
                uint8_t g = 255;
                uint8_t b = 255;
                uint8_t a = 255;
            };

            [[nodiscard]] virtual uint32_t FrameCreated() const = 0;
            [[nodiscard]] virtual uint32_t FrameKilled() const = 0;

            [[nodiscard]] virtual bool IsKilled() const = 0;

            [[nodiscard]] virtual const sf::Vector2f & GetPosition() const = 0;
        };

        class Food: public virtual BaseEntity{
        public:
            using Shared = std::shared_ptr<Food>;

            [[nodiscard]] virtual float GetRadius() const = 0;
            [[nodiscard]] virtual const uint8_t & GetPower() const = 0;
            [[nodiscard]] virtual const Color & GetColor() const = 0;
        };

        class Snake: public virtual BaseEntity {
        public:
            using Shared = std::shared_ptr<Snake>;

            [[nodiscard]] virtual float GetRadius(bool head = false) const = 0;
            [[nodiscard]] virtual std::string GetName() const = 0;
            [[nodiscard]] virtual const uint32_t & GetExperience() const = 0;
            [[nodiscard]] virtual const std::list<sf::Vector2f> & Segments() const = 0;
        };
    }

    class Game
    {
    public:
        constexpr static float AreaWidth = 10000.0;
        constexpr static float AreaHeight = 10000.0;

        constexpr static float AreaRadius = 5000.0;
        static const sf::Vector2f AreaCenter;

        constexpr static int FoodCount = 500;

        virtual void StartGame(const std::string & name) = 0;

        virtual GameState State() = 0;

        virtual Utils::Event::System<GameEvents> & Events() = 0;
    };
}