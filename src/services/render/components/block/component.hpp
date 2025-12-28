#pragma once

#include "../[base_component].hpp"

#include <event_system.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <memory>
#include <vector>
#include <optional>
#include <algorithm>
#include <cmath>

namespace Core::App::Render::UI::Components {

    // Rounded rectangle implemented as custom sf::Shape
    class RoundedRectShape final : public sf::Shape
    {
    public:
        struct Radius
        {
            float tl { 0.f };
            float tr { 0.f };
            float br { 0.f };
            float bl { 0.f };
        };

    private:
        sf::Vector2f size_ { 0.f, 0.f };
        Radius radius_ {};
        unsigned int cornerPointCount_ { 10 };

    public:
        RoundedRectShape() = default;

        explicit RoundedRectShape(const sf::Vector2f& size, const float radius, const unsigned int cornerPoints = 10)
            : size_(size), radius_{radius, radius, radius, radius}, cornerPointCount_(std::max(2u, cornerPoints))
        {
            update();
        }

        void SetSize(const sf::Vector2f& size)
        {
            size_ = size;
            update();
        }

        void SetRadius(const Radius& r)
        {
            radius_ = r;
            update();
        }

        void SetRadiusUniform(const float r)
        {
            radius_ = { r, r, r, r };
            update();
        }

        void SetCornerPointCount(const unsigned int n)
        {
            cornerPointCount_ = std::max(2u, n);
            update();
        }

        [[nodiscard]] const sf::Vector2f& GetSize() const { return size_; }
        [[nodiscard]] const Radius& GetRadius() const { return radius_; }
        [[nodiscard]] unsigned int GetCornerPointCount() const { return cornerPointCount_; }

        [[nodiscard]] std::size_t getPointCount() const override;
        [[nodiscard]] sf::Vector2f getPoint(std::size_t index) const override;

    private:
        [[nodiscard]] float ClampRadius(float r) const;
    };

    class Block final : public BaseComponent
    {
    public:
        enum class Event
        {
            HoverEnter,
            HoverLeave,
            Press,
            Release,
            Click
        };

        using EventsSystem = Utils::Event::System<Event>;

        struct BorderRadius
        {
            float tl { 0.f };
            float tr { 0.f };
            float br { 0.f };
            float bl { 0.f };

            static BorderRadius Uniform(const float r) { return { r, r, r, r }; }
        };

        struct ShadowStyle
        {
            bool enabled { false };
            sf::Vector2f offset { 0.f, 6.f };
            float radius { 16.f };                // “мягкость” (по факту — расширение + альфа)
            sf::Color color { sf::Color(0, 0, 0, 120) };
        };

        struct Style
        {
            sf::Color background { sf::Color(40, 40, 40, 230) };

            float borderThickness { 0.f };
            sf::Color borderColor { sf::Color::Transparent };

            BorderRadius radius { BorderRadius::Uniform(0.f) };
            unsigned int radiusQuality { 12 }; // сколько сегментов на угол (8..24 обычно)

            ShadowStyle shadow {};
        };

        struct Anim
        {
            // Smooth transition between states (CSS-like)
            bool enableStyleTransitions { true };
            float transitionSec { 0.12f };

            // Fade-in on spawn
            bool enableFadeIn { false };
            float fadeInSec { 0.25f };

            // “breathing” scale
            bool enablePulseScale { false };
            float pulseAmplitude { 0.03f }; // 3%
            float pulseSpeed { 1.5f };      // cycles/sec

            // slight hover scale
            bool enableHoverScale { true };
            float hoverScale { 1.02f };
        };

        struct Config
        {
            // position is CENTER (как у кнопки)
            sf::Vector2f position { Width / 2.f, Height / 2.f };
            sf::Vector2f size { 420.f, 240.f };

            bool enabled { true };

            sf::Mouse::Button mouseButton { sf::Mouse::Left };

            Style normal {};
            Style hover {};
            Style pressed {};
            Style disabled {};

            Anim anim {};
        };

        struct StyleState
        {
            Style normal {};
            Style hover {};
            Style pressed {};
            Style disabled {};
        };

    public:
        using Shared = std::shared_ptr<Block>;

        explicit Block(const Config& cfg);

        void Update() override;
        std::vector<sf::Drawable*> Drawables() override;

        void Update(const sf::RenderWindow& window);
        void HandleEvent(const sf::Event& event, const sf::RenderWindow& window);

        EventsSystem& Events() { return events_; }

        // Layout
        void SetWindow(const sf::RenderWindow* window);

        void SetEnabled(bool value);
        void SetPosition(const sf::Vector2f& pos);
        void SetSize(const sf::Vector2f& size);

        [[nodiscard]] const sf::Vector2f& Position() const { return config_.position; }
        [[nodiscard]] const sf::Vector2f& Size() const { return config_.size; }

        [[nodiscard]] bool IsEnabled() const { return config_.enabled; }
        [[nodiscard]] bool IsHovered() const { return hovered_; }
        [[nodiscard]] bool IsPressed() const { return pressed_; }

        [[nodiscard]] sf::FloatRect Bounds() const;

        // Styles
        void SetStyles(const Style& normal,
                       const Style& hover,
                       const Style& pressed,
                       const Style& disabled);

        void SetStyleState(const StyleState& st) { SetStyles(st.normal, st.hover, st.pressed, st.disabled); }

        // Optional: “selected” mode (like tabs). Selected overrides normal/hover.
        void SetSelected(bool value);
        void SetSelectedStyle(const Style& style);

        // Runtime effects
        void ResetAnimations(); // restart fade/pulse timers

        static Shared Create(const Config& cfg)
        {
            return std::make_shared<Block>(cfg);
        }

    private:
        enum class VisualState { Normal, Hover, Pressed, Disabled };

        [[nodiscard]] bool HitTest(const sf::RenderWindow& window) const;

        void ApplyStateFromInput(const sf::RenderWindow& window);
        void ApplyTargetStyle(const Style& s);     // set immediately
        void ApplyInterpolatedStyle(const Style& from, const Style& to, float t);

        void UpdateShadowFromStyle(const Style& s);
        void UpdateShapeGeometryFromStyle(const Style& s);
        void UpdateTransforms();

        [[nodiscard]] VisualState CurrentVisualState() const;
        [[nodiscard]] const Style& TargetStyle() const;

        // helpers
        static sf::Color LerpColor(const sf::Color& a, const sf::Color& b, float t);
        static float LerpFloat(float a, float b, float t);
        static BorderRadius LerpRadius(const BorderRadius& a, const BorderRadius& b, float t);

    private:
        Config config_;

        // visuals
        RoundedRectShape shape_;
        RoundedRectShape shadowShape_;

        std::vector<sf::Drawable*> drawables_;

        // state
        bool hovered_ { false };
        bool pressed_ { false };
        bool pressedInside_ { false };
        bool prevMouseDown_ { false };

        bool selected_ { false };
        std::optional<Style> selectedStyle_;

        const sf::RenderWindow* window_ { nullptr };
        EventsSystem events_;

        // animation
        sf::Clock tickClock_;
        sf::Clock fadeClock_;
        sf::Clock pulseClock_;

        // style transition
        bool transitioning_ { false };
        float transitionT_ { 1.f };
        Style transitionFrom_ {};
        Style transitionTo_ {};
    };

} // namespace Core::App::Render::UI::Components