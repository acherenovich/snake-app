#include "component.hpp"

#include <algorithm>

namespace Core::App::Render::UI::Components {

    Overlay::Overlay(const Config& cfg)
        : config_(cfg)
    {
        clock_.restart();
        elapsed_ = 0.f;

        RebuildGeometry();

        drawables_.push_back(&base_);
        for (auto& r : softLayers_)
            drawables_.push_back(&r);

        ApplyFade();
    }

    void Overlay::Update() {}

    std::vector<sf::Drawable*> Overlay::Drawables()
    {
        // если выключен — не рисуем ничего
        if (!config_.enabled)
            return {};

        return drawables_;
    }

    void Overlay::Update(const sf::RenderWindow& /*window*/)
    {
        if (!config_.enabled)
            return;

        const float dt = clock_.restart().asSeconds();
        elapsed_ += dt;

        ApplyFade();
    }

    void Overlay::HandleEvent(const sf::Event& e, const sf::RenderWindow& window)
    {
        if (!config_.enabled)
            return;

        // если overlay не блокирует инпут — не трогаем события
        if (!config_.blockInput && !config_.closeOnClick)
            return;

        // close on click (по затемнению)
        if (config_.closeOnClick &&
            e.type == sf::Event::MouseButtonPressed &&
            e.mouseButton.button == config_.closeButton)
        {
            if (ContainsPoint(window, e.mouseButton.x, e.mouseButton.y))
            {
                events_.CallEvent(Event::Click);
                events_.CallEvent(Event::Consumed);
                if (config_.blockInput)
                    return;
            }
        }

        // блокируем все фоновые события (съедаем)
        if (config_.blockInput)
        {
            // Если хочешь — можно оставить Escape наружу, но пока “жёстко блокируем”
            events_.CallEvent(Event::Consumed);
        }
    }

    void Overlay::SetEnabled(const bool enabled)
    {
        if (config_.enabled == enabled)
            return;

        config_.enabled = enabled;
        elapsed_ = 0.f;
        clock_.restart();

        ApplyFade();
    }

    void Overlay::SetColor(const sf::Color& c)
    {
        config_.color = c;
        RebuildGeometry();
        ApplyFade();
    }

    void Overlay::SetPosition(const sf::Vector2f& p)
    {
        config_.position = p;
        RebuildGeometry();
        ApplyFade();
    }

    void Overlay::SetSize(const sf::Vector2f& s)
    {
        config_.size = s;
        RebuildGeometry();
        ApplyFade();
    }

    void Overlay::RebuildGeometry()
    {
        base_.setPosition(config_.position);
        base_.setSize(config_.size);
        base_.setFillColor(config_.color);

        softLayers_.clear();
        if (!config_.enableSoftLayers || config_.softLayersCount <= 0)
            return;

        softLayers_.reserve(static_cast<std::size_t>(config_.softLayersCount));

        for (int i = 0; i < config_.softLayersCount; ++i)
        {
            const float grow = config_.softLayersStep * static_cast<float>(i + 1);

            auto r = sf::RectangleShape();
            r.setPosition({ config_.position.x - grow, config_.position.y - grow });
            r.setSize({ config_.size.x + grow * 2.f, config_.size.y + grow * 2.f });

            sf::Color c = config_.color;
            const int a = static_cast<int>(c.a) + static_cast<int>(config_.softLayersAlphaStep) * (i + 1);
            c.a = static_cast<sf::Uint8>(std::clamp(a, 0, 255));
            r.setFillColor(c);

            softLayers_.push_back(r);
        }
    }

    void Overlay::ApplyFade()
    {
        if (!config_.enableFadeIn || config_.fadeInDuration <= 0.f)
            return;

        const float t = std::min(elapsed_ / config_.fadeInDuration, 1.f);

        // умножаем альфу на t
        auto apply = [t](sf::Color c) -> sf::Color {
            const auto a = static_cast<sf::Uint8>(static_cast<float>(c.a) * t);
            c.a = a;
            return c;
        };

        base_.setFillColor(apply(config_.color));

        if (config_.enableSoftLayers)
        {
            // мягкие слои тоже фейдим
            for (int i = 0; i < static_cast<int>(softLayers_.size()); ++i)
            {
                // берём текущий цвет слоя и домножаем его альфу ещё раз
                auto c = softLayers_[i].getFillColor();
                c.a = static_cast<sf::Uint8>(static_cast<float>(c.a) * t);
                softLayers_[i].setFillColor(c);
            }
        }
    }

    bool Overlay::ContainsPoint(const sf::RenderWindow& window, int px, int py) const
    {
        const sf::Vector2f p = window.mapPixelToCoords({ px, py });
        const sf::FloatRect r(config_.position, config_.size);
        return r.contains(p);
    }

} // namespace Core::App::Render::UI::Components