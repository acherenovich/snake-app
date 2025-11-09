#pragma once

#include "common.hpp"
#include "game/math.hpp"

#include <SFML/System/Vector2.hpp>

namespace Entity {
    class BaseEntity: public virtual Interface::Entity::BaseEntity {
    protected:
        uint32_t frameCreated = 0, frameKilled = 0;
    public:
        using Shared = std::shared_ptr<BaseEntity>;

        [[nodiscard]] uint32_t FrameCreated() const override
        {
            return frameCreated;
        }

        [[nodiscard]] uint32_t FrameKilled() const override
        {
            return frameKilled;
        }

        void Kill(uint32_t frame)
        {
            frameKilled = frame;
        }

        [[nodiscard]] bool IsKilled() const override
        {
            return !!frameKilled;
        }
    };
}