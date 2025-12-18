#pragma once

#include "[pages_loader].hpp"

#include "../components/text/component.hpp"
#include "../components/button/component.hpp"
#include "../components/input/component.hpp"

#include "network/websocket/interfaces/client.hpp"

#include <SFML/Graphics.hpp>

namespace Core::App::Render::Pages {

    class Authorization final : public PagesServiceInstance
    {
        using Client = Network::Websocket::Interface::Client;
        using ConnectionState = Network::Websocket::Interface::Client::ConnectionState;

        enum class Mode
        {
            Login,
            Register
        };

        const sf::Vector2f center_ { Width / 2.f, Height / 2.f };

        struct UIState
        {
            UI::Components::Text::Shared title;

            UI::Components::Button::Shared tabLogin;
            UI::Components::Button::Shared tabRegister;

            UI::Components::Input::Shared login;
            UI::Components::Input::Shared password;
            UI::Components::Input::Shared password2;

            UI::Components::Button::Shared submit;
            UI::Components::Text::Shared hint;
        } ui;

        Mode mode_ { Mode::Login };

        Client::Shared client_;
        ConnectionState connectionState_ = ConnectionState::ConnectionState_Connecting;

    public:
        void Initialise() override;
        void OnAllInterfacesLoaded() override;

        void UpdateScene() override;
        void HandleEvent(sf::Event& event, sf::RenderWindow& window) override;

        [[nodiscard]] Game::MainState GetType() const override
        {
            return Game::MainState_Login;
        }

    private:
        void SetMode(Mode m);
        void RefreshModeUI();
        void Validate();
    };

} // namespace Core::App::Render::Pages