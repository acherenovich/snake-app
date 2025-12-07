#include "authorization.hpp"

#include <cmath>

namespace Core::App::Render::Pages {

    [[maybe_unused]] __used Utils::Service::Loader::Add<Authorization> AuthorizationPage(PagesLoader());

    void Authorization::Initialise()
    {
        Log()->Debug("Initializing Connecting page");

    }

    void Authorization::OnAllInterfacesLoaded()
    {
        client_ = IFace().Get<Client>();

        client_->RegisterConnectionStateCallback([this](const ConnectionState & old, const ConnectionState & current) {
            connectionState_ = current;

            if (connectionState_ == ConnectionState::ConnectionState_ConnectingFailed)
            {
            }
            else
            {
            }
        });
    }

    void Authorization::UpdateScene()
    {
        auto& window = GetController()->Window();
        if (!window.isOpen())
            return;

    }

} // namespace Core::App::Render::Pages