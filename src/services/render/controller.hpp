#pragma once

#include "services/game/interfaces/controller.hpp"
#include "interfaces/controller.hpp"

#include <SFML/Graphics.hpp>

namespace Core::App::Render
{
    class Controller final : public Interface::Controller, public std::enable_shared_from_this<Controller>
    {
        using Game = Game::Interface::Controller;

        sf::ContextSettings settings_ {
            0,   // depth
            0,   // stencil
            4,   // antialiasing level (4 или 8 – норм)
        };

        sf::RenderWindow window_ {
            sf::VideoMode(
                static_cast<unsigned int>(Width),
                static_cast<unsigned int>(Height),
                64
            ),
            "Snake",
            sf::Style::Titlebar | sf::Style::Close,
            settings_
        };
        const sf::Vector2f size_ = {Width, Height};
        sf::Vector2f center_ = {Width / 2.f, Height / 2.f};
        sf::View view_ = {center_, size_};

        Game::Shared game_;
    public:
        using Shared = std::shared_ptr<Controller>;

    protected:
        void Initialise() override;

        void OnAllInterfacesLoaded() override;

        void ProcessTick() override;

        void UpdateScene();

    public:
        sf::RenderWindow & Window();

    protected:
        std::string GetServiceContainerName() const override
        {
            return "Render";
        }

        std::vector<Utils::Service::SubLoader> SubLoaders() override;;
    };
}
