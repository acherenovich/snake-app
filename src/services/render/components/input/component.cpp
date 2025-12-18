#include "component.hpp"

#include <cmath>

#include <SFML/System/Utf.hpp>

namespace Core::App::Render::UI::Components {

    static sf::Color WithAlpha(sf::Color c, sf::Uint8 a)
    {
        c.a = a;
        return c;
    }

    static sf::String Utf8ToSfString(const std::string& s)
    {
        return sf::String::fromUtf8(s.begin(), s.end());
    }

    static std::string SfStringToUtf8(const sf::String& s)
    {
        std::string out;
        out.reserve(s.getSize() * 4); // UTF-8 до 4 байт на символ

        const auto u32 = s.toUtf32(); // std::basic_string<sf::Uint32>
        sf::Utf<32>::toUtf8(u32.begin(), u32.end(), std::back_inserter(out));

        return out;
    }

    Input::Input(Config cfg)
        : config_(std::move(cfg))
        , enabled_(config_.enabled)
    {
        // font
        if (!config_.font.empty())
        {
            const bool ok = font_.loadFromFile(config_.font);
            if (!ok && config_.requireFont)
            {
                // оставим компонент живым, но текст может не отрисоваться
            }
        }

        // utf32 init
        value32_ = Utf8ToSfString(config_.value);
        placeholder32_ = Utf8ToSfString(config_.placeholder);
        mask32_ = Utf8ToSfString(config_.passwordMask);

        // box
        box_.setPosition(config_.position);
        box_.setSize(config_.size);

        // selection
        selectionRect_.setFillColor(config_.selectionStyle.color);

        // caret
        caretRect_.setFillColor(config_.caretStyle.color);
        caretRect_.setSize({ config_.caretStyle.width, config_.size.y - config_.caretStyle.paddingTopBottom * 2.f });

        // texts
        text_.setFont(font_);
        text_.setCharacterSize(config_.textStyle.size);
        text_.setFillColor(config_.textStyle.color);
        text_.setStyle(config_.textStyle.sfmlStyle);

        placeholderText_.setFont(font_);
        placeholderText_.setCharacterSize(config_.placeholderStyle.size);
        placeholderText_.setFillColor(config_.placeholderStyle.color);
        placeholderText_.setStyle(config_.placeholderStyle.sfmlStyle);

        caretClock_.restart();
        pulseClock_.restart();

        // draw order
        drawables_.push_back(&box_);
        drawables_.push_back(&selectionRect_);
        drawables_.push_back(&placeholderText_);
        drawables_.push_back(&text_);
        drawables_.push_back(&caretRect_);

        // clamp maxLength (characters)
        if (value32_.getSize() > config_.maxLength)
            value32_ = value32_.substring(0, config_.maxLength);

        caret_ = value32_.getSize();

        ApplyVisualState(enabled_ ? VisualState::Normal : VisualState::Disabled);

        RebuildTextObjects();
        UpdateLayout();
        UpdateSelectionVisual();
        UpdateCaretVisual();
        EnsureCaretVisible();
    }

    void Input::Update() {}

    std::vector<sf::Drawable*> Input::Drawables()
    {
        return drawables_;
    }

    std::string Input::Value() const
    {
        return SfStringToUtf8(value32_);
    }

    void Input::SetValue(std::string v)
    {
        value32_ = Utf8ToSfString(v);
        if (value32_.getSize() > config_.maxLength)
            value32_ = value32_.substring(0, config_.maxLength);

        caret_ = std::min<std::size_t>(caret_, value32_.getSize());
        ClearSelection();
        RebuildTextObjects();
        caretClock_.restart();
    }

    void Input::SetFocused(bool focused)
    {
        if (focused_ == focused)
            return;

        focused_ = focused;
        caretClock_.restart();

        if (focused_)
            events_.CallEvent(Event::Focus);
        else
            events_.CallEvent(Event::Blur);

        if (!focused_)
            ClearSelection();
    }

