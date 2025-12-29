#pragma once

#include "common.hpp"
#include "game_client.hpp"

#include "coroutine.hpp"

namespace Core::App::Game
{
    namespace Interface
    {
        class Controller: public BaseServiceInterface, public BaseServiceInstance
        {
        public:
            struct Profile
            {
                bool loggedIn_ = false;
                uint32_t playerID_ = 0;
                uint32_t experience_ = 0;
                std::string login_;

                void Login(const std::string & login, const uint32_t playerID, const uint32_t experience)
                {
                    loggedIn_ = true;
                    login_ = login;
                    playerID_ = playerID;
                    experience_ = experience;
                }

                void Logout()
                {
                    loggedIn_ = false;
                }
            };

            struct Stats
            {
                struct Session
                {
                    uint32_t sessionID_ = 0;
                    uint32_t players_ = 0;
                };

                std::vector<Session> sessions_;
            };
        public:
            using Shared = std::shared_ptr<Controller>;

            [[nodiscard]] virtual MainState GetMainState() const = 0;

            virtual Utils::Task<ActionResult<>> PerformLogin(std::string login, std::string password, bool save) = 0;

            virtual Utils::Task<ActionResult<>> PerformLogin(std::string token) = 0;

            virtual Utils::Task<ActionResult<>> PerformRegister(std::string login, std::string password) = 0;

            virtual void Logout() = 0;

            virtual Utils::Task<ActionResult<Stats>> GetStats() = 0;

            virtual Utils::Task<ActionResult<std::unordered_map<std::string, uint32_t>>> GetLeaderboard() = 0;

            virtual Utils::Task<ActionResult<>> JoinSession(uint32_t sessionID) = 0;

            [[nodiscard]] virtual GameClient::Shared GetCurrentGameClient() const = 0;

            [[nodiscard]] virtual const Profile & GetProfile() const = 0;

            virtual void ExitToMenu() = 0;
        };
    }
}
