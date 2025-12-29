#pragma once

#include "[pages_loader].hpp"

#include "../components/text/component.hpp"
#include "../components/block/component.hpp"

#include "network/websocket/interfaces/client.hpp"

#include "legacy_common.hpp"

#include <SFML/Graphics.hpp>

#include "legacy_entities.hpp"

namespace Core::App::Render::Pages {

    class Playing final : public PagesServiceInstance
    {
        using Client = Network::Websocket::Interface::Client;
        using Message = Network::Websocket::Interface::Message;
        using ConnectionState = Network::Websocket::Interface::Client::ConnectionState;
        using GameController = Game::Interface::Controller;

        sf::Vector2f center_ {
            Width  / 2.f,
            Height / 2.f
        };
        const sf::Vector2f size_ = {Width, Height};
        sf::View view_ = {center_, size_};
        float zoom_ = 10.0;

        sf::Clock clock;
        sf::Font font;
        sf::Shader glowShader, snakeShader;
        sf::Texture whiteTexture;

        uint32_t frame_ = 0;

        struct
        {
            UI::Components::Block::Shared debugPanel;
            UI::Components::Text::Shared  debugText;
        } ui;

        GameController::Shared gameController_;
    public:
        void Initialise() override;

        void OnAllInterfacesLoaded() override;

        void UpdateScene() override;

        void HandleEvent(sf::Event &event, sf::RenderWindow &window) override;

        void DrawGrid(sf::RenderWindow & window);

        void DrawSnake(sf::RenderWindow & window, const Utils::Legacy::Game::Interface::Entity::Snake::Shared & snake);

        void DrawFood(sf::RenderWindow & window, const Utils::Legacy::Game::Interface::Entity::Food::Shared & food);

        sf::Vector2f GetCameraCenter();

        void SetCameraCenter(const sf::Vector2f & point);

        void SetZoom(float z);

        sf::Vector2f GetMousePosition(sf::RenderWindow & window);
    private:

        [[nodiscard]] Game::MainState GetType() const override
        {
            return Game::MainState_Playing;
        }
    };

} // namespace Core::App::Render::Pages