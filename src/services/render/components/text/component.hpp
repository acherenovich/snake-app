#pragma once

#include "../[base_component].hpp"

#include <SFML/Graphics.hpp>

#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace Core::App::Render::UI::Components {

    class Text final : public BaseComponent
    {
    public:
        enum class HAlign
        {
            Left,
            Center,
            Right
        };

        enum class VAlign
        {
            Top,
            Center,
            Bottom
        };

        struct Config
        {
            std::string text { "Text" };
            std::string font { "assets/fonts/Roboto-Regular.ttf" };
            unsigned int characterSize { 32 };

            sf::Vector2f position { Width / 2.f, Height / 2.f };

            HAlign hAlign { HAlign::Center };
            VAlign vAlign { VAlign::Center };

            sf::Color color { sf::Color::White };

            bool  enableFadeIn    { false };
            float fadeInDuration  { 0.5f };

            bool  enablePulseScale      { false };
            float pulseScaleAmplitude   { 0.08f };
            float pulseScaleSpeed       { 2.0f };

            bool  enablePulseAlpha      { false };
            float pulseAlphaMin         { 0.5f };
            float pulseAlphaSpeed       { 0.5f };
        };

    private:
        Config config_;

        sf::Font font_;
        sf::Text text_;        // ⚠️ SFML3: must be constructed with font
        sf::Clock clock_;

        float elapsed_ { 0.f };

        std::vector<sf::Drawable*> drawables_;

    public:
        using Shared = std::shared_ptr<Text>;

        explicit Text(const Config& cfg);

        void Update() override;
        std::vector<sf::Drawable*> Drawables() override;

        [[nodiscard]] const Config& GetConfig() const { return config_; }

        void SetText(const std::string& value);
        void SetPosition(const sf::Vector2f& pos);
        void SetColor(const sf::Color& color);
        void SetAlignment(HAlign h, VAlign v);
        void ResetTimer();

        static Shared Create(const Config& cfg)
        {
            return std::make_shared<Text>(cfg);
        }

    private:
        void ApplyAlignment();
        void ApplyEffects();
    };

} // namespace Core::App::Render::UI::Components
