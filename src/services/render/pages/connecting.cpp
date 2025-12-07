#include "connecting.hpp"

#include <cmath>

namespace Core::App::Render::Pages {

    [[maybe_unused]] __used Utils::Service::Loader::Add<Connecting> ConnectingPage(PagesLoader());

    void Connecting::Initialise()
    {
        Log()->Debug("Initializing Connecting page");

        ui.loader_ = UI::Components::Loader::Create({center_});
        ui.text_ = UI::Components::Text::Create({
            .text= "Connecting...",
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

    void Connecting::OnAllInterfacesLoaded()
    {
        client_ = IFace().Get<Client>();

        client_->RegisterConnectionStateCallback([this](const ConnectionState & old, const ConnectionState & current) {
            connectionState_ = current;

            if (connectionState_ == ConnectionState::ConnectionState_ConnectingFailed)
            {
                connectionErrors_++;
                ui.errorText_->SetText(std::format("[{}] {}", connectionErrors_, client_->GetConnectionError()));
            }
            else
            {
                connectionErrors_ = 0;
            }
        });
    }

    void Connecting::UpdateScene()
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

        if (connectionState_ == ConnectionState::ConnectionState_ConnectingFailed)
        {
            for (const auto & drawable : ui.errorText_->Drawables())
                window.draw(*drawable);
        }
    }

} // namespace Core::App::Render::Pages