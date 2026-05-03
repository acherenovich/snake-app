#include "main_menu.hpp"

#include <chrono>
using namespace std::chrono_literals;

namespace Core::App::Render::Pages {

    [[maybe_unused]] [[gnu::used]] Utils::Service::Loader::Add<MainMenu> MainMenuPage(PagesLoader());

    // ============================================================
    // Style helpers
    // ============================================================

    static UI::Components::Button::Style ButtonNormal()
    {
        UI::Components::Button::Style s{};
        s.background = sf::Color(45, 52, 160);
        s.borderThickness = 2.f;
        s.borderColor = sf::Color(255, 255, 255, 30);
        s.text = { .color = sf::Color::White, .size = 22, .sfmlStyle = sf::Text::Bold };
        return s;
    }

    static UI::Components::Button::Style ButtonHover()
    {
        auto s = ButtonNormal();
        s.background = sf::Color(60, 70, 220);
        s.borderColor = sf::Color(255, 255, 255, 60);
        return s;
    }

    static UI::Components::Button::Style ButtonPressed()
    {
        auto s = ButtonNormal();
        s.background = sf::Color(35, 40, 120);
        s.borderColor = sf::Color(255, 255, 255, 10);
        return s;
    }

    static UI::Components::Button::Style ButtonDisabled()
    {
        UI::Components::Button::Style s{};
        s.background = sf::Color(110, 110, 110);
        s.borderThickness = 2.f;
        s.borderColor = sf::Color(255, 255, 255, 20);
        s.text = { .color = sf::Color(230, 230, 230), .size = 22, .sfmlStyle = sf::Text::Bold };
        return s;
    }

    static UI::Components::Button::Style DangerNormal()
    {
        UI::Components::Button::Style s{};
        s.background = sf::Color(170, 50, 60);
        s.borderThickness = 2.f;
        s.borderColor = sf::Color(255, 255, 255, 30);
        s.text = { .color = sf::Color::White, .size = 20, .sfmlStyle = sf::Text::Bold };
        return s;
    }

    static UI::Components::Button::Style DangerHover()
    {
        auto s = DangerNormal();
        s.background = sf::Color(200, 70, 80);
        s.borderColor = sf::Color(255, 255, 255, 60);
        return s;
    }

    static UI::Components::Button::Style DangerPressed()
    {
        auto s = DangerNormal();
        s.background = sf::Color(140, 40, 50);
        s.borderColor = sf::Color(255, 255, 255, 10);
        return s;
    }

    static UI::Components::Block::Style PanelStyle()
    {
        UI::Components::Block::Style s{};
        s.background = sf::Color(20, 22, 30, 210);
        s.borderThickness = 2.f;
        s.borderColor = sf::Color(255, 255, 255, 18);
        s.radius = UI::Components::Block::BorderRadius::Uniform(14.f);
        s.radiusQuality = 14;
        s.shadow = { .enabled = false };
        return s;
    }

    static UI::Components::Block::Style PanelHoverStyle()
    {
        auto s = PanelStyle();
        s.background = sf::Color(24, 26, 36, 220);
        s.borderColor = sf::Color(255, 255, 255, 26);
        return s;
    }

    // ============================================================
    // MainMenu
    // ============================================================

    void MainMenu::Initialise()
    {
        Log()->Debug("Initializing MainMenu page");

        ui.title = UI::Components::Text::Create({
            .text = "Snake",
            .font = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 56,
            .position = { center_.x, 90.f },
            .hAlign = UI::Components::Text::HAlign::Center,
            .vAlign = UI::Components::Text::VAlign::Center,
            .color = sf::Color::White,
            .enableFadeIn = true,
            .fadeInDuration = 0.35f
        });

        BuildLayout();
    }

    void MainMenu::OnAllInterfacesLoaded()
    {
        client_         = IFace().Get<Client>();
        gameController_ = IFace().Get<GameController>();
    }

    void MainMenu::ProcessTick()
    {
        if (gameController_->GetMainState() != Game::MainState_Menu)
            return;

        const auto now = std::chrono::steady_clock::now();
        if (now - lastUpdate_ >= 10s)
        {
            LoadStats();
            lastUpdate_ = std::chrono::steady_clock::now();
        }
    }

