#pragma once

#include "[pages_loader].hpp"

#include "../components/text/component.hpp"
#include "../components/button/component.hpp"
#include "../components/block/component.hpp"

#include "network/websocket/interfaces/client.hpp"

#include <SFML/Graphics.hpp>
#include <array>

namespace Core::App::Render::Pages {

    class MainMenu final : public PagesServiceInstance
    {
        using Client = Network::Websocket::Interface::Client;
        using GameController = Game::Interface::Controller;

        const sf::Vector2f center_ { Width / 2.f, Height / 2.f };

        struct LobbyUI
        {
            UI::Components::Block::Shared row;
            UI::Components::Text::Shared title;
            UI::Components::Text::Shared players;
            UI::Components::Button::Shared play;
        };

        struct UIState
        {
            UI::Components::Text::Shared title;

            // Main centered container
            UI::Components::Block::Shared container;

            // Left
            UI::Components::Button::Shared playBig;

            // Right (Account)
            UI::Components::Block::Shared accountPanel;
            UI::Components::Text::Shared accountTitle;
            UI::Components::Text::Shared accountLogin;
            UI::Components::Text::Shared accountMaxExp;
            UI::Components::Button::Shared profileSettings;
            UI::Components::Button::Shared logout;

            // Bottom (Lobbies)
            UI::Components::Text::Shared lobbiesTitle;
            std::array<LobbyUI, 3> lobbies;
        } ui;

        Client::Shared client_;
        GameController::Shared gameController_;

        std::chrono::steady_clock::time_point lastUpdate_ = std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration::zero());
    public:
        void Initialise() override;
        void OnAllInterfacesLoaded() override;

        void ProcessTick() override;

        void UpdateScene() override;
        void HandleEvent(sf::Event& event, sf::RenderWindow& window) override;

        [[nodiscard]] Game::MainState GetType() const override
        {
            return Game::MainState_Menu;
        }

    private:
        void BuildLayout();

        void LoadStats();

        void OnPlayClick();
        void OnPlaySessionClick(uint32_t id);

        void OnLogoutClick();
    };

} // namespace Core::App::Render::Pages