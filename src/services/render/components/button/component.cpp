#include "component.hpp"

namespace Core::App::Render::UI::Components {

    Button::Button(const Config& cfg)
        : config_(cfg)
    {
        rect_.setSize(config_.size);
        rect_.setPosition(config_.position);
        rect_.setOrigin(config_.size.x / 2.f, config_.size.y / 2.f);

        // как ты просил: шрифт грузим "текстом" внутри компонента (потом оптимизируешь)
        font_.loadFromFile(config_.font);
        label_.setFont(font_);
        label_.setString(config_.text);

        drawables_.push_back(&rect_);
        drawables_.push_back(&label_);

        ApplyCurrentStyle();
    }

    void Button::Update() {}

    std::vector<sf::Drawable*> Button::Drawables()
    {
        return drawables_;
    }

    sf::FloatRect Button::Bounds() const
    {
        return rect_.getGlobalBounds();
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
        label_.setString(config_.text);
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
        rect_.setOrigin(config_.size.x / 2.f, config_.size.y / 2.f);
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

        // Hover — из положения мыши (визуал), события клика — только из HandleEvent
        const bool nowHovered = HitTestMouse(window);
        SetHovered(nowHovered);

        ApplyCurrentStyle();
    }

    void Button::HandleEvent(const sf::Event& e, const sf::RenderWindow& window)
    {
        if (!config_.enabled)
            return;

        // hover update from events too (чтобы не ждать Update)
        if (e.type == sf::Event::MouseMoved)
        {
            const bool nowHovered = HitTest(window, e.mouseMove.x, e.mouseMove.y);
            SetHovered(nowHovered);
            ApplyCurrentStyle();
            return;
        }

        // press
        if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == config_.mouseButton)
        {
            const bool inside = HitTest(window, e.mouseButton.x, e.mouseButton.y);

            if (inside)
            {
                pressed_ = true;
                pressedInside_ = true;
                events_.CallEvent(Event::Press);
            }
            else
            {
                // клик вне — сбрасываем
                pressed_ = false;
                pressedInside_ = false;
            }

            ApplyCurrentStyle();
            return;
        }

        // release + click
        if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == config_.mouseButton)
        {
            const bool inside = HitTest(window, e.mouseButton.x, e.mouseButton.y);

            if (pressed_)
            {
                pressed_ = false;
                events_.CallEvent(Event::Release);

                if (pressedInside_ && inside)
                    events_.CallEvent(Event::Click);
            }

            pressedInside_ = false;

            // обновим hover по факту release позиции
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

        // selected overrides hover/normal, но не overrides pressed (кнопку можно жать даже если selected)
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

        // как ты просил: шрифт — текстом, внутри компонента
        font_.loadFromFile(config_.font);
        label_.setFont(font_);

        label_.setCharacterSize(style.text.size);
        label_.setFillColor(style.text.color);
        label_.setStyle(style.text.sfmlStyle);

        ApplyLabelAlignment(style.text);
    }

    void Button::ApplyLabelAlignment(const TextStyle& ts)
    {
        const auto b = label_.getLocalBounds();

        sf::Vector2f origin { 0.f, 0.f };

        switch (ts.hAlign)
        {
            case HAlign::Left:   origin.x = b.left; break;
            case HAlign::Center: origin.x = b.left + b.width / 2.f; break;
            case HAlign::Right:  origin.x = b.left + b.width; break;
        }

        switch (ts.vAlign)
        {
            case VAlign::Top:    origin.y = b.top; break;
            case VAlign::Center: origin.y = b.top + b.height / 2.f; break;
            case VAlign::Bottom: origin.y = b.top + b.height; break;
        }

        label_.setOrigin(origin);

        const auto center = rect_.getPosition();
        label_.setPosition(center + ts.offset);
    }

} // namespace Core::App::Render::UI::Components