    void MainMenu::HandleEvent(sf::Event& event, sf::RenderWindow& window)
    {
        ui.container->HandleEvent(event, window);
        ui.playBig->HandleEvent(event, window);

        ui.accountPanel->HandleEvent(event, window);
        ui.profileSettings->HandleEvent(event, window);
        ui.logout->HandleEvent(event, window);

        ui.pagePrev->HandleEvent(event, window);
        ui.pageNext->HandleEvent(event, window);

        for (auto& l : ui.lobbyRows)
        {
            if (!l.visible) continue;
            l.row->HandleEvent(event, window);
            l.play->HandleEvent(event, window);
        }
    }

    void MainMenu::UpdateScene()
    {
        auto& window = GetController()->Window();
        if (!window.isOpen())
            return;

        auto& profile = gameController_->GetProfile();
        ui.accountLogin->SetText("User: " + profile.login_);
        ui.accountMaxExp->SetText("Max exp: " + std::to_string(profile.experience_));

        // Update
        ui.title->Update();
        ui.container->Update(window);
        ui.playBig->Update(window);

        ui.accountPanel->Update(window);
        ui.accountTitle->Update();
        ui.accountLogin->Update();
        ui.accountMaxExp->Update();
        ui.profileSettings->Update(window);
        ui.logout->Update(window);

        ui.lobbiesTitle->Update();

        for (auto& l : ui.lobbyRows)
        {
            if (!l.visible) continue;
            l.row->Update(window);
            l.title->Update();
            l.players->Update();
            l.play->Update(window);
        }

        if (sessions_.empty())
            ui.noLobbies->Update();

        ui.pagePrev->Update(window);
        ui.pageInfo->Update();
        ui.pageNext->Update(window);

        // Draw order
        for (auto* d : ui.title->Drawables())     window.draw(*d);
        for (auto* d : ui.container->Drawables()) window.draw(*d);
        for (auto* d : ui.playBig->Drawables())   window.draw(*d);

        for (auto* d : ui.accountPanel->Drawables())    window.draw(*d);
        for (auto* d : ui.accountTitle->Drawables())    window.draw(*d);
        for (auto* d : ui.accountLogin->Drawables())    window.draw(*d);
        for (auto* d : ui.accountMaxExp->Drawables())   window.draw(*d);
        for (auto* d : ui.profileSettings->Drawables()) window.draw(*d);
        for (auto* d : ui.logout->Drawables())          window.draw(*d);

        for (auto* d : ui.lobbiesTitle->Drawables()) window.draw(*d);

        for (auto& l : ui.lobbyRows)
        {
            if (!l.visible) continue;
            for (auto* d : l.row->Drawables())     window.draw(*d);
            for (auto* d : l.title->Drawables())   window.draw(*d);
            for (auto* d : l.players->Drawables()) window.draw(*d);
            for (auto* d : l.play->Drawables())    window.draw(*d);
        }

        if (sessions_.empty())
            for (auto* d : ui.noLobbies->Drawables()) window.draw(*d);

        for (auto* d : ui.pagePrev->Drawables())  window.draw(*d);
        for (auto* d : ui.pageInfo->Drawables())  window.draw(*d);
        for (auto* d : ui.pageNext->Drawables())  window.draw(*d);
    }

