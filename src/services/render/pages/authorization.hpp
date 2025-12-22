#pragma once

#include "[pages_loader].hpp"

#include "../components/text/component.hpp"
#include "../components/button/component.hpp"
#include "../components/input/component.hpp"
#include "../components/loader/component.hpp"
#include "../components/overlay/component.hpp"

#include "network/websocket/interfaces/client.hpp"
#include "services/game/interfaces/controller.hpp"
#include "components/local_storage/interfaces/storage.hpp"

#include <SFML/Graphics.hpp>

namespace Core::App::Render::Pages {

    class Authorization final : public PagesServiceInstance
    {
        using Client = Network::Websocket::Interface::Client;
        using GameController = Game::Interface::Controller;
        using ConnectionState = Network::Websocket::Interface::Client::ConnectionState;
        using Storage = Components::LocalStorage::Interface::Storage;

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

            // NEW
            UI::Components::Overlay::Shared overlay;
            UI::Components::Loader::Shared loader;
            UI::Components::Text::Shared loadingText;
        } ui;

        bool submitting_ { false };

        Mode mode_ { Mode::Login };

        Client::Shared client_;
        GameController::Shared gameController_;
        Storage::Shared localStorage_;

        bool loginTokenTried { false };
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
        void TryLoginToken();
        void Submit(const std::string & token = "");
        void SetMode(Mode m);
        void RefreshModeUI();
        void Validate();
    };

} // namespace Core::App::Render::Pages