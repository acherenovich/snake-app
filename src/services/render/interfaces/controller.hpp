#pragma once

#include "common.hpp"

namespace Core::App::Render
{
    namespace Interface
    {
        class Controller: public virtual BaseServiceInstance
        {
        public:
            using Shared = std::shared_ptr<Controller>;

        };
    }
}