    void MainMenu::BuildLayout()
    {
        // ─── Main container ──────────────────────────────────────────────
        ui.container = UI::Components::Block::Create({
            .position = containerCenter,
            .size     = containerSize,
            .enabled  = true
        });
        ui.container->SetStyles(PanelStyle(), PanelHoverStyle(), PanelHoverStyle(), PanelHoverStyle());

        // ─── Left: Big Play ───────────────────────────────────────────────
        ui.playBig = UI::Components::Button::Create({
            .position = { cLT.x + 250.f, cLT.y + 150.f },
            .size     = { 380.f, 100.f },
            .text     = "Play",
            .font     = "assets/fonts/Roboto-Regular.ttf"
        });
        auto pn = ButtonNormal();  pn.text.size = 32;
        auto ph = ButtonHover();   ph.text.size = 32;
        auto pp = ButtonPressed(); pp.text.size = 32;
        auto pd = ButtonDisabled();pd.text.size = 32;
        ui.playBig->SetStyles(pn, ph, pp, pd);
        ui.playBig->Events().HookEvent(UI::Components::Button::Event::Click) =
            std::function([this] { OnPlayClick(); });

        // ─── Right: Account panel ─────────────────────────────────────────
        const sf::Vector2f accSize   { 360.f, 250.f };
        const sf::Vector2f accCenter { cRT.x - 220.f, cLT.y + 160.f };
        const sf::Vector2f accLT     { accCenter.x - accSize.x * 0.5f,
                                       accCenter.y - accSize.y * 0.5f };

        ui.accountPanel = UI::Components::Block::Create({
            .position = accCenter,
            .size     = accSize,
            .enabled  = true
        });
        auto accS = PanelStyle();     accS.radius = UI::Components::Block::BorderRadius::Uniform(14.f);
        auto accH = PanelHoverStyle();accH.radius = UI::Components::Block::BorderRadius::Uniform(14.f);
        ui.accountPanel->SetStyles(accS, accH, accH, accH);

        ui.accountTitle = UI::Components::Text::Create({
            .text          = "Account",
            .font          = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 22,
            .position      = { accLT.x + 18.f, accLT.y + 18.f },
            .hAlign        = UI::Components::Text::HAlign::Left,
            .vAlign        = UI::Components::Text::VAlign::Top,
            .color         = sf::Color(255, 255, 255, 220)
        });

        ui.accountLogin = UI::Components::Text::Create({
            .text          = "User: —",
            .font          = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 20,
            .position      = { accLT.x + 18.f, accLT.y + 58.f },
            .hAlign        = UI::Components::Text::HAlign::Left,
            .vAlign        = UI::Components::Text::VAlign::Top,
            .color         = sf::Color(235, 235, 235)
        });

        ui.accountMaxExp = UI::Components::Text::Create({
            .text          = "Max exp: —",
            .font          = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 20,
            .position      = { accLT.x + 18.f, accLT.y + 88.f },
            .hAlign        = UI::Components::Text::HAlign::Left,
            .vAlign        = UI::Components::Text::VAlign::Top,
            .color         = sf::Color(235, 235, 235)
        });

        ui.profileSettings = UI::Components::Button::Create({
            .position = { accCenter.x, accLT.y + 150.f },
            .size     = { 250.f, 40.f },
            .text     = "Profile settings",
            .font     = "assets/fonts/Roboto-Regular.ttf"
        });
        ui.profileSettings->SetStyles(ButtonNormal(), ButtonHover(), ButtonPressed(), ButtonDisabled());
        ui.profileSettings->Events().HookEvent(UI::Components::Button::Event::Click) =
            std::function<void()>([] {});

        ui.logout = UI::Components::Button::Create({
            .position = { accCenter.x, accLT.y + 215.f },
            .size     = { 250.f, 40.f },
            .text     = "Logout",
            .font     = "assets/fonts/Roboto-Regular.ttf"
        });
        ui.logout->SetStyles(DangerNormal(), DangerHover(), DangerPressed(), ButtonDisabled());
        ui.logout->Events().HookEvent(UI::Components::Button::Event::Click) =
            std::function<void()>([this] { OnLogoutClick(); });

        // ─── Bottom: Lobbies title ────────────────────────────────────────
        ui.lobbiesTitle = UI::Components::Text::Create({
            .text          = "Available lobbies",
            .font          = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 22,
            .position      = { containerCenter.x, cLT.y + 300.f },
            .hAlign        = UI::Components::Text::HAlign::Center,
            .vAlign        = UI::Components::Text::VAlign::Center,
            .color         = sf::Color(255, 255, 255, 220)
        });

        // ─── Lobby rows (fixed slots) ─────────────────────────────────────
        const float rowW    = containerSize.x - 120.f;
        const float rowH    = 62.f;
        const float gap     = 10.f;
        const float rowSlot = rowH + gap;
        const float listTop = cLT.y + 330.f;

        const float rowLeft  = containerCenter.x - rowW * 0.5f;
        const float rowRight = containerCenter.x + rowW * 0.5f;

        const sf::Vector2f playSize { 120.f, 44.f };
        const float playCenterX = rowRight - 22.f - playSize.x * 0.5f;

        for (int i = 0; i < kRowsPerPage; ++i)
        {
            const float rowCenterY = listTop + static_cast<float>(i) * rowSlot + rowH * 0.5f;
            auto& l = ui.lobbyRows[static_cast<size_t>(i)];

            l.row = UI::Components::Block::Create({
                .position = { containerCenter.x, rowCenterY },
                .size     = { rowW, rowH },
                .enabled  = true
            });
            auto rowN = PanelStyle();
            rowN.radius = UI::Components::Block::BorderRadius::Uniform(12.f);
            rowN.borderColor = sf::Color(255, 255, 255, 14);
            auto rowHov = PanelHoverStyle();
            rowHov.radius = UI::Components::Block::BorderRadius::Uniform(12.f);
            rowHov.borderColor = sf::Color(255, 255, 255, 22);
            l.row->SetStyles(rowN, rowHov, rowHov, rowHov);

            l.title = UI::Components::Text::Create({
                .text          = "",
                .font          = "assets/fonts/Roboto-Regular.ttf",
                .characterSize = 20,
                .position      = { rowLeft + 22.f, rowCenterY },
                .hAlign        = UI::Components::Text::HAlign::Left,
                .vAlign        = UI::Components::Text::VAlign::Center,
                .color         = sf::Color(240, 240, 240)
            });

            l.players = UI::Components::Text::Create({
                .text          = "",
                .font          = "assets/fonts/Roboto-Regular.ttf",
                .characterSize = 18,
                .position      = { playCenterX - playSize.x * 0.5f - 18.f, rowCenterY },
                .hAlign        = UI::Components::Text::HAlign::Right,
                .vAlign        = UI::Components::Text::VAlign::Center,
                .color         = sf::Color(200, 200, 200)
            });

            l.play = UI::Components::Button::Create({
                .position = { playCenterX, rowCenterY },
                .size     = playSize,
                .text     = "Play",
                .font     = "assets/fonts/Roboto-Regular.ttf"
            });
            auto bn = ButtonNormal();  bn.text.size = 18;
            auto bh = ButtonHover();   bh.text.size = 18;
            auto bp = ButtonPressed(); bp.text.size = 18;
            auto bd = ButtonDisabled();bd.text.size = 18;
            l.play->SetStyles(bn, bh, bp, bd);

            l.visible = false;
        }

        // ─── "No lobbies" placeholder ─────────────────────────────────────
        const float listMidY = listTop + (static_cast<float>(kRowsPerPage) * rowSlot - gap) * 0.5f;
        ui.noLobbies = UI::Components::Text::Create({
            .text          = "No lobbies available",
            .font          = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 20,
            .position      = { containerCenter.x, listMidY },
            .hAlign        = UI::Components::Text::HAlign::Center,
            .vAlign        = UI::Components::Text::VAlign::Center,
            .color         = sf::Color(180, 180, 180, 160)
        });

        // ─── Pagination ───────────────────────────────────────────────────
        const float pageY = cLT.y + 562.f;

        ui.pagePrev = UI::Components::Button::Create({
            .position = { containerCenter.x - 90.f, pageY },
            .size     = { 60.f, 36.f },
            .text     = "<",
            .font     = "assets/fonts/Roboto-Regular.ttf"
        });
        auto prevN = ButtonNormal();  prevN.text.size = 20;
        auto prevH = ButtonHover();   prevH.text.size = 20;
        auto prevP = ButtonPressed(); prevP.text.size = 20;
        auto prevD = ButtonDisabled();prevD.text.size = 20;
        ui.pagePrev->SetStyles(prevN, prevH, prevP, prevD);
        ui.pagePrev->Events().HookEvent(UI::Components::Button::Event::Click) =
            std::function([this] {
                if (currentPage_ > 0) { --currentPage_; RebuildPage(); }
            });

        ui.pageInfo = UI::Components::Text::Create({
            .text          = "0 / 0",
            .font          = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 18,
            .position      = { containerCenter.x, pageY },
            .hAlign        = UI::Components::Text::HAlign::Center,
            .vAlign        = UI::Components::Text::VAlign::Center,
            .color         = sf::Color(200, 200, 200)
        });

        ui.pageNext = UI::Components::Button::Create({
            .position = { containerCenter.x + 90.f, pageY },
            .size     = { 60.f, 36.f },
            .text     = ">",
            .font     = "assets/fonts/Roboto-Regular.ttf"
        });
        auto nextN = ButtonNormal();  nextN.text.size = 20;
        auto nextH = ButtonHover();   nextH.text.size = 20;
        auto nextP = ButtonPressed(); nextP.text.size = 20;
        auto nextD = ButtonDisabled();nextD.text.size = 20;
        ui.pageNext->SetStyles(nextN, nextH, nextP, nextD);
        ui.pageNext->Events().HookEvent(UI::Components::Button::Event::Click) =
            std::function([this] {
                const int totalPages = (static_cast<int>(sessions_.size()) + kRowsPerPage - 1) / kRowsPerPage;
                if (currentPage_ < totalPages - 1) { ++currentPage_; RebuildPage(); }
            });

        RebuildPage();
    }

