#include "joining_session.hpp"

#include <cmath>

namespace Core::App::Render::Pages {

    [[maybe_unused]] [[gnu::used]] Utils::Service::Loader::Add<JoiningSession> JoiningSessionPage(PagesLoader());

    void JoiningSession::Initialise()
    {
        Log()->Debug("Initializing JoiningSession page");

        ui.loader_ = UI::Components::Loader::Create({center_});
        ui.text_ = UI::Components::Text::Create({
            .text= "JoiningSession...",
            .font = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 32,

            .position = {center_.x, center_.y + ui.loader_->GetConfig().radius * 2},

            .color = sf::Color(255, 255, 255, 200),
            .enablePulseAlpha = true,
        });
        ui.errorText_ = UI::Components::Text::Create({
            .characterSize = 20,

            .position = {center_.x, center_.y + ui.loader_->GetConfig().radius * 3},

            .color = sf::Color(255, 20, 20, 150),
        });
    }

    void JoiningSession::OnAllInterfacesLoaded()
    {
    }

    void JoiningSession::UpdateScene()
    {
        auto& window = GetController()->Window();
        if (!window.isOpen())
            return;

        ui.loader_->Update();
        ui.text_->Update();
        ui.errorText_->Update();

        for (const auto & drawable : ui.loader_->Drawables())
            window.draw(*drawable);

        for (const auto & drawable : ui.text_->Drawables())
            window.draw(*drawable);
    }

} // namespace Core::App::Render::Pages