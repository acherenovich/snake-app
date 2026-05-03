#include "component.hpp"

namespace Core::App::Render::UI::Components {

    Text::Text(const Config& cfg)
        : config_(cfg)
        , text_(font_) // ✅ SFML3: text needs font ref
    {
        font_.openFromFile(config_.font);

        text_.setString(config_.text);
        text_.setCharacterSize(config_.characterSize);
        text_.setFillColor(config_.color);

        ApplyAlignment();

        drawables_.push_back(&text_);

        clock_.restart();
        elapsed_ = 0.f;
    }

    void Text::Update()
    {
        const float dt = clock_.restart().asSeconds();
        elapsed_ += dt;

        ApplyEffects();
        ApplyAlignment();
    }

    std::vector<sf::Drawable*> Text::Drawables()
    {
        return drawables_;
    }

    void Text::SetText(const std::string& value)
    {
        config_.text = value;
        text_.setString(config_.text);
        ApplyAlignment();
    }

    void Text::SetPosition(const sf::Vector2f& pos)
    {
        config_.position = pos;
        ApplyAlignment();
    }

    void Text::SetColor(const sf::Color& color)
    {
        config_.color = color;
        text_.setFillColor(config_.color);
    }

    void Text::SetAlignment(HAlign h, VAlign v)
    {
        config_.hAlign = h;
        config_.vAlign = v;
        ApplyAlignment();
    }

    void Text::ResetTimer()
    {
        elapsed_ = 0.f;
        clock_.restart();
    }

    void Text::ApplyAlignment()
    {
        const sf::FloatRect bounds = text_.getLocalBounds();

        sf::Vector2f origin { 0.f, 0.f };

        // Horizontal
        switch (config_.hAlign)
        {
            case HAlign::Left:
                origin.x = bounds.position.x;
                break;
            case HAlign::Center:
                origin.x = bounds.position.x + bounds.size.x * 0.5f;
                break;
            case HAlign::Right:
                origin.x = bounds.position.x + bounds.size.x;
                break;
        }

        // Vertical
        switch (config_.vAlign)
        {
            case VAlign::Top:
                origin.y = bounds.position.y;
                break;
            case VAlign::Center:
                origin.y = bounds.position.y + bounds.size.y * 0.5f;
                break;
            case VAlign::Bottom:
                origin.y = bounds.position.y + bounds.size.y;
                break;
        }

        text_.setOrigin(origin);
        text_.setPosition(config_.position);
    }

    void Text::ApplyEffects()
    {
        sf::Color color = config_.color;

        float alphaFactor = 1.f;
        float scaleFactor = 1.f;

        // Fade-in
        if (config_.enableFadeIn && config_.fadeInDuration > 0.f)
        {
            const float t = std::min(elapsed_ / config_.fadeInDuration, 1.f);
            alphaFactor *= t;
        }

        // Pulse scale
        if (config_.enablePulseScale)
        {
            constexpr float PI = 3.1415926535f;
            const float s = std::sin(2.f * PI * config_.pulseScaleSpeed * elapsed_);
            scaleFactor *= 1.f + config_.pulseScaleAmplitude * s;
        }

        // Pulse alpha
        if (config_.enablePulseAlpha)
        {
            constexpr float PI = 3.1415926535f;
            const float s = std::sin(2.f * PI * config_.pulseAlphaSpeed * elapsed_);
            const float norm = 0.5f * (s + 1.f);
            const float k = config_.pulseAlphaMin + (1.f - config_.pulseAlphaMin) * norm;
            alphaFactor *= k;
        }

        alphaFactor = std::clamp(alphaFactor, 0.f, 1.f);

        const auto newAlpha = static_cast<std::uint8_t>(
            static_cast<float>(color.a) * alphaFactor
        );

        color.a = newAlpha;
        text_.setFillColor(color);

        // ✅ SFML3: setScale only vector
        text_.setScale({ scaleFactor, scaleFactor });
    }

} // namespace Core::App::Render::UI::Components
