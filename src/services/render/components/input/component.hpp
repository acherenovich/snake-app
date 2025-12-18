#pragma once

#include "../[base_component].hpp"

#include <event_system.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System/String.hpp>

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <algorithm>
#include <cctype>
#include <limits>

namespace Core::App::Render::UI::Components {

    class Input final : public BaseComponent
    {
    public:
        enum class Event
        {
            Changed,
            Submit,
            Focus,
            Blur
        };

        using EventsSystem = Utils::Event::System<Event>;

        struct TextStyle
        {
            sf::Color color { sf::Color::White };
            unsigned int size { 22 };
            sf::Uint32 sfmlStyle { sf::Text::Regular };

            bool pulseAlpha { false };
            float pulseSpeed { 2.0f };
            sf::Uint8 pulseMin { 160 };
            sf::Uint8 pulseMax { 255 };
        };

        struct BoxStyle
        {
            sf::Color background { sf::Color(40, 40, 40, 220) };
            sf::Color borderColor { sf::Color(255, 255, 255, 40) };
            float borderThickness { 2.f };
            float radiusHint { 0.f };
        };

        struct SelectionStyle
        {
            sf::Color color { sf::Color(120, 160, 255, 120) };
        };

        struct CaretStyle
        {
            sf::Color color { sf::Color::White };
            float width { 2.f };
            float blinkPeriodSec { 0.9f };
            float paddingTopBottom { 6.f };
        };

        struct Config
        {
            // geometry
            sf::Vector2f position { 0.f, 0.f }; // left-top
            sf::Vector2f size { 420.f, 54.f };
            float paddingX { 14.f };

            // font
            std::string font;
            bool requireFont { true };

            // content (UTF-8)
            std::string placeholder;
            std::string value;

            bool enabled { true };
            bool password { false };

            // password mask (UTF-8, single glyph recommended)
            std::string passwordMask { "•" };

            // maxLength in characters (not bytes!)
            std::size_t maxLength { 128 };

            // validation
            bool allowSpaces { true };
            bool allowNewLine { false };
            bool allowLetters { true };
            bool allowDigits { true };
            bool allowUnderscore { true };
            bool allowDash { true };
            bool allowDot { true };
            bool allowAt { true };
            bool allowOther { true };

            // styles
            BoxStyle normalBox {};
            BoxStyle hoverBox { .background = sf::Color(45, 45, 45, 230), .borderColor = sf::Color(255,255,255,70), .borderThickness = 2.f };
            BoxStyle focusBox { .background = sf::Color(50, 50, 55, 240), .borderColor = sf::Color(120,160,255,180), .borderThickness = 2.f };
            BoxStyle disabledBox { .background = sf::Color(30, 30, 30, 140), .borderColor = sf::Color(255,255,255,20), .borderThickness = 2.f };

            TextStyle textStyle {};
            TextStyle placeholderStyle { .color = sf::Color(255,255,255,120), .size = 22, .sfmlStyle = sf::Text::Regular, .pulseAlpha = false };

            SelectionStyle selectionStyle {};
            CaretStyle caretStyle {};

            // behavior
            bool submitOnEnter { true };
            bool clearOnSubmit { false };

            bool verticalCenterText { true };
        };

        using Shared = std::shared_ptr<Input>;

    public:
        explicit Input(Config cfg);

        // BaseComponent
        void Update() override;
        std::vector<sf::Drawable*> Drawables() override;

    public:
        void Update(const sf::RenderWindow& window);
        void HandleEvent(const sf::Event& e, const sf::RenderWindow& window);

        // API (UTF-8)
        [[nodiscard]] std::string Value() const;
        void SetValue(std::string v);

        [[nodiscard]] bool IsFocused() const { return focused_; }
        void SetFocused(bool focused);

        void SetEnabled(bool enabled);

        EventsSystem& Events() { return events_; }
        [[nodiscard]] const Config& GetConfig() const { return config_; }

        static Shared Create(Config cfg)
        {
            return std::make_shared<Input>(std::move(cfg));
        }

    private:
        enum class VisualState { Normal, Hover, Focus, Disabled };

        void RebuildTextObjects();
        void UpdateLayout();
        void UpdatePlaceholderVisibility();
        void ApplyVisualState(VisualState st);

        [[nodiscard]] bool ContainsPoint(const sf::RenderWindow& window, int px, int py) const;

        [[nodiscard]] bool IsAllowedChar(char32_t u) const;

        // text ops (UTF-32)
        void InsertUtf32(char32_t u);
        void Backspace();
        void Delete();
        void MoveCaretLeft(bool ctrl);
        void MoveCaretRight(bool ctrl);
        void MoveCaretHome();
        void MoveCaretEnd();
        void SelectAll();
        void ClearSelection();
        [[nodiscard]] bool HasSelection() const;
        void DeleteSelection();

        void EnsureCaretVisible();
        void UpdateCaretVisual();
        void UpdateSelectionVisual();

        [[nodiscard]] sf::String VisibleString() const;
        [[nodiscard]] sf::String MaskString(std::size_t count) const;

        [[nodiscard]] std::size_t PrevWordBoundary(std::size_t pos) const;
        [[nodiscard]] std::size_t NextWordBoundary(std::size_t pos) const;

        [[nodiscard]] float TextXForIndex(std::size_t index) const;

    private:
        Config config_;

        // state
        bool hovered_ { false };
        bool focused_ { false };
        bool enabled_ { true };

        // UTF-32 storage
        sf::String value32_ {};
        sf::String placeholder32_ {};
        sf::String mask32_ {}; // passwordMask in UTF-32

        std::size_t caret_ { 0 }; // in characters

        // selection
        std::optional<std::size_t> selectionAnchor_ {};

        // horizontal scroll
        float scrollX_ { 0.f };

        // visuals
        sf::RectangleShape box_;
        sf::Text text_;
        sf::Text placeholderText_;
        sf::RectangleShape caretRect_;
        sf::RectangleShape selectionRect_;

        sf::Font font_;

        // caret blink
        sf::Clock caretClock_;
        bool caretVisible_ { true };

        // placeholder pulse
        sf::Clock pulseClock_;

        std::vector<sf::Drawable*> drawables_;

        EventsSystem events_;
    };

} // namespace Core::App::Render::UI::Components