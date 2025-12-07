#pragma once

#include "../[base_component].hpp"

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cmath>

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
            // Базовый текст
            std::string text { "Text" };
            std::string font { "assets/fonts/Roboto-Regular.ttf" };   // ОБЯЗАТЕЛЬНО задать снаружи
            unsigned int characterSize { 32 };

            // Позиция и выравнивание
            sf::Vector2f position {
                Width  / 2.f,
                Height / 2.f
            };

            HAlign hAlign { HAlign::Center };
            VAlign vAlign { VAlign::Center };

            // Цвет
            sf::Color color { sf::Color::White };

            // Эффект плавного появления (fade-in)
            bool  enableFadeIn    { false };
            float fadeInDuration  { 0.5f }; // секунды

            // Пульсация масштаба (легкое “дышание” текста)
            bool  enablePulseScale      { false };
            float pulseScaleAmplitude   { 0.08f };  // +8%
            float pulseScaleSpeed       { 2.0f };   // циклов в секунду

            // Пульсация альфы
            bool  enablePulseAlpha      { false };
            float pulseAlphaMin         { 0.5f };   // минимальный множитель альфы (0..1)
            float pulseAlphaSpeed       { 0.5f };   // циклов в секунду
        };

    private:
        Config config_;

        sf::Text text_;
        sf::Font font_;
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