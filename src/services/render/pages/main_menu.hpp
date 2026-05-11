#pragma once

#include "[pages_loader].hpp"

#include "../components/text/component.hpp"
#include "../components/button/component.hpp"
#include "../components/block/component.hpp"

#include "network/websocket/interfaces/client.hpp"

#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <chrono>

namespace Core::App::Render::Pages {

    class MainMenu final : public PagesServiceInstance
    {
        using Client         = Network::Websocket::Interface::Client;
        using GameController = Game::Interface::Controller;
        using Session        = Game::Interface::Controller::Stats::Session;

        static constexpr int kRowsPerPage = 3;

        const sf::Vector2f center_         { Width / 2.f, Height / 2.f };
        const sf::Vector2f containerSize   { 980.f, 600.f };
        const sf::Vector2f containerCenter { center_.x, center_.y + 40.f };

        const sf::Vector2f cLT { containerCenter.x - containerSize.x * 0.5f,
                                 containerCenter.y - containerSize.y * 0.5f };
        const sf::Vector2f cRT { containerCenter.x + containerSize.x * 0.5f,
                                 containerCenter.y - containerSize.y * 0.5f };

        struct LobbyUI
        {
            UI::Components::Block::Shared  row;
            UI::Components::Text::Shared   title;
            UI::Components::Text::Shared   players;
            UI::Components::Button::Shared play;
            bool visible { false };
        };

        struct UIState
        {
            UI::Components::Text::Shared  title;

            UI::Components::Block::Shared  container;

            UI::Components::Button::Shared playBig;

            UI::Components::Block::Shared  accountPanel;
            UI::Components::Text::Shared   accountTitle;
            UI::Components::Text::Shared   accountLogin;
            UI::Components::Text::Shared   accountMaxExp;
            UI::Components::Button::Shared profileSettings;
            UI::Components::Button::Shared logout;

            UI::Components::Text::Shared   lobbiesTitle;
            std::array<LobbyUI, kRowsPerPage> lobbyRows;
            UI::Components::Text::Shared   noLobbies;

            // Pagination
            UI::Components::Button::Shared pagePrev;
            UI::Components::Text::Shared   pageInfo;
            UI::Components::Button::Shared pageNext;
        } ui;

        Client::Shared        client_;
        GameController::Shared gameController_;

        std::vector<Session> sessions_;
        int currentPage_ { 0 };
        bool joinInProgress_ { false };
        Game::MainState lastMainState_ { Game::MainState_Connecting };

        std::chrono::steady_clock::time_point lastUpdate_ =
            std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration::zero());

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
        void RebuildPage();

        void OnPlayClick();
        void OnPlaySessionClick(uint32_t serverID);
        void OnLogoutClick();
    };

} // namespace Core::App::Render::Pages
