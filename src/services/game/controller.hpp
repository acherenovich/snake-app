#pragma once

#include "interfaces/controller.hpp"

namespace Core::App::Game
{
    class Controller final : public Interface::Controller, public std::enable_shared_from_this<Controller>
    {
        MainState mainState_ = MainState_Connecting;
    public:
        using Shared = std::shared_ptr<Controller>;

        void Initialise() override;

        void OnAllServicesLoaded() override;

        void OnAllInterfacesLoaded() override;

        void ProcessTick() override;

        MainState GetMainState() const override;

    private:
        std::string GetServiceContainerName() const override
        {
            return "Game";
        }
    };
}