#pragma once

#include "[pages_loader].hpp"

#include "../components/loader/component.hpp"
#include "../components/text/component.hpp"

#include "network/websocket/interfaces/client.hpp"

#include <SFML/Graphics.hpp>

namespace Core::App::Render::Pages {

    class JoiningSession final : public PagesServiceInstance
    {
        using Client = Network::Websocket::Interface::Client;
        using Message = Network::Websocket::Interface::Message;
        using ConnectionState = Network::Websocket::Interface::Client::ConnectionState;

        const sf::Vector2f center_ {
            Width  / 2.f,
            Height / 2.f
        };

        struct
        {
            UI::Components::Loader::Shared loader_;
            UI::Components::Text::Shared text_;
            UI::Components::Text::Shared errorText_;
        } ui;

        size_t connectionErrors_ = 0;
    public:
        void Initialise() override;

        void OnAllInterfacesLoaded() override;

        void UpdateScene() override;

        [[nodiscard]] Game::MainState GetType() const override
        {
            return Game::MainState_JoiningSession;
        }
    };

} // namespace Core::App::Render::Pages