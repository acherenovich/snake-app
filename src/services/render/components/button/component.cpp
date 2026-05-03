#include "component.hpp"

namespace Core::App::Render::UI::Components {

    Button::Button(const Config& cfg)
        : config_(cfg)
    {
        rect_.setSize(config_.size);
        rect_.setPosition(config_.position);
        rect_.setOrigin({ config_.size.x / 2.f, config_.size.y / 2.f });

        drawables_.push_back(&rect_);

        EnsureFontLoaded();
        EnsureLabelCreated();

        ApplyCurrentStyle();
    }

    void Button::Update() {}

    std::vector<sf::Drawable*> Button::Drawables()
    {
        // label_ может быть не создан, если шрифт не загрузился
        if (label_)
        {
            // гарантируем что label в drawables
            if (std::find(drawables_.begin(), drawables_.end(), &(*label_)) == drawables_.end())
                drawables_.push_back(&(*label_));
        }

        return drawables_;
    }

    sf::FloatRect Button::Bounds() const
    {
        return rect_.getGlobalBounds();
    }

    bool Button::EnsureFontLoaded()
    {
        // Если шрифт уже загружен — ничего не делаем
        if (font_.getInfo().family != "")
            return true;

        if (!font_.openFromFile(config_.font))
        {
            // Лог можешь сам воткнуть если надо
            return false;
        }

        return true;
    }

    void Button::EnsureLabelCreated()
    {
        if (!EnsureFontLoaded())
            return;

        if (!label_)
        {
            label_.emplace(font_);
            label_->setString(config_.text);
        }
    }

    void Button::SetEnabled(bool value)
    {
        config_.enabled = value;

        hovered_ = false;
        pressed_ = false;
        pressedInside_ = false;

        ApplyCurrentStyle();
    }

    void Button::SetText(const std::string& value)
    {
        config_.text = value;

        EnsureLabelCreated();
        if (label_)
            label_->setString(config_.text);

        ApplyCurrentStyle();
    }

    void Button::SetPosition(const sf::Vector2f& pos)
    {
        config_.position = pos;
        rect_.setPosition(config_.position);
        ApplyCurrentStyle();
    }

    void Button::SetSize(const sf::Vector2f& size)
    {
        config_.size = size;
        rect_.setSize(config_.size);
        rect_.setOrigin({ config_.size.x / 2.f, config_.size.y / 2.f });
        ApplyCurrentStyle();
    }

    void Button::SetStyles(const Style& normal,
                           const Style& hover,
                           const Style& pressed,
                           const Style& disabled)
    {
        config_.normal = normal;
        config_.hover = hover;
        config_.pressed = pressed;
        config_.disabled = disabled;

        ApplyCurrentStyle();
    }

    void Button::SetSelected(bool value)
    {
        selected_ = value;
        ApplyCurrentStyle();
    }

    void Button::SetSelectedStyle(const Style& style)
    {
        selectedStyle_ = style;
        ApplyCurrentStyle();
    }

    void Button::ClearSelectedStyle()
    {
        selectedStyle_.reset();
        ApplyCurrentStyle();
    }

    void Button::Update(const sf::RenderWindow& window)
    {
        if (!config_.enabled)
        {
            ApplyCurrentStyle();
            return;
        }

        const bool nowHovered = HitTestMouse(window);
        SetHovered(nowHovered);

        ApplyCurrentStyle();
    }

    void Button::HandleEvent(const sf::Event& e, const sf::RenderWindow& window)
    {
        if (!config_.enabled)
            return;

        // MouseMoved
        if (const auto* mm = e.getIf<sf::Event::MouseMoved>())
        {
            const bool nowHovered = HitTest(window, mm->position.x, mm->position.y);
            SetHovered(nowHovered);
            ApplyCurrentStyle();
            return;
        }

        // Press
        if (const auto* mb = e.getIf<sf::Event::MouseButtonPressed>())
        {
            if (mb->button != config_.mouseButton)
                return;

            const bool inside = HitTest(window, mb->position.x, mb->position.y);

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

            ApplyCurrentStyle();
            return;
        }

        // Release + Click
        if (const auto* mr = e.getIf<sf::Event::MouseButtonReleased>())
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

            SetHovered(inside);
            ApplyCurrentStyle();
            return;
        }
    }

    void Button::SetHovered(bool value)
    {
        if (value == hovered_)
            return;

        hovered_ = value;

        if (hovered_)
            events_.CallEvent(Event::HoverEnter);
        else
            events_.CallEvent(Event::HoverLeave);
    }

    bool Button::HitTest(const sf::RenderWindow& window, int px, int py) const
    {
        const sf::Vector2f p = window.mapPixelToCoords({ px, py });
        return rect_.getGlobalBounds().contains(p);
    }

    bool Button::HitTestMouse(const sf::RenderWindow& window) const
    {
        const auto pixel = sf::Mouse::getPosition(window);
        return HitTest(window, pixel.x, pixel.y);
    }

    void Button::ApplyCurrentStyle()
    {
        if (!config_.enabled)
        {
            ApplyStyle(config_.disabled);
            return;
        }

        if (pressed_)
        {
            ApplyStyle(config_.pressed);
            return;
        }

        if (selected_ && selectedStyle_)
        {
            ApplyStyle(*selectedStyle_);
            return;
        }

        if (hovered_)
        {
            ApplyStyle(config_.hover);
            return;
        }

        ApplyStyle(config_.normal);
    }

    void Button::ApplyStyle(const Style& style)
    {
        rect_.setFillColor(style.background);
        rect_.setOutlineThickness(style.borderThickness);
        rect_.setOutlineColor(style.borderColor);

        EnsureLabelCreated();
        if (!label_)
            return;

        label_->setCharacterSize(style.text.size);
        label_->setFillColor(style.text.color);
        label_->setStyle(style.text.sfmlStyle);

        ApplyLabelAlignment(style.text);
    }

    void Button::ApplyLabelAlignment(const TextStyle& ts)
    {
        if (!label_)
            return;

        const auto b = label_->getLocalBounds();

        sf::Vector2f origin { 0.f, 0.f };

        switch (ts.hAlign)
        {
            case HAlign::Left:   origin.x = b.position.x; break;
            case HAlign::Center: origin.x = b.position.x + b.size.x / 2.f; break;
            case HAlign::Right:  origin.x = b.position.x + b.size.x; break;
        }

        switch (ts.vAlign)
        {
            case VAlign::Top:    origin.y = b.position.y; break;
            case VAlign::Center: origin.y = b.position.y + b.size.y / 2.f; break;
            case VAlign::Bottom: origin.y = b.position.y + b.size.y; break;
        }

        label_->setOrigin(origin);

        const auto center = rect_.getPosition();
        label_->setPosition(center + ts.offset);
    }

} // namespace Core::App::Render::UI::Components
