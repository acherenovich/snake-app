#include "authorization.hpp"

namespace Core::App::Render::Pages {

    [[maybe_unused]] __used Utils::Service::Loader::Add<Authorization> AuthorizationPage(PagesLoader());

    // ==============================
    // Styles
    // ==============================

    static UI::Components::Button::Style TabInactiveNormal()
    {
        UI::Components::Button::Style s{};
        s.background = sf::Color(35, 35, 45, 220);
        s.borderThickness = 2.f;
        s.borderColor = sf::Color(255, 255, 255, 25);
        s.text = { .color = sf::Color(230, 230, 230), .size = 22, .sfmlStyle = sf::Text::Bold };
        return s;
    }

    static UI::Components::Button::Style TabInactiveHover()
    {
        auto s = TabInactiveNormal();
        s.background = sf::Color(45, 45, 60, 235);
        s.borderColor = sf::Color(255, 255, 255, 45);
        return s;
    }

    static UI::Components::Button::Style TabInactivePressed()
    {
        auto s = TabInactiveNormal();
        s.background = sf::Color(25, 25, 35, 235);
        s.borderColor = sf::Color(255, 255, 255, 10);
        return s;
    }

    static UI::Components::Button::Style TabDisabled()
    {
        UI::Components::Button::Style s{};
        s.background = sf::Color(80, 80, 80, 120);
        s.borderThickness = 2.f;
        s.borderColor = sf::Color(255, 255, 255, 15);
        s.text = { .color = sf::Color(220, 220, 220, 120), .size = 22, .sfmlStyle = sf::Text::Bold };
        return s;
    }

    static UI::Components::Button::Style TabSelectedStyle()
    {
        UI::Components::Button::Style s{};
        s.background = sf::Color(70, 80, 230);
        s.borderThickness = 2.f;
        s.borderColor = sf::Color(255, 255, 255, 70);
        s.text = { .color = sf::Color::White, .size = 22, .sfmlStyle = sf::Text::Bold };
        return s;
    }

    static UI::Components::Button::Style SubmitNormal()
    {
        UI::Components::Button::Style s{};
        s.background = sf::Color(60, 70, 220);
        s.borderThickness = 2.f;
        s.borderColor = sf::Color(255, 255, 255, 30);
        s.text = { .color = sf::Color::White, .size = 26, .sfmlStyle = sf::Text::Bold };
        return s;
    }

    static UI::Components::Button::Style SubmitHover()
    {
        auto s = SubmitNormal();
        s.background = sf::Color(80, 90, 255);
        s.borderColor = sf::Color(255, 255, 255, 60);
        return s;
    }

    static UI::Components::Button::Style SubmitPressed()
    {
        auto s = SubmitNormal();
        s.background = sf::Color(40, 50, 180);
        s.borderColor = sf::Color(255, 255, 255, 0);
        return s;
    }

    static UI::Components::Button::Style SubmitDisabled()
    {
        UI::Components::Button::Style s{};
        s.background = sf::Color(110, 110, 110);
        s.borderThickness = 2.f;
        s.borderColor = sf::Color(255, 255, 255, 20);
        s.text = { .color = sf::Color(230, 230, 230), .size = 26, .sfmlStyle = sf::Text::Bold };
        return s;
    }

    static UI::Components::Input::Config InputStyle(const sf::Vector2f pos,
                                                    const std::string& placeholder,
                                                    const bool password = false)
    {
        UI::Components::Input::Config cfg{};
        cfg.position = pos;
        cfg.size = { 520.f, 58.f };
        cfg.paddingX = 14.f;

        cfg.font = "assets/fonts/Roboto-Regular.ttf";
        cfg.placeholder = placeholder;
        cfg.password = password;

        cfg.normalBox = { .background = sf::Color(25,25,30,230), .borderColor = sf::Color(255,255,255,25), .borderThickness = 2.f };
        cfg.hoverBox  = { .background = sf::Color(30,30,40,240), .borderColor = sf::Color(255,255,255,45), .borderThickness = 2.f };
        cfg.focusBox  = { .background = sf::Color(30,30,45,245), .borderColor = sf::Color(120,160,255,200), .borderThickness = 2.f };
        cfg.disabledBox = { .background = sf::Color(30,30,30,140), .borderColor = sf::Color(255,255,255,15), .borderThickness = 2.f };

        cfg.textStyle = { .color = sf::Color::White, .size = 22, .sfmlStyle = sf::Text::Regular };
        cfg.placeholderStyle = { .color = sf::Color(255,255,255,120), .size = 22, .sfmlStyle = sf::Text::Regular, .pulseAlpha = false };

        cfg.maxLength = 64;
        cfg.allowNewLine = false;

        return cfg;
    }

    // ==============================
    // Authorization
    // ==============================

