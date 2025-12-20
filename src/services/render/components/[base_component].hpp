#pragma once

#include "services/render/interfaces/common.hpp"

#include <SFML/Graphics.hpp>

#include <numbers>

constexpr float Pi = std::numbers::pi_v<float>;


namespace Core::App::Render::UI::Components {
    class BaseComponent
    {
    public:
        using Shared = std::shared_ptr<BaseComponent>;

        virtual ~BaseComponent() = default;

        virtual void Update() = 0;

        virtual std::vector<sf::Drawable*> Drawables() = 0;
    };
}
