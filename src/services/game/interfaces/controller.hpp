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
            using Shared = std::shared_ptr<Controller>;

            [[nodiscard]] virtual MainState GetMainState() const = 0;

            virtual Utils::Task<ActionResult<>> PerformLogin(std::string login, std::string password, bool save) = 0;

            virtual Utils::Task<ActionResult<>> PerformLogin(std::string token) = 0;

            virtual Utils::Task<ActionResult<>> PerformRegister(std::string login, std::string password) = 0;

            virtual Utils::Task<ActionResult<>> JoinSession(uint32_t sessionID) = 0;

            [[nodiscard]] virtual GameClient::Shared GetCurrentGameClient() const = 0;
        };
    }
}
