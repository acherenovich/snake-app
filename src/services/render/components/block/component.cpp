#include "component.hpp"

namespace Core::App::Render::UI::Components {

    // =========================
    // RoundedRectShape
    // =========================

    float RoundedRectShape::ClampRadius(float r) const
    {
        const float maxR = std::min(size_.x, size_.y) * 0.5f;
        return std::clamp(r, 0.f, maxR);
    }

    std::size_t RoundedRectShape::getPointCount() const
    {
        return static_cast<std::size_t>(cornerPointCount_) * 4u;
    }

    sf::Vector2f RoundedRectShape::getPoint(std::size_t index) const
    {
        const unsigned int cp = cornerPointCount_;
        const unsigned int corner = static_cast<unsigned int>(index / cp);
        const unsigned int i = static_cast<unsigned int>(index % cp);
        const float t = static_cast<float>(i) / static_cast<float>(cp - 1);

        const float tl = ClampRadius(radius_.tl);
        const float tr = ClampRadius(radius_.tr);
        const float br = ClampRadius(radius_.br);
        const float bl = ClampRadius(radius_.bl);

        constexpr float PI = 3.1415926535f;

        float angle0 = 0.f;
        float r = 0.f;
        sf::Vector2f c { 0.f, 0.f };

        switch (corner)
        {
            case 0: // TL: 180..270
                angle0 = PI;
                r = tl;
                c = { tl, tl };
                break;
            case 1: // TR: 270..360
                angle0 = 1.5f * PI;
                r = tr;
                c = { size_.x - tr, tr };
                break;
            case 2: // BR: 0..90
                angle0 = 0.f;
                r = br;
                c = { size_.x - br, size_.y - br };
                break;
            default: // BL: 90..180
                angle0 = 0.5f * PI;
                r = bl;
                c = { bl, size_.y - bl };
                break;
        }

        const float angle = angle0 + t * (0.5f * PI);
        return { c.x + std::cos(angle) * r, c.y + std::sin(angle) * r };
    }

    // =========================
    // Block
    // =========================

    static RoundedRectShape::Radius ToShapeRadius(const Block::BorderRadius& r)
    {
        return { r.tl, r.tr, r.br, r.bl };
    }

    sf::Color Block::LerpColor(const sf::Color& a, const sf::Color& b, float t)
    {
        const auto L = [&](uint8_t x, uint8_t y) -> uint8_t {
            const float v = static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t;
            return static_cast<uint8_t>(std::clamp(v, 0.f, 255.f));
        };
        return { L(a.r, b.r), L(a.g, b.g), L(a.b, b.b), L(a.a, b.a) };
    }

