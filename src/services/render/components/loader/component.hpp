#pragma once

#include "../[base_component].hpp"

#include <memory>

namespace Core::App::Render::UI::Components {

    class Loader final : public BaseComponent
    {
        struct Config
        {
            sf::Vector2f center {};

            float radius       { 60.f };
            float dotRadius    { 8.f };
            float baseAngleDeg { 0.f };

            int dotsCount = 8;
        } config_;

        std::vector<sf::CircleShape> dots_;
        sf::CircleShape baseCircle_;

        sf::Clock clock_;
        std::vector<sf::Drawable *> drawables_;
    public:
        using Shared = std::shared_ptr<Loader>;

        Loader();

        void Update() override;

        std::vector<sf::Drawable *> Drawables() override;

    public:
        const Config & GetConfig() const { return config_; }

        static Shared Create(const Config & config)
        {
            auto ui = std::make_shared<Loader>();
            ui->config_ = config;
            return ui;
        }
    };
}
