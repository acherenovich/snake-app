#pragma once

#include "interfaces/game_client.hpp"

#include "udp.hpp"
#include "game_messages.hpp"

#include <unordered_map>
#include <unordered_set>

namespace Core::App::Game
{
    using UdpClient   = Utils::Net::Udp::Client;
    using UdpListener = Utils::Net::Udp::ClientListener;

    using EntitySnake = Utils::Legacy::Game::Entity::Snake;
    using EntityFood  = Utils::Legacy::Game::Entity::Food;

    class GameClient final :
        public Interface::GameClient,
        public Logic,
        public UdpListener,
        public std::enable_shared_from_this<GameClient>
    {
        UdpClient::Shared udpClient_;

        Snake::Shared clientSnake_;

        struct NetState
        {
            std::uint32_t lastServerSeq { 0 };
            bool hasSeq { false };

            bool pendingFullRequest { false };
            bool pendingFullRequestAllSegments { false }; // NEW

            std::uint32_t lastInputSeq { 0 };
        };

        struct SnakePredictState
        {
            std::uint32_t lastSeq { 0 };
        };

        struct EntityTTL
        {
            std::uint32_t lastSeenSeq { 0 };
        };

        std::unordered_map<std::uint32_t, EntityTTL> snakesTTL_;
        std::unordered_map<std::uint32_t, EntityTTL> foodsTTL_;

        NetState net_;

        std::unordered_map<std::uint32_t, EntitySnake::Shared> snakesByID_;
        std::unordered_map<std::uint32_t, EntityFood::Shared>  foodsByID_;

        std::unordered_map<std::uint32_t, SnakePredictState> predict_;

        std::uint32_t playerEntityID_ { 0 };

        float visibilityPaddingPercent_ { 0.20f };

        // debug / safety
        std::uint32_t lastFullPacketBytes_ { 0 };
        std::uint32_t lastPartialPacketBytes_ { 0 };
        std::uint32_t lastFullPayloadBytes_ { 0 };
        std::uint32_t lastPartialPayloadBytes_ { 0 };
        std::uint32_t badPacketsDropped_ { 0 };

        bool awaitingPlayerRebuild_ { false }; // NEW: после full(allSegments) не валидируем player пока не построили

    public:
        using Shared = std::shared_ptr<GameClient>;

        GameClient();

        void Initialise(std::uint8_t serverID);

        void ProcessTick() override;

    public:
        void OnConnected() override;

        void OnDisconnected() override;

        void OnConnectionError(const std::string & error, bool reconnect) override;

        void OnMessage(const std::vector<std::uint8_t>& data) override;

    public:
        Snake::Shared GetPlayerSnake() override;

        std::unordered_set<Snake::Shared> GetNearestVictims() override;

        std::unordered_set<Food::Shared> GetNearestFoods() override;

        void ForceFullUpdateRequest() override;

        [[nodiscard]] DebugInfo GetDebugInfo() const override;

        static Shared Create(const BaseServiceContainer * parent, std::uint8_t serverID);

    private:
        void ClearWorld();

        void UpsertSnake(const std::uint32_t entityID,
                         const Utils::Legacy::Game::Net::SnakeState& ss,
                         const std::vector<sf::Vector2f>& samples,
                         bool isNew);

        void UpsertFood(const std::uint32_t entityID,
                        const Utils::Legacy::Game::Net::FoodState& fs,
                        bool isNew);

        void RemoveEntity(const Utils::Legacy::Game::Net::EntityType type,
                          const std::uint32_t entityID);

        void ApplyFullUpdate(Utils::Legacy::Game::Net::ByteReader& reader);

        void ApplyPartialUpdate(Utils::Legacy::Game::Net::ByteReader& reader);

        [[nodiscard]] float GetVisibleRadiusWithPadding() const;
    };

    // ===== helpers (non-static) =====

    std::vector<sf::Vector2f> ReadSnakeSamples(Utils::Legacy::Game::Net::ByteReader& r, std::uint8_t count);

    std::vector<sf::Vector2f> BuildExpectedSampleIndices(const std::list<sf::Vector2f>& segments,
                                                         std::size_t desiredCount);

    bool ValidateSamples(const std::list<sf::Vector2f>& predicted,
                         const std::vector<sf::Vector2f>& serverSamples,
                         float threshold);

} // namespace Core::App::Game