    float Block::LerpFloat(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    Block::BorderRadius Block::LerpRadius(const BorderRadius& a, const BorderRadius& b, float t)
    {
        return {
            LerpFloat(a.tl, b.tl, t),
            LerpFloat(a.tr, b.tr, t),
            LerpFloat(a.br, b.br, t),
            LerpFloat(a.bl, b.bl, t),
        };
    }

    Block::Block(const Config& cfg)
        : config_(cfg)
    {
        shape_.SetSize(config_.size);
        shape_.SetCornerPointCount(std::max(2u, config_.normal.radiusQuality));
        shape_.SetRadius(ToShapeRadius(config_.normal.radius));
        shape_.setOrigin({ config_.size.x * 0.5f, config_.size.y * 0.5f });
        shape_.setPosition(config_.position);

        shadowShape_.SetSize(config_.size);
        shadowShape_.SetCornerPointCount(std::max(2u, config_.normal.radiusQuality));
        shadowShape_.SetRadius(ToShapeRadius(config_.normal.radius));
        shadowShape_.setOrigin({ config_.size.x * 0.5f, config_.size.y * 0.5f });
        shadowShape_.setPosition(config_.position);

        drawables_.push_back(&shadowShape_);
        drawables_.push_back(&shape_);

        tickClock_.restart();
        fadeClock_.restart();
        pulseClock_.restart();

        ApplyTargetStyle(TargetStyle());
        ResetAnimations();
    }

    void Block::ResetAnimations()
    {
        fadeClock_.restart();
        pulseClock_.restart();

        transitioning_ = false;
        transitionT_ = 1.f;
        transitionFrom_ = TargetStyle();
        transitionTo_ = TargetStyle();
    }

    void Block::SetWindow(const sf::RenderWindow* window)
    {
        window_ = window;
    }

    void Block::SetEnabled(bool value)
    {
        config_.enabled = value;
        hovered_ = false;
        pressed_ = false;
        pressedInside_ = false;
        prevMouseDown_ = false;

        const auto& to = TargetStyle();

        if (config_.anim.enableStyleTransitions)
        {
            transitioning_ = true;
            transitionT_ = 0.f;
            transitionFrom_ = transitionTo_;
            transitionTo_ = to;
        }
        else
        {
            ApplyTargetStyle(to);
        }
    }

    void Block::SetPosition(const sf::Vector2f& pos)
    {
        config_.position = pos;
        shape_.setPosition(config_.position);
        shadowShape_.setPosition(config_.position);
    }

    void Block::SetSize(const sf::Vector2f& size)
    {
        config_.size = size;

        shape_.SetSize(config_.size);
        shape_.setOrigin({ config_.size.x * 0.5f, config_.size.y * 0.5f });

        shadowShape_.SetSize(config_.size);
        shadowShape_.setOrigin({ config_.size.x * 0.5f, config_.size.y * 0.5f });

        ApplyTargetStyle(TargetStyle());
    }

    sf::FloatRect Block::Bounds() const
    {
        return shape_.getGlobalBounds();
    }

    void Block::SetStyles(const Style& normal,
                          const Style& hover,
                          const Style& pressed,
                          const Style& disabled)
    {
        config_.normal = normal;
        config_.hover = hover;
        config_.pressed = pressed;
        config_.disabled = disabled;

        const auto& to = TargetStyle();

        if (config_.anim.enableStyleTransitions)
        {
            transitioning_ = true;
            transitionT_ = 0.f;
            transitionFrom_ = transitionTo_;
            transitionTo_ = to;
        }
        else
        {
            ApplyTargetStyle(to);
        }
    }

    void Block::SetSelected(bool value)
    {
        selected_ = value;

        const auto& to = TargetStyle();

        if (config_.anim.enableStyleTransitions)
        {
            transitioning_ = true;
            transitionT_ = 0.f;
            transitionFrom_ = transitionTo_;
            transitionTo_ = to;
        }
        else
        {
            ApplyTargetStyle(to);
        }
    }

    void Block::SetSelectedStyle(const Style& style)
    {
        selectedStyle_ = style;

        const auto& to = TargetStyle();

        if (config_.anim.enableStyleTransitions)
        {
            transitioning_ = true;
            transitionT_ = 0.f;
            transitionFrom_ = transitionTo_;
            transitionTo_ = to;
        }
        else
        {
            ApplyTargetStyle(to);
        }
    }

    void Block::Update()
    {
        if (!window_)
            return;

        Update(*window_);
    }

    std::vector<sf::Drawable*> Block::Drawables()
    {
        return drawables_;
    }

    bool Block::HitTest(const sf::RenderWindow& window, int px, int py) const
    {
        const sf::Vector2f p = window.mapPixelToCoords({ px, py });
        return shape_.getGlobalBounds().contains(p);
    }

    bool Block::HitTest(const sf::RenderWindow& window) const
    {
        const auto pixel = sf::Mouse::getPosition(window);
        return HitTest(window, pixel.x, pixel.y);
    }

    Block::VisualState Block::CurrentVisualState() const
    {
        if (!config_.enabled) return VisualState::Disabled;
        if (pressed_) return VisualState::Pressed;
        if (hovered_) return VisualState::Hover;
        return VisualState::Normal;
    }

    const Block::Style& Block::TargetStyle() const
    {
        if (!config_.enabled) return config_.disabled;

        if (selected_ && selectedStyle_ && !pressed_)
            return *selectedStyle_;

        switch (CurrentVisualState())
        {
            case VisualState::Pressed: return config_.pressed;
            case VisualState::Hover: return config_.hover;
            case VisualState::Disabled: return config_.disabled;
            default: return config_.normal;
        }
    }

    void Block::HandleEvent(const sf::Event& event, const sf::RenderWindow& window)
    {
        if (!config_.enabled)
            return;

        if (const auto* mm = event.getIf<sf::Event::MouseMoved>())
        {
            const bool nowHovered = HitTest(window, mm->position.x, mm->position.y);

            if (nowHovered && !hovered_)
            {
                hovered_ = true;
                events_.CallEvent(Event::HoverEnter);
            }
            else if (!nowHovered && hovered_)
            {
                hovered_ = false;
                events_.CallEvent(Event::HoverLeave);
            }
            return;
        }

        if (const auto* mp = event.getIf<sf::Event::MouseButtonPressed>())
        {
            if (mp->button != config_.mouseButton)
                return;

            const bool inside = HitTest(window, mp->position.x, mp->position.y);

            if (inside)
            {
                pressed_ = true;
                pressedInside_ = true;
                events_.CallEvent(Event::Press);
            }
            else
            {
                pressed_ = false;
                pressedInside_ = false;
            }
            return;
        }

        if (const auto* mr = event.getIf<sf::Event::MouseButtonReleased>())
        {
            if (mr->button != config_.mouseButton)
                return;

            const bool inside = HitTest(window, mr->position.x, mr->position.y);

            if (pressed_)
            {
                pressed_ = false;
                events_.CallEvent(Event::Release);

                if (pressedInside_ && inside)
                    events_.CallEvent(Event::Click);
            }

            pressedInside_ = false;
            return;
        }
    }

    void Block::ApplyStateFromInput(const sf::RenderWindow& window)
    {
        if (!config_.enabled)
            return;

        const bool nowHovered = HitTest(window);

        if (nowHovered && !hovered_)
        {
            hovered_ = true;
            events_.CallEvent(Event::HoverEnter);
        }
        else if (!nowHovered && hovered_)
        {
            hovered_ = false;
            events_.CallEvent(Event::HoverLeave);
        }

        const bool mouseDown = sf::Mouse::isButtonPressed(config_.mouseButton);
        const bool mouseUp = (prevMouseDown_ && !mouseDown);
        prevMouseDown_ = mouseDown;

        if (!pressed_ && mouseDown && nowHovered)
        {
            pressed_ = true;
            pressedInside_ = true;
            events_.CallEvent(Event::Press);
        }

        if (pressed_ && mouseDown && !nowHovered)
            pressedInside_ = false;

        if (pressed_ && mouseUp)
        {
            pressed_ = false;
            events_.CallEvent(Event::Release);

            if (pressedInside_ && nowHovered)
                events_.CallEvent(Event::Click);

            pressedInside_ = false;
        }
    }

    void Block::Update(const sf::RenderWindow& window)
    {
        ApplyStateFromInput(window);

        const float dt = tickClock_.restart().asSeconds();
        const auto& to = TargetStyle();

        const bool targetChanged =
            (to.background != transitionTo_.background) ||
            (to.borderThickness != transitionTo_.borderThickness) ||
            (to.borderColor != transitionTo_.borderColor) ||
            (to.radius.tl != transitionTo_.radius.tl) ||
            (to.radius.tr != transitionTo_.radius.tr) ||
            (to.radius.br != transitionTo_.radius.br) ||
            (to.radius.bl != transitionTo_.radius.bl) ||
            (to.radiusQuality != transitionTo_.radiusQuality) ||
            (to.shadow.enabled != transitionTo_.shadow.enabled) ||
            (to.shadow.offset != transitionTo_.shadow.offset) ||
            (to.shadow.radius != transitionTo_.shadow.radius) ||
            (to.shadow.color != transitionTo_.shadow.color);

        if (targetChanged)
        {
            if (config_.anim.enableStyleTransitions)
            {
                transitioning_ = true;
                transitionT_ = 0.f;
                transitionFrom_ = transitionTo_;
                transitionTo_ = to;
            }
            else
            {
                transitioning_ = false;
                transitionT_ = 1.f;
                transitionFrom_ = to;
                transitionTo_ = to;
                ApplyTargetStyle(to);
            }
        }

        if (transitioning_)
        {
            const float dur = std::max(0.001f, config_.anim.transitionSec);
            transitionT_ = std::min(1.f, transitionT_ + dt / dur);
            ApplyInterpolatedStyle(transitionFrom_, transitionTo_, transitionT_);
            if (transitionT_ >= 1.f)
                transitioning_ = false;
        }
        else
        {
            ApplyTargetStyle(to);
        }

        UpdateTransforms();
    }

    void Block::ApplyTargetStyle(const Style& s)
    {
        UpdateShapeGeometryFromStyle(s);

        shape_.setFillColor(s.background);
        shape_.setOutlineThickness(s.borderThickness);
        shape_.setOutlineColor(s.borderColor);

        UpdateShadowFromStyle(s);
    }

    void Block::ApplyInterpolatedStyle(const Style& from, const Style& to, float t)
    {
        Style s{};
        s.background = LerpColor(from.background, to.background, t);
        s.borderThickness = LerpFloat(from.borderThickness, to.borderThickness, t);
        s.borderColor = LerpColor(from.borderColor, to.borderColor, t);
        s.radius = LerpRadius(from.radius, to.radius, t);
        s.radiusQuality = (t < 0.5f) ? from.radiusQuality : to.radiusQuality;

        s.shadow.enabled = (t < 0.5f) ? from.shadow.enabled : to.shadow.enabled;
        s.shadow.offset = {
            LerpFloat(from.shadow.offset.x, to.shadow.offset.x, t),
            LerpFloat(from.shadow.offset.y, to.shadow.offset.y, t)
        };
        s.shadow.radius = LerpFloat(from.shadow.radius, to.shadow.radius, t);
        s.shadow.color = LerpColor(from.shadow.color, to.shadow.color, t);

        ApplyTargetStyle(s);
    }

    void Block::UpdateShapeGeometryFromStyle(const Style& s)
    {
        shape_.SetCornerPointCount(std::max(2u, s.radiusQuality));
        shape_.SetRadius(ToShapeRadius(s.radius));
        shape_.SetSize(config_.size);
        shape_.setOrigin({ config_.size.x * 0.5f, config_.size.y * 0.5f });
        shape_.setPosition(config_.position);
    }

    void Block::UpdateShadowFromStyle(const Style& s)
    {
        if (!s.shadow.enabled)
        {
            shadowShape_.setFillColor(sf::Color(0, 0, 0, 0));
            shadowShape_.setOutlineThickness(0.f);
            return;
        }

        const float expand = std::max(0.f, s.shadow.radius);

        shadowShape_.SetCornerPointCount(std::max(2u, s.radiusQuality));
        shadowShape_.SetRadius(ToShapeRadius(s.radius));
        shadowShape_.SetSize({ config_.size.x + expand * 2.f, config_.size.y + expand * 2.f });
        shadowShape_.setOrigin({ (config_.size.x + expand * 2.f) * 0.5f, (config_.size.y + expand * 2.f) * 0.5f });

        shadowShape_.setPosition(config_.position + s.shadow.offset);
        shadowShape_.setFillColor(s.shadow.color);
        shadowShape_.setOutlineThickness(0.f);
        shadowShape_.setOutlineColor(sf::Color::Transparent);
    }

    void Block::UpdateTransforms()
    {
        float scale = 1.f;

        if (config_.anim.enableHoverScale && hovered_ && config_.enabled)
            scale *= config_.anim.hoverScale;

        if (config_.anim.enablePulseScale)
        {
            constexpr float PI = 3.1415926535f;
            const float t = pulseClock_.getElapsedTime().asSeconds();
            const float s = std::sin(2.f * PI * config_.anim.pulseSpeed * t);
            scale *= (1.f + config_.anim.pulseAmplitude * s);
        }

        float alphaFactor = 1.f;
        if (config_.anim.enableFadeIn)
        {
            const float dur = std::max(0.001f, config_.anim.fadeInSec);
            const float tt = std::min(1.f, fadeClock_.getElapsedTime().asSeconds() / dur);
            alphaFactor *= tt;
        }

        shape_.setScale({ scale, scale });
        shadowShape_.setScale({ scale, scale });

        auto c = shape_.getFillColor();
        c.a = static_cast<uint8_t>(static_cast<float>(c.a) * std::clamp(alphaFactor, 0.f, 1.f));
        shape_.setFillColor(c);

        auto sc = shadowShape_.getFillColor();
        sc.a = static_cast<uint8_t>(static_cast<float>(sc.a) * std::clamp(alphaFactor, 0.f, 1.f));
        shadowShape_.setFillColor(sc);
    }

} // namespace Core::App::Render::UI::Components
