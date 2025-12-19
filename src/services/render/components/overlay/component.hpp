#pragma once

#include "../[base_component].hpp"

#include <event_system.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <memory>
#include <vector>

namespace Core::App::Render::UI::Components {

    class Overlay final : public BaseComponent
    {
    public:
        enum class Event
        {
            Click,      // клик по затемнению
            Consumed    // событие было “съедено”
        };

        using EventsSystem = Utils::Event::System<Event>;
        using Shared = std::shared_ptr<Overlay>;

        struct Config
        {
            bool enabled { true };

            // Рисуем поверх всего: прямоугольник на весь экран (обычно под view)
            sf::Vector2f position { 0.f, 0.f };
            sf::Vector2f size { Width, Height };

            // затемнение/цвет
            sf::Color color { sf::Color(0, 0, 0, 140) };

            // “blur-like” эффект без шейдеров:
            // рисуем поверх несколько слоёв с разными альфами (простая имитация softness)
            bool enableSoftLayers { false };
            int softLayersCount { 4 };
            float softLayersStep { 2.f };          // расширение прямоугольника на слой
            sf::Uint8 softLayersAlphaStep { 18 };  // прибавка альфы

            // блокировать действия на фоне
            bool blockInput { true };

            // если хочешь закрывать модалку кликом по фону
            bool closeOnClick { false };
            sf::Mouse::Button closeButton { sf::Mouse::Left };

            // если хочешь анимировать появление
            bool enableFadeIn { false };
            float fadeInDuration { 0.15f }; // seconds
        };

    public:
        explicit Overlay(const Config& cfg);

        // BaseComponent
        void Update() override;
        std::vector<sf::Drawable*> Drawables() override;

        // preferred
        void Update(const sf::RenderWindow& window);

        // events
        void HandleEvent(const sf::Event& e, const sf::RenderWindow& window);

        EventsSystem& Events() { return events_; }

        // runtime api
        void SetEnabled(bool enabled);
        [[nodiscard]] bool IsEnabled() const { return config_.enabled; }

        void SetColor(const sf::Color& c);

        void SetPosition(const sf::Vector2f& p);
        void SetSize(const sf::Vector2f& s);

        [[nodiscard]] const Config& GetConfig() const { return config_; }

        static Shared Create(const Config& cfg)
        {
            return std::make_shared<Overlay>(cfg);
        }

    private:
        void RebuildGeometry();
        void ApplyFade();

        [[nodiscard]] bool ContainsPoint(const sf::RenderWindow& window, int px, int py) const;

    private:
        Config config_;

        sf::RectangleShape base_;
        std::vector<sf::RectangleShape> softLayers_;

        std::vector<sf::Drawable*> drawables_;

        sf::Clock clock_;
        float elapsed_ { 0.f };

        EventsSystem events_;
    };

} // namespace Core::App::Render::UI::Components