    void Authorization::Initialise()
    {
        Log()->Debug("Initializing Authorization page");

        ui.title = UI::Components::Text::Create({
            .text = "Snake",
            .font = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 54,
            .position = { center_.x, center_.y - 260.f },
            .hAlign = UI::Components::Text::HAlign::Center,
            .vAlign = UI::Components::Text::VAlign::Center,
            .color = sf::Color::White,
            .enableFadeIn = true,
            .fadeInDuration = 0.35f
        });

        ui.hint = UI::Components::Text::Create({
            .text = "",
            .font = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 18,
            .position = { center_.x, center_.y + 210.f },
            .hAlign = UI::Components::Text::HAlign::Center,
            .vAlign = UI::Components::Text::VAlign::Center,
            .color = sf::Color(255, 120, 120)
        });

        // Tabs: создаём один раз
        ui.tabLogin = UI::Components::Button::Create({
            .position = { center_.x - 130.f, center_.y - 190.f },
            .size = { 240.f, 52.f },
            .text = "Login",
            .font = "assets/fonts/Roboto-Regular.ttf"
        });

        ui.tabRegister = UI::Components::Button::Create({
            .position = { center_.x + 130.f, center_.y - 190.f },
            .size = { 240.f, 52.f },
            .text = "Register",
            .font = "assets/fonts/Roboto-Regular.ttf"
        });

        // Общие стили табов (inactive как базовые), selected style будет сверху
        ui.tabLogin->SetStyles(
            TabInactiveNormal(),
            TabInactiveHover(),
            TabInactivePressed(),
            TabDisabled()
        );
        ui.tabRegister->SetStyles(
            TabInactiveNormal(),
            TabInactiveHover(),
            TabInactivePressed(),
            TabDisabled()
        );

        // Selected style (активная вкладка)
        ui.tabLogin->SetSelectedStyle(TabSelectedStyle());
        ui.tabRegister->SetSelectedStyle(TabSelectedStyle());

        ui.tabLogin->Events().HookEvent(UI::Components::Button::Event::Click) =
            std::function([this] { SetMode(Mode::Login); });

        ui.tabRegister->Events().HookEvent(UI::Components::Button::Event::Click) =
            std::function([this] { SetMode(Mode::Register); });

        // Inputs
        ui.login = UI::Components::Input::Create(InputStyle({ center_.x - 260.f, center_.y - 100.f }, "Login"));
        ui.password = UI::Components::Input::Create(InputStyle({ center_.x - 260.f, center_.y - 30.f }, "Password", true));
        ui.password2 = UI::Components::Input::Create(InputStyle({ center_.x - 260.f, center_.y + 40.f }, "Confirm password", true));

        // Submit
        ui.submit = UI::Components::Button::Create({
            .position = { center_.x, center_.y + 140.f },
            .size = { 320.f, 64.f },
            .text = "Login",
            .font = "assets/fonts/Roboto-Regular.ttf"
        });

        ui.submit->SetStyles(
            SubmitNormal(),
            SubmitHover(),
            SubmitPressed(),
            SubmitDisabled()
        );

        ui.submit->Events().HookEvent(UI::Components::Button::Event::Click) =
            std::function([this] {
                Validate();
                if (!ui.submit->IsEnabled())
                    return;

                Log()->Debug("[Authorization] submit");

                Submit();
            });

        // Validation triggers
        ui.login->Events().HookEvent(UI::Components::Input::Event::Changed) =
            std::function([this] { Validate(); });
        ui.password->Events().HookEvent(UI::Components::Input::Event::Changed) =
            std::function([this] { Validate(); });
        ui.password2->Events().HookEvent(UI::Components::Input::Event::Changed) =
            std::function([this] { Validate(); });


        ui.overlay = UI::Components::Overlay::Create({
            .enabled = false,
            .position = { 0.f, 0.f },
            .size = { Width, Height },
            .color = sf::Color(0, 0, 0, 160),
            .blockInput = true,
            .closeOnClick = false,
            .enableFadeIn = true,
            .fadeInDuration = 0.15f,
        });

        ui.loader = UI::Components::Loader::Create({
            .center = { center_.x, center_.y + 10.f },
            .radius = 52.f,
            .dotRadius = 7.f,
            .baseAngleDeg = 0.f,
            .dotsCount = 8
        });

        ui.loadingText = UI::Components::Text::Create({
            .text = "Loading...",
            .font = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 22,
            .position = { center_.x, center_.y + 90.f },
            .hAlign = UI::Components::Text::HAlign::Center,
            .vAlign = UI::Components::Text::VAlign::Center,
            .color = sf::Color(255,255,255,220),
            .enableFadeIn = true,
            .fadeInDuration = 0.15f,
            .enablePulseAlpha = true,
            .pulseAlphaMin = 0.55f,
            .pulseAlphaSpeed = 0.7f
        });

        SetMode(Mode::Login);
    }

