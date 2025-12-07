#pragma once

#include "[pages_loader].hpp"

#include "../components/loader/component.hpp"
#include "../components/text/component.hpp"

#include <SFML/Graphics.hpp>

namespace Core::App::Render::Pages {

    class Connecting final : public PagesServiceInstance
    {
        const sf::Vector2f center_ {
            Width  / 2.f,
            Height / 2.f
        };

        struct
        {
            UI::Components::Text::Shared text_;
            UI::Components::Loader::Shared loader_;
        } ui;
    public:
        void Initialise() override;
        void UpdateScene() override;

        [[nodiscard]] Game::MainState GetType() const override
        {
            return Game::MainState_Connecting;
        }
    };

} // namespace Core::App::Render::Pages