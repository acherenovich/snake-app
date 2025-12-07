#include "controller.hpp"

namespace Core::App::Game
{
    void Controller::Initialise()
    {

    }

    void Controller::OnAllServicesLoaded()
    {
        IFace().Register<Interface::Controller>(shared_from_this());
    }

    void Controller::OnAllInterfacesLoaded()
    {
        client_ = IFace().Get<Client>();

        client_->RegisterConnectionStateCallback([this](const ConnectionState & old, const ConnectionState & current) {
            connectionState_ = current;

            if (connectionState_ == ConnectionState::ConnectionState_Connected)
            {
                SetMainState(MainState_Login);
            }
        });
    }

    void Controller::ProcessTick()
    {
    }

    MainState Controller::GetMainState() const
    {
        return mainState_;
    }

    void Controller::SetMainState(const MainState & state)
    {
        mainState_ = state;
    }
}