    void Authorization::OnAllInterfacesLoaded()
    {
        client_ = IFace().Get<Client>();
        gameController_ = IFace().Get<GameController>();

        // client_->RegisterConnectionStateCallback(
        //     [this](const ConnectionState& /*old*/, const ConnectionState& current) {
        //         connectionState_ = current;
        //         Validate();
        //     });
    }

    void Authorization::Submit()
    {
        if (submitting_)
            return;

        submitting_ = true;

        ui.overlay->SetEnabled(true);
        ui.loadingText->SetText(mode_ == Mode::Login ? "Logging in..." : "Registering...");
        ui.loadingText->ResetTimer();

        ui.submit->SetEnabled(false);
        ui.hint->SetText("");

        if (mode_ == Mode::Login)
        {
            gameController_->PerformLogin(ui.login->Value(), ui.password->Value(), true) =
                [this](const Game::ActionResult<>& result)
                {
                    submitting_ = false;
                    ui.overlay->SetEnabled(false);

                    Validate();

                    if (!result.success)
                        ui.hint->SetText(result.error);
                };

            return;
        }

        // Register (если у тебя есть метод)
        // gameController_->PerformRegister(...) = [this](...) { ... };

        // если пока нет регистрации — снимаем submitting
        submitting_ = false;
        ui.overlay->SetEnabled(false);
        RefreshModeUI();
        Validate();
    }

    void Authorization::SetMode(const Mode m)
    {
        mode_ = m;

        ui.tabLogin->SetSelected(mode_ == Mode::Login);
        ui.tabRegister->SetSelected(mode_ == Mode::Register);

        RefreshModeUI();
        Validate();
    }

    void Authorization::RefreshModeUI()
    {
        if (mode_ == Mode::Login)
        {
            ui.submit->SetText("Login");

            ui.password2->SetEnabled(false);
            ui.password2->SetFocused(false); // предполагаю, что у Input есть такой метод
        }
        else
        {
            ui.submit->SetText("Register");
            ui.password2->SetEnabled(true);
        }
    }

    void Authorization::Validate()
    {
        const auto& login = ui.login->Value();
        const auto& pass  = ui.password->Value();
        const auto& pass2 = ui.password2->Value();

        bool ok = true;
        std::string hint;

        if (login.size() < 3)
        {
            ok = false;
            hint = "Login is too short";
        }
        else if (pass.size() < 6)
        {
            ok = false;
            hint = "Password is too short";
        }
        else if (mode_ == Mode::Register)
        {
            if (pass2.size() < 6)
            {
                ok = false;
                hint = "Confirm password";
            }
            else if (pass != pass2)
            {
                ok = false;
                hint = "Passwords do not match";
            }
        }

        ui.submit->SetEnabled(ok);
        ui.hint->SetText(hint);
    }

    void Authorization::HandleEvent(sf::Event& event, sf::RenderWindow& window)
    {
        ui.overlay->HandleEvent(event, window);

        if (submitting_ && ui.overlay->IsEnabled())
            return;

        ui.login->HandleEvent(event, window);
        ui.password->HandleEvent(event, window);

        if (mode_ == Mode::Register)
            ui.password2->HandleEvent(event, window);

        ui.tabLogin->HandleEvent(event, window);
        ui.tabRegister->HandleEvent(event, window);
        ui.submit->HandleEvent(event, window);
    }

    void Authorization::UpdateScene()
    {
        auto& window = GetController()->Window();
        if (!window.isOpen())
            return;

        ui.title->Update();
        ui.hint->Update();

        ui.tabLogin->Update(window);
        ui.tabRegister->Update(window);

        ui.login->Update(window);
        ui.password->Update(window);
        if (mode_ == Mode::Register)
            ui.password2->Update(window);

        ui.submit->Update(window);

        for (auto* d : ui.title->Drawables()) window.draw(*d);
        for (auto* d : ui.tabLogin->Drawables()) window.draw(*d);
        for (auto* d : ui.tabRegister->Drawables()) window.draw(*d);

        for (auto* d : ui.login->Drawables()) window.draw(*d);
        for (auto* d : ui.password->Drawables()) window.draw(*d);
        if (mode_ == Mode::Register)
            for (auto* d : ui.password2->Drawables()) window.draw(*d);

        for (auto* d : ui.submit->Drawables()) window.draw(*d);
        for (auto* d : ui.hint->Drawables()) window.draw(*d);

        if (ui.overlay->IsEnabled())
        {
            ui.overlay->Update(window);
            ui.loader->Update();
            ui.loadingText->Update();

            for (auto* d : ui.overlay->Drawables()) window.draw(*d);
            for (auto* d : ui.loader->Drawables()) window.draw(*d);
            for (auto* d : ui.loadingText->Drawables()) window.draw(*d);
        }
    }

} // namespace Core::App::Render::Pages