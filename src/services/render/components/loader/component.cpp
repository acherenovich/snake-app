#include "component.hpp"

#include <cmath>

namespace Core::App::Render::UI::Components {
    Loader::Loader()
    {
        // Базовый круг (почти незаметный фон под спиннером)
        const float baseRadius = config_.radius - config_.dotRadius;
        baseCircle_.setRadius(baseRadius);
        baseCircle_.setOrigin(baseRadius, baseRadius);
        baseCircle_.setOutlineThickness(config_.dotRadius * 2);
        baseCircle_.setFillColor(sf::Color::Transparent);
        baseCircle_.setOutlineColor(sf::Color(255, 255, 255, 10));

        // Настраиваем точки спиннера
        const float dotRadius = config_.dotRadius;
        for (int i = 0; i < config_.dotsCount; i++)
        {
            auto & dot = dots_.emplace_back();
            dot.setRadius(dotRadius);
            dot.setOrigin(dotRadius, dotRadius);
            dot.setFillColor(sf::Color::White);
        }

        drawables_.push_back(&baseCircle_);
        for (auto & dot : dots_)
            drawables_.push_back(&dot);

        clock_.restart();
    }

    void Loader::Update()
    {
        const float dt = clock_.restart().asSeconds();

        // Скорость вращения
        constexpr float rotationSpeedDegPerSec = 180.f;
        config_.baseAngleDeg += rotationSpeedDegPerSec * dt;
        if (config_.baseAngleDeg > 360.f)
            config_.baseAngleDeg -= 360.f;

        const sf::Vector2f center {
            Width  / 2.f,
            Height / 2.f
        };

        baseCircle_.setPosition(center);

        // Расставляем точки по окружности с фазовым сдвигом
        for (int i = 0; i < config_.dotsCount; ++i)
        {
            const float phase  = 360.f * static_cast<float>(i) / static_cast<float>(config_.dotsCount);
            const float angle  = (config_.baseAngleDeg + phase) * static_cast<float>(Pi) / 180.f;

            const sf::Vector2f pos {
                center.x + std::cos(angle) * config_.radius,
                center.y + std::sin(angle) * config_.radius
            };

            // Альфа плавно убывает по индексу точки
            const float t = static_cast<float>(i) / static_cast<float>(config_.dotsCount - 1);
            const auto alpha = static_cast<sf::Uint8>(50 + (1.f - t) * 205.f);

            dots_[i].setPosition(pos);
            dots_[i].setFillColor(sf::Color(255, 255, 255, alpha));
        }
    }

    std::vector<sf::Drawable *> Loader::Drawables()
    {
        return drawables_;
    }

}