    void Input::SetEnabled(bool enabled)
    {
        enabled_ = enabled;
        if (!enabled_)
            SetFocused(false);
    }

    void Input::Update(const sf::RenderWindow& window)
    {
        hovered_ = enabled_ && ContainsPoint(window, sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);

        if (!enabled_)
            ApplyVisualState(VisualState::Disabled);
        else if (focused_)
            ApplyVisualState(VisualState::Focus);
        else if (hovered_)
            ApplyVisualState(VisualState::Hover);
        else
            ApplyVisualState(VisualState::Normal);

        // caret blink
        if (focused_ && enabled_)
        {
            const float t = caretClock_.getElapsedTime().asSeconds();
            const float period = std::max(0.1f, config_.caretStyle.blinkPeriodSec);
            caretVisible_ = std::fmod(t, period) < (period * 0.5f);
        }
        else
        {
            caretVisible_ = false;
        }

        // placeholder pulse (only if shown)
        if (config_.placeholderStyle.pulseAlpha && value32_.isEmpty() && !focused_)
        {
            const float t = pulseClock_.getElapsedTime().asSeconds();
            const float s = (std::sin(t * config_.placeholderStyle.pulseSpeed) + 1.f) * 0.5f;
            const auto a = static_cast<sf::Uint8>(config_.placeholderStyle.pulseMin + s * (config_.placeholderStyle.pulseMax - config_.placeholderStyle.pulseMin));
            placeholderText_.setFillColor(WithAlpha(config_.placeholderStyle.color, a));
        }
        else
        {
            placeholderText_.setFillColor(config_.placeholderStyle.color);
        }

        UpdateLayout();
        UpdateSelectionVisual();
        UpdateCaretVisual();
        EnsureCaretVisible();
    }

    void Input::HandleEvent(const sf::Event& e, const sf::RenderWindow& window)
    {
        if (!enabled_)
            return;

        if (e.type == sf::Event::MouseMoved)
        {
            hovered_ = ContainsPoint(window, e.mouseMove.x, e.mouseMove.y);
            return;
        }

        if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left)
        {
            const bool inside = ContainsPoint(window, e.mouseButton.x, e.mouseButton.y);
            SetFocused(inside);

            if (inside)
            {
                const sf::Vector2f click = window.mapPixelToCoords({ e.mouseButton.x, e.mouseButton.y });

                std::size_t best = 0;
                float bestDist = std::numeric_limits<float>::max();

                const auto shown = VisibleString();
                text_.setString(shown);

                const std::size_t n = shown.getSize();
                for (std::size_t i = 0; i <= n; ++i)
                {
                    const float x = TextXForIndex(i);
                    const float dist = std::fabs((config_.position.x + config_.paddingX + x - scrollX_) - click.x);
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        best = i;
                    }
                }

                caret_ = best;
                ClearSelection();
                caretClock_.restart();
            }