    // ─── Data ─────────────────────────────────────────────────────────────

    void MainMenu::LoadStats()
    {
        gameController_->GetStats() = [this](const Game::ActionResult<GameController::Stats>& stats) {
            if (!stats.success)
            {
                Log()->Error("GetStats failed: {}", stats.error);
                return;
            }

            sessions_.assign(stats.result.sessions_.begin(), stats.result.sessions_.end());
            RebuildPage();
        };
    }

    void MainMenu::RebuildPage()
    {
        const int total      = static_cast<int>(sessions_.size());
        const int totalPages = (total + kRowsPerPage - 1) / kRowsPerPage;

        // Clamp page index
        if (totalPages > 0 && currentPage_ >= totalPages)
            currentPage_ = totalPages - 1;
        if (currentPage_ < 0)
            currentPage_ = 0;

        // Fill visible slots from current page
        for (int slot = 0; slot < kRowsPerPage; ++slot)
        {
            const int idx = currentPage_ * kRowsPerPage + slot;
            auto& l = ui.lobbyRows[static_cast<size_t>(slot)];

            if (idx < total)
            {
                const auto& s = sessions_[static_cast<size_t>(idx)];
                l.title->SetText("Lobby #" + std::to_string(s.sessionID_));
                l.players->SetText(std::to_string(s.players_) + " players");
                // Re-bind click with correct session id for this slot
                l.play->Events().HookEvent(UI::Components::Button::Event::Click) =
                    std::function([id = s.sessionID_, this] { OnPlaySessionClick(id); });
                l.visible = true;
            }
            else
            {
                l.visible = false;
            }
        }

        // Pagination controls
        const int pageDisplay = (totalPages == 0) ? 0 : currentPage_ + 1;
        ui.pageInfo->SetText(std::to_string(pageDisplay) + " / " + std::to_string(totalPages));

        ui.pagePrev->SetEnabled(currentPage_ > 0);
        ui.pageNext->SetEnabled(totalPages > 0 && currentPage_ < totalPages - 1);
    }

    // ─── Handlers ─────────────────────────────────────────────────────────

    void MainMenu::OnPlayClick()
    {
        // Join the first available session, or session #1 as fallback
        const uint32_t id = sessions_.empty() ? 1u : sessions_.front().sessionID_;
        OnPlaySessionClick(id);
    }

    void MainMenu::OnPlaySessionClick(const uint32_t id)
    {
        gameController_->JoinSession(id) = [=, this](const Game::ActionResult<>& result) {
            Log()->Debug("OnPlaySessionClick({}): {}", id, result.success ? "success" : "failed");
        };
    }

    void MainMenu::OnLogoutClick()
    {
        gameController_->Logout();
    }

} // namespace Core::App::Render::Pages
