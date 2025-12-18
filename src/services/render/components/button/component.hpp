#pragma once

#include "../[base_component].hpp"

#include <SFML/Graphics.hpp>
#include <event_system.hpp>

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>

namespace Core::App::Render::UI::Components {

    class Button final : public BaseComponent
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

        enum class HAlign { Left, Center, Right };
        enum class VAlign { Top, Center, Bottom };

        struct TextStyle
        {
            sf::Color color { sf::Color::White };
            unsigned int size { 22 };
            sf::Uint32 sfmlStyle { sf::Text::Regular };

            HAlign hAlign { HAlign::Center };
            VAlign vAlign { VAlign::Center };

            sf::Vector2f offset { 0.f, 0.f };
        };

        struct Style
        {
            sf::Color background { sf::Color(40, 40, 40) };

            float borderThickness { 0.f };
            sf::Color borderColor { sf::Color::Transparent };

            sf::Vector2f padding { 14.f, 10.f };

            TextStyle text {};
        };

        struct Config
        {
            sf::Vector2f position { Width / 2.f, Height / 2.f };
            sf::Vector2f size { 220.f, 54.f };

            std::string text { "Button" };
            std::string font { "assets/fonts/Roboto-Regular.ttf" };

            bool enabled { true };

            Style normal {};
            Style hover {};
            Style pressed {};
            Style disabled {};

            sf::Mouse::Button mouseButton { sf::Mouse::Left };
        };

        struct StyleState
        {
            Style normal {};
            Style hover {};
            Style pressed {};
            Style disabled {};
        };

    public:
        using Shared = std::shared_ptr<Button>;
        using EventsSystem = Utils::Event::System<Event>;

    private:
        Config config_;

        // visuals
        sf::RectangleShape rect_;
        sf::Text label_;
        sf::Font font_;
        std::vector<sf::Drawable*> drawables_;

        // state
        bool hovered_ { false };
        bool pressed_ { false };
        bool pressedInside_ { false };

        bool selected_ { false };
        std::optional<Style> selectedStyle_;

        EventsSystem events_;

    public:
        explicit Button(const Config& cfg);

        // BaseComponent
        void Update() override; // noop
        std::vector<sf::Drawable*> Drawables() override;

        // preferred
        void Update(const sf::RenderWindow& window);
        void HandleEvent(const sf::Event& e, const sf::RenderWindow& window);

        // events
        EventsSystem& Events() { return events_; }

        // runtime api (layout)
        void SetEnabled(bool value);
        void SetText(const std::string& value);

        void SetPosition(const sf::Vector2f& pos);
        void SetSize(const sf::Vector2f& size);

        // styles api
        void SetStyles(const Style& normal,
                       const Style& hover,
                       const Style& pressed,
                       const Style& disabled);

        void SetStyleState(const StyleState& st)
        {
            SetStyles(st.normal, st.hover, st.pressed, st.disabled);
        }

        // selected (tabs)
        void SetSelected(bool value);
        void SetSelectedStyle(const Style& style);
        void ClearSelectedStyle();

        [[nodiscard]] bool IsHovered() const { return hovered_; }
        [[nodiscard]] bool IsPressed() const { return pressed_; }
        [[nodiscard]] bool IsEnabled() const { return config_.enabled; }
        [[nodiscard]] bool IsSelected() const { return selected_; }

        [[nodiscard]] sf::FloatRect Bounds() const;

        static Shared Create(const Config& cfg)
        {
            return std::make_shared<Button>(cfg);
        }

    private:
        void ApplyStyle(const Style& style);
        void ApplyLabelAlignment(const TextStyle& ts);

        [[nodiscard]] bool HitTest(const sf::RenderWindow& window, int px, int py) const;
        [[nodiscard]] bool HitTestMouse(const sf::RenderWindow& window) const;

        void SetHovered(bool value);

        void ApplyCurrentStyle();
    };

} // namespace Core::App::Render::UI::Components