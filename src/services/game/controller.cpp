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

    }

    void Controller::ProcessTick()
    {
    }

    MainState Controller::GetMainState() const
    {
        return mainState_;
    }


}