            return;
        }

        if (!focused_)
            return;

        if (e.type == sf::Event::TextEntered)
        {
            const char32_t u = static_cast<char32_t>(e.text.unicode);

            // ignore control keys here
            if (u == 13 || u == 10) return; // Enter
            if (u == 8) return;             // Backspace
            if (u == 9) return;             // Tab

            if (IsAllowedChar(u))
            {
                InsertUtf32(u);
                events_.CallEvent(Event::Changed);
            }

            caretClock_.restart();
            return;
        }

        if (e.type == sf::Event::KeyPressed)
        {
            const bool ctrl = e.key.control;

            switch (e.key.code)
            {
                case sf::Keyboard::Enter:
                {
                    if (config_.submitOnEnter)
                    {
                        events_.CallEvent(Event::Submit);
                        if (config_.clearOnSubmit)
                        {
                            value32_.clear();
                            caret_ = 0;
                            ClearSelection();
                            RebuildTextObjects();
                            events_.CallEvent(Event::Changed);
                        }
                    }
                    break;
                }
                case sf::Keyboard::BackSpace:
                {
                    if (HasSelection()) DeleteSelection();
                    else Backspace();
                    events_.CallEvent(Event::Changed);
                    break;
                }
                case sf::Keyboard::Delete:
                {
                    if (HasSelection()) DeleteSelection();
                    else Delete();
                    events_.CallEvent(Event::Changed);
                    break;
                }
                case sf::Keyboard::Left:  MoveCaretLeft(ctrl); break;
                case sf::Keyboard::Right: MoveCaretRight(ctrl); break;
                case sf::Keyboard::Home:  MoveCaretHome(); break;
                case sf::Keyboard::End:   MoveCaretEnd(); break;
                case sf::Keyboard::A:
                    if (ctrl) SelectAll();
                    break;
                default:
                    break;
            }

            caretClock_.restart();
            return;
        }
    }

    void Input::RebuildTextObjects()
    {
        text_.setString(VisibleString());
        placeholderText_.setString(placeholder32_);
        UpdatePlaceholderVisibility();
    }

    void Input::UpdatePlaceholderVisibility()
    {
        // placeholder visible only when empty
        if (!placeholder32_.isEmpty() && value32_.isEmpty())
            placeholderText_.setString(placeholder32_);
        else
            placeholderText_.setString(sf::String());
    }

    void Input::UpdateLayout()
    {
        box_.setPosition(config_.position);
        box_.setSize(config_.size);

        const auto centerY = config_.position.y + config_.size.y * 0.5f;

        // set sizes/styles each tick (ok)
        text_.setFont(font_);
        placeholderText_.setFont(font_);

        text_.setCharacterSize(config_.textStyle.size);
        text_.setStyle(config_.textStyle.sfmlStyle);

        placeholderText_.setCharacterSize(config_.placeholderStyle.size);
        placeholderText_.setStyle(config_.placeholderStyle.sfmlStyle);

        // base position (left padding + scroll)
        const sf::Vector2f basePos {
            config_.position.x + config_.paddingX - scrollX_,
            config_.position.y
        };

        if (config_.verticalCenterText)
        {
            const auto tb = text_.getLocalBounds();
            const float y = centerY - (tb.height * 0.5f) - tb.top;
            text_.setPosition(basePos.x, y);

            const auto pb = placeholderText_.getLocalBounds();
            const float py = centerY - (pb.height * 0.5f) - pb.top;
            placeholderText_.setPosition(config_.position.x + config_.paddingX - scrollX_, py);
        }
        else
        {
            text_.setPosition(basePos.x, config_.position.y + 6.f);
            placeholderText_.setPosition(config_.position.x + config_.paddingX - scrollX_, config_.position.y + 6.f);
        }
    }

    void Input::ApplyVisualState(VisualState st)
    {
        const BoxStyle* b = &config_.normalBox;
        if (st == VisualState::Disabled) b = &config_.disabledBox;
        else if (st == VisualState::Focus) b = &config_.focusBox;
        else if (st == VisualState::Hover) b = &config_.hoverBox;

        box_.setFillColor(b->background);
        box_.setOutlineThickness(b->borderThickness);
        box_.setOutlineColor(b->borderColor);

        text_.setFillColor(enabled_ ? config_.textStyle.color : WithAlpha(config_.textStyle.color, 120));
    }

    bool Input::ContainsPoint(const sf::RenderWindow& window, int px, int py) const
    {
        const sf::Vector2f p = window.mapPixelToCoords({ px, py });
        const sf::FloatRect r(config_.position, config_.size);
        return r.contains(p);
    }

    bool Input::IsAllowedChar(char32_t u) const
    {
        if (!config_.allowNewLine && (u == U'\n' || u == U'\r'))
            return false;

        if (!config_.allowSpaces && (u == U' '))
            return false;

        if (u < 32)
            return false;

        if (u <= 0x7F)
        {
            const unsigned char c = static_cast<unsigned char>(u);

            if (std::isalpha(c)) return config_.allowLetters;
            if (std::isdigit(c)) return config_.allowDigits;

            if (c == '_') return config_.allowUnderscore;
            if (c == '-') return config_.allowDash;
            if (c == '.') return config_.allowDot;
            if (c == '@') return config_.allowAt;

            return config_.allowOther;
        }

        return config_.allowOther;
    }

    void Input::InsertUtf32(char32_t u)
    {
        if (value32_.getSize() >= config_.maxLength)
            return;

        if (HasSelection())
            DeleteSelection();

        // insert at caret (character index)
        sf::String ins;
        ins += static_cast<sf::Uint32>(u);

        const std::size_t leftLen = std::min(caret_, static_cast<std::size_t>(value32_.getSize()));
        const sf::String left = value32_.substring(0, leftLen);
        const sf::String right = value32_.substring(leftLen);

        value32_ = left + ins + right;

        caret_ = std::min(leftLen + 1, static_cast<std::size_t>(value32_.getSize()));

        if (value32_.getSize() > config_.maxLength)
            value32_ = value32_.substring(0, config_.maxLength);

        RebuildTextObjects();
    }

    void Input::Backspace()
    {
        if (caret_ == 0 || value32_.isEmpty())
            return;

        const std::size_t idx = caret_ - 1;

        const sf::String left = value32_.substring(0, idx);
        const sf::String right = value32_.substring(idx + 1);

        value32_ = left + right;
        caret_ = idx;

        RebuildTextObjects();
    }

    void Input::Delete()
    {
        if (caret_ >= value32_.getSize() || value32_.isEmpty())
            return;

        const std::size_t idx = caret_;

        const sf::String left = value32_.substring(0, idx);
        const sf::String right = value32_.substring(idx + 1);

        value32_ = left + right;

        RebuildTextObjects();
    }

    void Input::MoveCaretLeft(bool ctrl)
    {
        if (caret_ == 0)
            return;

        caret_ = ctrl ? PrevWordBoundary(caret_) : (caret_ - 1);
        ClearSelection();
    }

    void Input::MoveCaretRight(bool ctrl)
    {
        const std::size_t n = value32_.getSize();
        if (caret_ >= n)
            return;

        caret_ = ctrl ? NextWordBoundary(caret_) : (caret_ + 1);
        ClearSelection();
    }

    void Input::MoveCaretHome()
    {
        caret_ = 0;
        ClearSelection();
    }

    void Input::MoveCaretEnd()
    {
        caret_ = value32_.getSize();
        ClearSelection();
    }

    void Input::SelectAll()
    {
        selectionAnchor_ = 0;
        caret_ = value32_.getSize();
    }

    void Input::ClearSelection()
    {
        selectionAnchor_.reset();
    }

    bool Input::HasSelection() const
    {
        return selectionAnchor_.has_value() && *selectionAnchor_ != caret_;
    }

    void Input::DeleteSelection()
    {
        if (!HasSelection())
            return;

        const auto a = *selectionAnchor_;
        const auto b = caret_;
        const auto l = std::min(a, b);
        const auto r = std::max(a, b);

        const sf::String left = value32_.substring(0, l);
        const sf::String right = value32_.substring(r);

        value32_ = left + right;
        caret_ = l;

        ClearSelection();
        RebuildTextObjects();
    }

    std::size_t Input::PrevWordBoundary(std::size_t pos) const
    {
        if (pos == 0) return 0;

        auto s = SfStringToUtf8(value32_);
        if (pos > s.size()) pos = s.size(); // safe for ascii; for unicode we do better below

        // Proper: work on UTF-32 directly
        std::size_t i = pos;
        while (i > 0)
        {
            const auto ch = static_cast<char32_t>(value32_[static_cast<unsigned int>(i - 1)]);
            if (!std::iswspace(static_cast<wint_t>(ch)))
                break;
            --i;
        }
        while (i > 0)
        {
            const auto ch = static_cast<char32_t>(value32_[static_cast<unsigned int>(i - 1)]);
            if (std::iswspace(static_cast<wint_t>(ch)))
                break;
            --i;
        }
        return i;
    }

    std::size_t Input::NextWordBoundary(std::size_t pos) const
    {
        const std::size_t n = value32_.getSize();
        std::size_t i = std::min(pos, n);

        while (i < n)
        {
            const auto ch = static_cast<char32_t>(value32_[static_cast<unsigned int>(i)]);
            if (std::iswspace(static_cast<wint_t>(ch)))
                break;
            ++i;
        }
        while (i < n)
        {
            const auto ch = static_cast<char32_t>(value32_[static_cast<unsigned int>(i)]);
            if (!std::iswspace(static_cast<wint_t>(ch)))
                break;
            ++i;
        }
        return i;
    }

    sf::String Input::MaskString(std::size_t count) const
    {
        sf::String out;

        // Если маска пустая — используем '*'
        if (mask32_.isEmpty())
        {
            for (std::size_t i = 0; i < count; ++i)
                out += static_cast<sf::Uint32>(U'*');

            return out;
        }

        // mask32_ может быть multi-char (например "●" или "••")
        for (std::size_t i = 0; i < count; ++i)
            out += mask32_;

        return out;
    }

    sf::String Input::VisibleString() const
    {
        if (!config_.password)
            return value32_;

        return MaskString(value32_.getSize());
    }

    float Input::TextXForIndex(std::size_t index) const
    {
        const auto& s = text_.getString();
        const std::size_t n = s.getSize();

        const std::size_t i = std::min(index, n);
        const auto p = text_.findCharacterPos(static_cast<unsigned int>(i));
        return p.x - text_.getPosition().x;
    }

    void Input::EnsureCaretVisible()
    {
        // ensure based on current shown string
        text_.setString(VisibleString());

        const float caretX = TextXForIndex(caret_);
        const float leftLimit  = 0.f;
        const float rightLimit = config_.size.x - config_.paddingX * 2.f;

        if (caretX < leftLimit)
            scrollX_ = std::max(0.f, scrollX_ - (leftLimit - caretX) - 10.f);

        if (caretX > rightLimit)
            scrollX_ += (caretX - rightLimit) + 10.f;
    }

    void Input::UpdateCaretVisual()
    {
        if (!focused_ || !enabled_ || !caretVisible_)
        {
            caretRect_.setSize({ 0.f, 0.f });
            return;
        }

        text_.setString(VisibleString());

        const float caretX = TextXForIndex(caret_);
        const float x = config_.position.x + config_.paddingX + caretX - scrollX_;

        const float top = config_.position.y + config_.caretStyle.paddingTopBottom;
        const float h = config_.size.y - config_.caretStyle.paddingTopBottom * 2.f;

        caretRect_.setSize({ config_.caretStyle.width, h });
        caretRect_.setPosition({ x, top });
        caretRect_.setFillColor(config_.caretStyle.color);
    }

    void Input::UpdateSelectionVisual()
    {
        if (!HasSelection() || !focused_ || !enabled_)
        {
            selectionRect_.setSize({ 0.f, 0.f });
            return;
        }

        text_.setString(VisibleString());

        const auto a = *selectionAnchor_;
        const auto b = caret_;
        const auto l = std::min(a, b);
        const auto r = std::max(a, b);

        const float x1 = config_.position.x + config_.paddingX + TextXForIndex(l) - scrollX_;
        const float x2 = config_.position.x + config_.paddingX + TextXForIndex(r) - scrollX_;

        const float y = config_.position.y + config_.caretStyle.paddingTopBottom;
        const float h = config_.size.y - config_.caretStyle.paddingTopBottom * 2.f;

        selectionRect_.setPosition({ x1, y });
        selectionRect_.setSize({ std::max(0.f, x2 - x1), h });
        selectionRect_.setFillColor(config_.selectionStyle.color);
    }

} // namespace Core::App::Render::UI::Components