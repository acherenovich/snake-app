#pragma once

#include "interfaces/controller.hpp"

#include "network/websocket/interfaces/client.hpp"

namespace Core::App::Game
{
    class Controller final : public Interface::Controller, public std::enable_shared_from_this<Controller>
    {
        using Client = Network::Websocket::Interface::Client;
        using Message = Network::Websocket::Interface::Message;
        using ConnectionState = Network::Websocket::Interface::Client::ConnectionState;

        Client::Shared client_;
        ConnectionState connectionState_ = ConnectionState::ConnectionState_Connecting;
        MainState mainState_ = MainState_Connecting;
    public:
        using Shared = std::shared_ptr<Controller>;

        void Initialise() override;

        void OnAllServicesLoaded() override;

        void OnAllInterfacesLoaded() override;

        void ProcessTick() override;

        MainState GetMainState() const override;

        void SetMainState(const MainState & state);

    private:
        std::string GetServiceContainerName() const override
        {
            return "Game";
        }
    };
}