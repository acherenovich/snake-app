#pragma once

#include "common.hpp"

namespace Core::App::Game
{
    namespace Interface
    {
        class Controller: public BaseServiceInterface, public BaseServiceInstance
        {
        public:
            using Shared = std::shared_ptr<Controller>;

            [[nodiscard]] virtual MainState GetMainState() const = 0;
        };
    }
}
