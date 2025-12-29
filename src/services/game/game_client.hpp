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

        EntitySnake::Shared clientSnake_;

        struct NetState
        {
            std::uint32_t lastServerSeq { 0 };
            bool hasSeq { false };

            bool pendingFullRequest { false };
            bool pendingFullRequestAllSegments { false };

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

        bool awaitingPlayerRebuild_ { false };

        // snake snapshot requests (pointed repair)
        std::unordered_map<std::uint32_t, std::uint32_t> snakeSnapshotCooldownFrame_; // entityID -> nextAllowedFrame
        std::unordered_set<std::uint32_t> pendingSnakeSnapshots_; // entityIDs to request

        std::chrono::steady_clock::time_point dateCreated = std::chrono::steady_clock::time_point::clock::now();

        bool connected_ { false };
        bool disconnected_ { false };

        std::function<void(uint64_t sessionID)> connectCallback_;
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

        bool IsLoaded() const;

        bool IsTimeout() const;

        void SetConnectCallback(std::function<void(uint64_t sessionID)> callback);
    public:
        Snake::Shared GetPlayerSnake() override;

        std::unordered_set<Snake::Shared> GetNearestVictims() override;

        std::unordered_set<Food::Shared> GetNearestFoods() override;

        void ForceFullUpdateRequest() override;

        [[nodiscard]] DebugInfo GetDebugInfo() const override;

        [[nodiscard]] uint32_t GetServerFrame() const override;


        static Shared Create(const BaseServiceContainer * parent, std::uint8_t serverID);

    private:
        void ClearWorld();

        void UpsertSnakeFull(const std::uint32_t entityID,
                             const Utils::Legacy::Game::Net::SnakeState& ss,
                             const std::vector<sf::Vector2f>& fullSegments,
                             const bool isNew);

        void ApplySnakeValidationUpdate(const std::uint32_t entityID,
                                        const Utils::Legacy::Game::Net::SnakeState& ss,
                                        const std::vector<sf::Vector2f>& samples,
                                        const bool isNew);

        void UpsertFood(const std::uint32_t entityID,
                        const Utils::Legacy::Game::Net::FoodState& fs,
                        const bool isNew);

        void RemoveEntity(const Utils::Legacy::Game::Net::EntityType type,
                          const std::uint32_t entityID);

        void ApplyFullUpdate(Utils::Legacy::Game::Net::ByteReader& reader);

        void ApplyPartialUpdate(Utils::Legacy::Game::Net::ByteReader& reader);

        void ApplySnakeSnapshot(Utils::Legacy::Game::Net::ByteReader& reader);

        void QueueSnakeSnapshotRequest(const std::uint32_t entityID);

        [[nodiscard]] float GetVisibleRadiusWithPadding() const;

    private:
        std::string GetServiceContainerName() const override
        {
            return "GameClient";
        }
    };

    // ===== helpers (non-static) =====

    std::vector<sf::Vector2f> ReadSnakePoints(Utils::Legacy::Game::Net::ByteReader& r, std::uint16_t count);

    std::vector<sf::Vector2f> BuildExpectedSamplesByRadius(const std::list<sf::Vector2f>& segments,
                                                           const float minDist);

    bool ValidateSamplesByRadius(const std::list<sf::Vector2f>& predicted,
                                 const std::vector<sf::Vector2f>& serverSamples,
                                 const float minDist,
                                 const float threshold);

} // namespace Core::App::Game