#pragma once

#include "interfaces/controller.hpp"

#include "network/websocket/interfaces/client.hpp"
#include "components/local_storage/interfaces/storage.hpp"

#include "game_client.hpp"

namespace Core::App::Game
{
    class Controller final : public Interface::Controller, public std::enable_shared_from_this<Controller>
    {
        using Client = Network::Websocket::Interface::Client;
        using Message = Network::Websocket::Interface::Message;
        using ConnectionState = Network::Websocket::Interface::Client::ConnectionState;
        using Storage = Components::LocalStorage::Interface::Storage;

        Client::Shared client_;
        ConnectionState connectionState_ = ConnectionState::ConnectionState_Connecting;
        MainState mainState_ = MainState_Connecting;

        Storage::Shared localStorage_;

        GameClient::Shared gameClient_;

        Profile profile_;

        uint32_t serverID_ = 0;

    public:
        using Shared = std::shared_ptr<Controller>;

        void Initialise() override;

        void OnAllServicesLoaded() override;

        void OnAllInterfacesLoaded() override;

        void ProcessTick() override;

        MainState GetMainState() const override;

        void SetMainState(const MainState & state);

        Utils::Task<ActionResult<>> PerformLogin(std::string login, std::string password, bool save) override;

        Utils::Task<ActionResult<>> PerformLogin(std::string token) override;

        Utils::Task<ActionResult<>> PerformRegister(std::string login, std::string password) override;

        void Logout() override;

        Utils::Task<ActionResult<Stats>> GetStats() override;

        Utils::Task<ActionResult<std::unordered_map<std::string, uint32_t>>> GetLeaderboard() override;

        Utils::Task<ActionResult<>> JoinSession(uint32_t sessionID) override;

        Utils::Task<ActionResult<>> SessionJoined(uint32_t serverID, uint64_t ssid);

        Interface::GameClient::Shared GetCurrentGameClient() const override;

        const Profile & GetProfile() const override;

        void ExitToMenu() override;
    private:
        std::string GetServiceContainerName() const override
        {
            return "Game";
        }
    };
}