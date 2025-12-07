#pragma once

#include "[pages_loader].hpp"

#include "../components/loader/component.hpp"
#include "../components/text/component.hpp"

#include "network/websocket/interfaces/client.hpp"

#include <SFML/Graphics.hpp>

namespace Core::App::Render::Pages {

    class Authorization final : public PagesServiceInstance
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
        } ui;

        Client::Shared client_;
        ConnectionState connectionState_ = ConnectionState::ConnectionState_Connecting;
    public:
        void Initialise() override;

        void OnAllInterfacesLoaded() override;

        void UpdateScene() override;

        [[nodiscard]] Game::MainState GetType() const override
        {
            return Game::MainState_Login;
        }
    };

} // namespace Core::App::Render::Pages