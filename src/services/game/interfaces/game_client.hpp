#pragma once

#include <unordered_set>

#include "[core_loader].hpp"

#include "legacy_logic.hpp"
#include "legacy_entities.hpp"

namespace Core::App::Game
{
    using Logic = Utils::Legacy::Game::Logic;
    using Snake = Utils::Legacy::Game::Interface::Entity::Snake;
    using Food = Utils::Legacy::Game::Interface::Entity::Food;

    struct DebugInfo
    {
        std::uint32_t foodsCount { 0 };
        std::uint32_t snakesCount { 0 };

        std::uint32_t lastFullPacketBytes { 0 };
        std::uint32_t lastPartialPacketBytes { 0 };

        std::uint32_t lastFullPayloadBytes { 0 };
        std::uint32_t lastPartialPayloadBytes { 0 };

        std::uint32_t lastServerSeq { 0 };

        bool pendingFullRequest { false };
        bool pendingFullRequestAllSegments { false };
        bool awaitingPlayerRebuild { false };

        std::uint32_t playerEntityID { 0 };

        std::uint32_t badPacketsDropped { 0 };
    };

    namespace Interface {
        class GameClient: public BaseServiceContainer
        {
        public:
            using Shared = std::shared_ptr<GameClient>;

            virtual Snake::Shared GetPlayerSnake() = 0;

            virtual std::unordered_set<Snake::Shared> GetNearestVictims() = 0;

            virtual std::unordered_set<Food::Shared> GetNearestFoods() = 0;

            virtual void ForceFullUpdateRequest() = 0;

            [[nodiscard]] virtual DebugInfo GetDebugInfo() const = 0;

            [[nodiscard]] virtual uint32_t GetServerFrame() const = 0;
        };
    }

}
