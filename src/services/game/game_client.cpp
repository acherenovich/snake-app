#include "game_client.hpp"

#include <cmath>
#include <utility>
#include <vector>
#include <algorithm>

#include "utils.hpp"

using namespace std::chrono_literals;

namespace Core::App::Game
{
    GameClient::GameClient()
    {
    }

    void GameClient::Initialise(const std::uint8_t serverID)
    {
        auto host = Utils::Env("WS_HOST");
        if (host.empty())
            host = "127.0.0.1";

        Utils::Net::Udp::ClientConfig cfg;
        cfg.host = host;
        cfg.port = 7777 + serverID;
        cfg.mode = Utils::Net::Udp::Mode::Bytes;
        cfg.ioThreads = 2;

        udpClient_ = UdpClient::Create(cfg, shared_from_this());
    }

    void GameClient::ProcessTick()
    {
        Logic::ProcessTick();

        if (udpClient_)
        {
            udpClient_->ProcessTick();
        }

        // send input at 32 tickrate (logic tick 64)
        if (frame_ % 2 == 0)
        {
            if (clientSnake_)
            {
                using namespace Utils::Legacy::Game::Net;

                ByteWriter payload(sizeof(ClientInputPayload));

                ClientInputPayload input{};
                input.destinationX = clientSnake_->GetDestination().x;
                input.destinationY = clientSnake_->GetDestination().y;
                input.clientFrame  = frame_;

                payload.WritePod(input);

                net_.lastInputSeq++;

                const auto msg = BuildMessage(MessageType::ClientInput,
                                              net_.lastInputSeq,
                                              0,
                                              payload.Data());

                udpClient_->Send(msg);
            }

            // pending full update request
            if (net_.pendingFullRequest)
            {
                using namespace Utils::Legacy::Game::Net;

                ByteWriter payload(sizeof(RequestFullUpdatePayload));

                RequestFullUpdatePayload rq{};
                if (net_.pendingFullRequestAllSegments)
                {
                    rq.flags |= RequestFullUpdateFlag_AllSegments;
                }

                payload.WritePod(rq);

                net_.lastInputSeq++;

                const auto msg = BuildMessage(MessageType::RequestFullUpdate,
                                              net_.lastInputSeq,
                                              0,
                                              payload.Data());

                udpClient_->Send(msg);

                net_.pendingFullRequest = false;
                net_.pendingFullRequestAllSegments = false;
            }

            // pending snake snapshot requests (pointed repair)
            if (!pendingSnakeSnapshots_.empty())
            {
                using namespace Utils::Legacy::Game::Net;

                std::uint32_t sent = 0;
                constexpr std::uint32_t perTickLimit = 16;

                for (auto it = pendingSnakeSnapshots_.begin(); it != pendingSnakeSnapshots_.end() && sent < perTickLimit; )
                {
                    const auto entityID = *it;

                    net_.lastInputSeq++;

                    ByteWriter payload(sizeof(RequestSnakeSnapshotPayload));
                    RequestSnakeSnapshotPayload rq{};
                    rq.entityID = entityID;
                    payload.WritePod(rq);

                    const auto msg = BuildMessage(MessageType::RequestSnakeSnapshot,
                                                  net_.lastInputSeq,
                                                  0,
                                                  payload.Data());
                    udpClient_->Send(msg);

                    it = pendingSnakeSnapshots_.erase(it);
                    sent++;
                }
            }
        }

        // ===================== stale entity cleanup =====================
        if (!clientSnake_)
        {
            return;
        }

        constexpr std::uint32_t ttlSeqDelta = 8;
        const std::uint32_t currentSeq = net_.lastServerSeq;

        const sf::Vector2f playerPos = clientSnake_->GetPosition();
        const float radius = GetVisibleRadiusWithPadding();
        const float radiusSq = radius * radius;

        for (auto it = foodsByID_.begin(); it != foodsByID_.end(); )
        {
            const auto id = it->first;
            const auto food = it->second;

            const bool wasSeen = foodsTTL_.contains(id);
            const std::uint32_t lastSeen = wasSeen ? foodsTTL_[id].lastSeenSeq : 0;
            const bool stale = wasSeen && (currentSeq > lastSeen) && ((currentSeq - lastSeen) >= ttlSeqDelta);

            bool far = true;
            if (food)
            {
                const auto pos = food->GetPosition();
                const float dx = pos.x - playerPos.x;
                const float dy = pos.y - playerPos.y;
                far = (dx * dx + dy * dy) > radiusSq;
            }

            if (stale && far)
            {
                if (food)
                {
                    foods_.erase(food);
                }

                foodsTTL_.erase(id);
                it = foodsByID_.erase(it);
                continue;
            }

            ++it;
        }

        for (auto it = snakesByID_.begin(); it != snakesByID_.end(); )
        {
            const auto id = it->first;

            if (id == playerEntityID_)
            {
                ++it;
                continue;
            }

            const auto snake = it->second;

            const bool wasSeen = snakesTTL_.contains(id);
            const std::uint32_t lastSeen = wasSeen ? snakesTTL_[id].lastSeenSeq : 0;
            const bool stale = wasSeen && (currentSeq > lastSeen) && ((currentSeq - lastSeen) >= ttlSeqDelta);

            bool far = true;
            if (snake)
            {
                const auto pos = snake->GetPosition();
                const float dx = pos.x - playerPos.x;
                const float dy = pos.y - playerPos.y;
                far = (dx * dx + dy * dy) > radiusSq;
            }

            if (stale && far)
            {
                if (snake)
                {
                    snakes_.erase(snake);
                }

                snakesTTL_.erase(id);
                predict_.erase(id);
                snakeSnapshotCooldownFrame_.erase(id);

                it = snakesByID_.erase(it);
                continue;
            }

            ++it;
        }
    }

    void GameClient::OnConnected()
    {
        ClearWorld();

        connected_ = true;

        if (connectCallback_)
        {
            connectCallback_(udpClient_->SessionID());
            connectCallback_ = {};
        }
    }

    void GameClient::OnDisconnected()
    {
        ClearWorld();

        disconnected_ = true;
    }

    void GameClient::OnConnectionError(const std::string & error, const bool /*reconnect*/)
    {
        Log()->Error("Connection error: {}", error);
    }

    void GameClient::OnMessage(const std::vector<std::uint8_t>& data)
    {
        using namespace Utils::Legacy::Game::Net;

        MessageHeader header{};
        const auto parseErr = ParseHeaderDetailed(std::span(data.data(), data.size()), header);

        if (parseErr != ParseError::Ok)
        {
            badPacketsDropped_++;

            // try best-effort to read type/seq from bytes when possible
            std::uint16_t typeRaw = 0;
            std::uint32_t seqRaw = 0;
            if (data.size() >= sizeof(MessageHeader))
            {
                std::memcpy(&typeRaw, data.data() + 0, sizeof(typeRaw));
                std::memcpy(&seqRaw, data.data() + 4, sizeof(seqRaw)); // after type+version (4 bytes)
            }

            Log()->Warning("[Net] Dropped packet: ParseHeaderDetailed failed. err={} bytes={} dropped={} typeRaw={} seqRaw={}",
                           ParseErrorToString(parseErr), data.size(), badPacketsDropped_, typeRaw, seqRaw);

            // CRC/size/version mismatch means packet is unusable -> request world repair
            net_.pendingFullRequest = true;
            net_.pendingFullRequestAllSegments = true;
            return;
        }

        // safety: ensure payloadBytes fits buffer (already guaranteed by ParseHeaderDetailed size check)
        const std::size_t totalNeed = sizeof(MessageHeader) + static_cast<std::size_t>(header.payloadBytes);
        if (totalNeed > data.size())
        {
            badPacketsDropped_++;
            Log()->Warning("[Net] Dropped packet: payloadBytes out of bounds. bytes={} payloadBytes={} need={} dropped={} type={} seq={}",
                           data.size(), header.payloadBytes, totalNeed, badPacketsDropped_, header.type, header.seq);
            net_.pendingFullRequest = true;
            net_.pendingFullRequestAllSegments = true;
            return;
        }

        const auto type = static_cast<MessageType>(header.type);

        // stats
        if (type == MessageType::FullUpdate)
        {
            lastFullPacketBytes_ = static_cast<std::uint32_t>(data.size());
            lastFullPayloadBytes_ = header.payloadBytes;
        }
        else if (type == MessageType::PartialUpdate)
        {
            lastPartialPacketBytes_ = static_cast<std::uint32_t>(data.size());
            lastPartialPayloadBytes_ = header.payloadBytes;
        }

        // sequence check ONLY for Full/Partial updates (snapshot is out-of-band)
        if (type == MessageType::FullUpdate || type == MessageType::PartialUpdate)
        {
            if (net_.hasSeq)
            {
                if (header.seq != net_.lastServerSeq + 1)
                {
                    Log()->Warning("[Net] Server seq mismatch: got={} expected={} -> request full update",
                                   header.seq, net_.lastServerSeq + 1);

                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    net_.lastServerSeq = header.seq;
                }
                else
                {
                    net_.lastServerSeq = header.seq;
                }
            }
            else
            {
                net_.hasSeq = true;
                net_.lastServerSeq = header.seq;
            }
        }

        const auto payloadSpan = std::span<const std::uint8_t>(
            data.data() + sizeof(MessageHeader),
            header.payloadBytes
        );

        ByteReader reader(payloadSpan);

        if (type == MessageType::FullUpdate)
        {
            ApplyFullUpdate(reader);
            return;
        }

        if (type == MessageType::PartialUpdate)
        {
            ApplyPartialUpdate(reader);
            return;
        }

        if (type == MessageType::SnakeSnapshot)
        {
            ApplySnakeSnapshot(reader);
            return;
        }
    }

    bool GameClient::IsLoaded() const
    {
        return connected_;
    }

    bool GameClient::IsTimeout() const
    {
        if (disconnected_)
            return true;


        return false;
    }

    void GameClient::SetConnectCallback(std::function<void(uint64_t sessionID)> callback)
    {
        connectCallback_ = std::move(callback);
    }

    Snake::Shared GameClient::GetPlayerSnake()
    {
        return clientSnake_;
    }

    float GameClient::GetVisibleRadiusWithPadding() const
    {
        if (!clientSnake_)
        {
            return 0.0f;
        }

        const float visibleRadius = EntitySnake::camera_radius * clientSnake_->GetZoom();
        return visibleRadius * (1.0f + visibilityPaddingPercent_);
    }

    std::unordered_set<Snake::Shared> GameClient::GetNearestVictims()
    {
        std::unordered_set<Snake::Shared> nearestSnakes;

        if (!clientSnake_)
        {
            return nearestSnakes;
        }

        const sf::Vector2f playerPos = clientSnake_->GetPosition();
        const float radius = GetVisibleRadiusWithPadding();
        const float radiusSq = radius * radius;

        nearestSnakes.reserve(snakes_.size());

        for (const auto& s : snakes_)
        {
            if (!s)
                continue;

            if (s->EntityID() == playerEntityID_)
                continue;

            nearestSnakes.insert(std::static_pointer_cast<Snake>(s));
        }

        return nearestSnakes;
    }

    std::unordered_set<Food::Shared> GameClient::GetNearestFoods()
    {
        std::unordered_set<Food::Shared> nearestFoods;

        if (!clientSnake_)
        {
            return nearestFoods;
        }

        const sf::Vector2f playerPos = clientSnake_->GetPosition();
        const float radius = GetVisibleRadiusWithPadding();
        const float radiusSq = radius * radius;

        nearestFoods.reserve(foods_.size());

        for (const auto& f : foods_)
        {
            if (!f)
                continue;

            const auto pos = f->GetPosition();
            const float dx = pos.x - playerPos.x;
            const float dy = pos.y - playerPos.y;

            if ((dx * dx + dy * dy) <= radiusSq)
            {
                nearestFoods.insert(std::static_pointer_cast<Food>(f));
            }
        }

        return nearestFoods;
    }

    void GameClient::ForceFullUpdateRequest()
    {
        net_.pendingFullRequest = true;
        net_.pendingFullRequestAllSegments = true;
        awaitingPlayerRebuild_ = true;

        Log()->Warning("[Net] ForceFullUpdateRequest() -> pendingFullRequestAllSegments=true awaitingPlayerRebuild=true");
    }

    DebugInfo GameClient::GetDebugInfo() const
    {
        DebugInfo info{};
        info.foodsCount = static_cast<std::uint32_t>(foodsByID_.size());
        info.snakesCount = static_cast<std::uint32_t>(snakesByID_.size());

        info.lastFullPacketBytes = lastFullPacketBytes_;
        info.lastPartialPacketBytes = lastPartialPacketBytes_;

        info.lastFullPayloadBytes = lastFullPayloadBytes_;
        info.lastPartialPayloadBytes = lastPartialPayloadBytes_;

        info.lastServerSeq = net_.lastServerSeq;

        info.pendingFullRequest = net_.pendingFullRequest;
        info.pendingFullRequestAllSegments = net_.pendingFullRequestAllSegments;
        info.awaitingPlayerRebuild = awaitingPlayerRebuild_;

        info.playerEntityID = playerEntityID_;

        info.badPacketsDropped = badPacketsDropped_;

        return info;
    }

    uint32_t GameClient::GetServerFrame() const
    {
        return frame_;
    }


    GameClient::Shared GameClient::Create(const BaseServiceContainer * parent, const std::uint8_t serverID)
    {
        const auto obj = std::make_shared<GameClient>();
        obj->SetupContainer(parent);
        obj->Initialise(serverID);
        return obj;
    }

    // ===================== internal world update helpers =====================

    void GameClient::ClearWorld()
    {
        snakesByID_.clear();
        foodsByID_.clear();
        predict_.clear();

        snakes_.clear();
        foods_.clear();

        snakesTTL_.clear();
        foodsTTL_.clear();

        snakeSnapshotCooldownFrame_.clear();
        pendingSnakeSnapshots_.clear();

        clientSnake_.reset();
        playerEntityID_ = 0;
    }

    void GameClient::RemoveEntity(const Utils::Legacy::Game::Net::EntityType type,
                                  const std::uint32_t entityID)
    {
        using namespace Utils::Legacy::Game::Net;

        if (type == EntityType::Snake)
        {
            if (snakesByID_.contains(entityID))
            {
                const auto ptr = snakesByID_[entityID];
                snakesByID_.erase(entityID);

                if (ptr)
                    snakes_.erase(ptr);

                predict_.erase(entityID);
                snakesTTL_.erase(entityID);
                snakeSnapshotCooldownFrame_.erase(entityID);

                if (entityID == playerEntityID_)
                {
                    if (clientSnake_)
                        clientSnake_->Kill(frame_);
                }
            }
        }
        else if (type == EntityType::Food)
        {
            if (foodsByID_.contains(entityID))
            {
                const auto ptr = foodsByID_[entityID];
                foodsByID_.erase(entityID);

                if (ptr)
                    foods_.erase(ptr);

                foodsTTL_.erase(entityID);
            }
        }
    }

    void GameClient::UpsertFood(const std::uint32_t entityID,
                               const Utils::Legacy::Game::Net::FoodState& fs,
                               const bool isNew)
    {
        if (isNew || !foodsByID_.contains(entityID))
        {
            auto f = std::make_shared<EntityFood>(0, sf::Vector2f(fs.x, fs.y));
            f->SetEntityID(entityID);

            foodsByID_[entityID] = f;
            foods_.insert(f);
        }
        else
        {
            auto f = std::make_shared<EntityFood>(0, sf::Vector2f(fs.x, fs.y));
            f->SetEntityID(entityID);

            const auto old = foodsByID_[entityID];
            if (old)
                foods_.erase(old);

            foodsByID_[entityID] = f;
            foods_.insert(f);
        }

        foodsTTL_[entityID].lastSeenSeq = net_.lastServerSeq;
    }

    void GameClient::UpsertSnakeFull(const std::uint32_t entityID,
                                    const Utils::Legacy::Game::Net::SnakeState& ss,
                                    const std::vector<sf::Vector2f>& fullSegments,
                                    const bool isNew)
    {
        // sanity
        constexpr std::uint32_t maxReasonableExp = 5'000'000;
        constexpr std::uint32_t maxReasonableSegments = 60'000;

        if (ss.experience > maxReasonableExp || ss.totalSegments == 0 || ss.totalSegments > maxReasonableSegments)
        {
            badPacketsDropped_++;
            Log()->Warning("[Net] Dropped snake FULL: insane exp/segments. entityID={} exp={} totalSegments={} dropped={} seq={}",
                           entityID, ss.experience, ss.totalSegments, badPacketsDropped_, net_.lastServerSeq);
            net_.pendingFullRequest = true;
            net_.pendingFullRequestAllSegments = true;
            return;
        }

        if (fullSegments.size() != static_cast<std::size_t>(ss.totalSegments))
        {
            badPacketsDropped_++;
            Log()->Warning("[Net] Dropped snake FULL: pointsCount != totalSegments. entityID={} pointsCount={} totalSegments={} dropped={} seq={}",
                           entityID, fullSegments.size(), ss.totalSegments, badPacketsDropped_, net_.lastServerSeq);
            net_.pendingFullRequest = true;
            net_.pendingFullRequestAllSegments = true;
            return;
        }

        EntitySnake::Shared snake;

        if (isNew || !snakesByID_.contains(entityID))
        {
            snake = std::make_shared<EntitySnake>(0, sf::Vector2f(ss.headX, ss.headY));
            snake->SetEntityID(entityID);

            snakesByID_[entityID] = snake;
            snakes_.insert(snake);

            if (entityID == playerEntityID_)
            {
                clientSnake_ = snake;
            }
        }
        else
        {
            snake = snakesByID_[entityID];
        }

        snake->NetApplyExperience(ss.experience);
        snake->NetSetFullSegments(fullSegments);

        snakesTTL_[entityID].lastSeenSeq = net_.lastServerSeq;

        if (entityID == playerEntityID_ && awaitingPlayerRebuild_)
        {
            awaitingPlayerRebuild_ = false;
            Log()->Warning("[Net] Player rebuilt from FULL segments OK. playerID={} segs={}",
                           playerEntityID_, snake->Segments().size());
        }
    }

    void GameClient::QueueSnakeSnapshotRequest(const std::uint32_t entityID)
    {
        // never request snapshot for player: player repair uses FullUpdate(allSegments)
        if (entityID == playerEntityID_)
        {
            ForceFullUpdateRequest();
            return;
        }

        // cooldown to avoid spam
        const std::uint32_t nowFrame = frame_;
        const auto nextAllowed = snakeSnapshotCooldownFrame_.contains(entityID) ? snakeSnapshotCooldownFrame_[entityID] : 0u;
        if (nowFrame < nextAllowed)
        {
            return;
        }

        snakeSnapshotCooldownFrame_[entityID] = nowFrame + 64; // ~1s at 64 logic tick
        pendingSnakeSnapshots_.insert(entityID);

        Log()->Warning("[Net] QueueSnakeSnapshotRequest(entityID={})", entityID);
    }

    void GameClient::ApplySnakeValidationUpdate(const std::uint32_t entityID,
                                               const Utils::Legacy::Game::Net::SnakeState& ss,
                                               const std::vector<sf::Vector2f>& samples,
                                               const bool isNew)
    {
        // if we don't have baseline full segments for this snake -> request snapshot, do NOT build from samples
        if (isNew || !snakesByID_.contains(entityID))
        {
            QueueSnakeSnapshotRequest(entityID);
            return;
        }

        auto snake = snakesByID_[entityID];
        if (!snake)
        {
            QueueSnakeSnapshotRequest(entityID);
            return;
        }

        // apply movement prediction step
        snake->NetSetHead({ ss.headX, ss.headY });
        snake->NetStepBody();
        snake->NetApplyExperience(ss.experience);

        snakesTTL_[entityID].lastSeenSeq = net_.lastServerSeq;

        // IMPORTANT: после ForceFullUpdate(allSegments) не валидируем player пока rebuild не завершён
        if (entityID == playerEntityID_ && awaitingPlayerRebuild_)
        {
            return;
        }

        if (samples.empty())
        {
            return;
        }

        // dynamic thresholds
        const float minDist = snake->GetRadius(false);
        const float base = std::max(120.0f, minDist * 3.0f);
        const float threshold = base;

        if (!ValidateSamplesByRadius(snake->Segments(), samples, minDist, threshold))
        {
            Log()->Warning("[Net] Snake drift validation failed -> request repair. entityID={} sampleCount={} segCount={}",
                           entityID, samples.size(), snake->Segments().size());

            QueueSnakeSnapshotRequest(entityID);
        }
    }

    void GameClient::ApplyFullUpdate(Utils::Legacy::Game::Net::ByteReader& reader)
    {
        using namespace Utils::Legacy::Game::Net;

        ClearWorld();

        FullUpdateHeader fh{};
        if (!ReadFullUpdateHeader(reader, fh))
        {
            badPacketsDropped_++;
            Log()->Warning("[Net] Dropped full update: ReadFullUpdateHeader failed. dropped={} seq={}",
                           badPacketsDropped_, net_.lastServerSeq);
            net_.pendingFullRequest = true;
            net_.pendingFullRequestAllSegments = true;
            return;
        }

        playerEntityID_ = fh.playerEntityID;
        awaitingPlayerRebuild_ = net_.pendingFullRequestAllSegments ? true : awaitingPlayerRebuild_;

        bool playerBuiltExact = false;

        while (!reader.End())
        {
            EntityEntryHeader entry{};
            if (!reader.ReadPod(entry))
            {
                Log()->Warning("[Net] Full update ended early: failed to read EntityEntryHeader");
                break;
            }

            if (entry.type == EntityType::Snake)
            {
                SnakeState ss{};
                if (!reader.ReadPod(ss))
                {
                    badPacketsDropped_++;
                    Log()->Warning("[Net] Dropped full update: failed to read SnakeState. entityID={} dropped={} seq={}",
                                   entry.entityID, badPacketsDropped_, net_.lastServerSeq);
                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    return;
                }

                // FullUpdate must always carry full segments for snakes
                if (ss.totalSegments == 0 || ss.pointsKind != SnakePointsKind::FullSegments)
                {
                    badPacketsDropped_++;
                    Log()->Warning("[Net] Dropped full update: invalid snake kind/segments. entityID={} totalSegments={} kind={} dropped={} seq={}",
                                   entry.entityID, ss.totalSegments, static_cast<std::uint32_t>(ss.pointsKind), badPacketsDropped_, net_.lastServerSeq);
                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    return;
                }

                if (ss.pointsCount != ss.totalSegments)
                {
                    badPacketsDropped_++;
                    Log()->Warning("[Net] Dropped full update: snake pointsCount != totalSegments. entityID={} pointsCount={} totalSegments={} dropped={} seq={}",
                                   entry.entityID, ss.pointsCount, ss.totalSegments, badPacketsDropped_, net_.lastServerSeq);
                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    return;
                }

                const auto segs = ReadSnakePoints(reader, ss.pointsCount);
                if (segs.size() != ss.pointsCount)
                {
                    badPacketsDropped_++;
                    Log()->Warning("[Net] Dropped full update: snake points read mismatch. entityID={} expectedPoints={} gotPoints={} dropped={} seq={}",
                                   entry.entityID, ss.pointsCount, segs.size(), badPacketsDropped_, net_.lastServerSeq);
                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    return;
                }

                UpsertSnakeFull(entry.entityID, ss, segs, true);

                if (entry.entityID == playerEntityID_)
                {
                    playerBuiltExact = true;
                }
            }
            else if (entry.type == EntityType::Food)
            {
                FoodState fs{};
                if (!reader.ReadPod(fs))
                {
                    badPacketsDropped_++;
                    Log()->Warning("[Net] Dropped full update: failed to read FoodState. entityID={} dropped={} seq={}",
                                   entry.entityID, badPacketsDropped_, net_.lastServerSeq);
                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    return;
                }

                auto f = std::make_shared<EntityFood>(0, sf::Vector2f(fs.x, fs.y));
                f->SetEntityID(entry.entityID);

                foodsByID_[entry.entityID] = f;
                foods_.insert(f);

                foodsTTL_[entry.entityID].lastSeenSeq = net_.lastServerSeq;
            }
            else
            {
                badPacketsDropped_++;
                Log()->Warning("[Net] Dropped full update: unknown entity type. type={} entityID={} dropped={} seq={}",
                               static_cast<std::uint32_t>(entry.type), entry.entityID, badPacketsDropped_, net_.lastServerSeq);
                net_.pendingFullRequest = true;
                net_.pendingFullRequestAllSegments = true;
                return;
            }
        }

        if (awaitingPlayerRebuild_)
        {
            if (!playerBuiltExact)
            {
                Log()->Warning("[Net] FullUpdate(allSegments) incomplete: player not built exact -> request again. playerID={}",
                               playerEntityID_);
                net_.pendingFullRequest = true;
                net_.pendingFullRequestAllSegments = true;
            }
            else
            {
                awaitingPlayerRebuild_ = false;
                Log()->Warning("[Net] FullUpdate(allSegments) OK: player rebuilt exact. playerID={} segs={}",
                               playerEntityID_, clientSnake_ ? clientSnake_->Segments().size() : 0);
            }
        }
    }

    void GameClient::ApplySnakeSnapshot(Utils::Legacy::Game::Net::ByteReader& reader)
    {
        using namespace Utils::Legacy::Game::Net;

        // Snapshot payload contains exactly one EntityEntryHeader + SnakeState + points
        EntityEntryHeader entry{};
        if (!reader.ReadPod(entry))
        {
            badPacketsDropped_++;
            Log()->Warning("[Net] Dropped snake snapshot: failed to read EntityEntryHeader. dropped={}", badPacketsDropped_);
            net_.pendingFullRequest = true;
            net_.pendingFullRequestAllSegments = true;
            return;
        }

        if (entry.type != EntityType::Snake)
        {
            badPacketsDropped_++;
            Log()->Warning("[Net] Dropped snake snapshot: entry.type != Snake. type={} entityID={} dropped={}",
                           static_cast<std::uint32_t>(entry.type), entry.entityID, badPacketsDropped_);
            return;
        }

        SnakeState ss{};
        if (!reader.ReadPod(ss))
        {
            badPacketsDropped_++;
            Log()->Warning("[Net] Dropped snake snapshot: failed to read SnakeState. entityID={} dropped={}",
                           entry.entityID, badPacketsDropped_);
            return;
        }

        if (ss.pointsKind != SnakePointsKind::FullSegments || ss.pointsCount != ss.totalSegments || ss.totalSegments == 0)
        {
            badPacketsDropped_++;
            Log()->Warning("[Net] Dropped snake snapshot: invalid kind/count. entityID={} kind={} pointsCount={} totalSegments={} dropped={}",
                           entry.entityID, static_cast<std::uint32_t>(ss.pointsKind), ss.pointsCount, ss.totalSegments, badPacketsDropped_);
            return;
        }

        const auto segs = ReadSnakePoints(reader, ss.pointsCount);
        if (segs.size() != ss.pointsCount)
        {
            badPacketsDropped_++;
            Log()->Warning("[Net] Dropped snake snapshot: points read mismatch. entityID={} expected={} got={} dropped={}",
                           entry.entityID, ss.pointsCount, segs.size(), badPacketsDropped_);
            return;
        }

        UpsertSnakeFull(entry.entityID, ss, segs, true);
    }

    void GameClient::ApplyPartialUpdate(Utils::Legacy::Game::Net::ByteReader& reader)
    {
        using namespace Utils::Legacy::Game::Net;

        while (!reader.End())
        {
            EntityEntryHeader entry{};
            if (!reader.ReadPod(entry))
            {
                Log()->Warning("[Net] Partial update ended early: failed to read EntityEntryHeader");
                break;
            }

            if (HasFlag(entry.flags, EntityFlags::Remove))
            {
                RemoveEntity(entry.type, entry.entityID);
                continue;
            }

            const bool isNew = HasFlag(entry.flags, EntityFlags::New);

            if (entry.type == EntityType::Snake)
            {
                SnakeState ss{};
                if (!reader.ReadPod(ss))
                {
                    badPacketsDropped_++;
                    Log()->Warning("[Net] Dropped partial update: failed to read SnakeState. entityID={} dropped={} seq={}",
                                   entry.entityID, badPacketsDropped_, net_.lastServerSeq);
                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    return;
                }

                if (ss.totalSegments == 0 || ss.pointsCount == 0)
                {
                    badPacketsDropped_++;
                    Log()->Warning("[Net] Dropped partial update: invalid SnakeState fields. entityID={} totalSegments={} pointsCount={} dropped={} seq={}",
                                   entry.entityID, ss.totalSegments, ss.pointsCount, badPacketsDropped_, net_.lastServerSeq);
                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    return;
                }

                // safety
                constexpr std::uint32_t maxReasonableSegments = 60'000;
                if (ss.totalSegments > maxReasonableSegments || ss.pointsCount > maxReasonableSegments)
                {
                    badPacketsDropped_++;
                    Log()->Warning("[Net] Dropped partial update: insane segment counts. entityID={} totalSegments={} pointsCount={} dropped={} seq={}",
                                   entry.entityID, ss.totalSegments, ss.pointsCount, badPacketsDropped_, net_.lastServerSeq);
                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    return;
                }

                const auto points = ReadSnakePoints(reader, ss.pointsCount);
                if (points.size() != ss.pointsCount)
                {
                    badPacketsDropped_++;
                    Log()->Warning("[Net] Dropped partial update: points read mismatch. entityID={} expectedPoints={} gotPoints={} dropped={} seq={}",
                                   entry.entityID, ss.pointsCount, points.size(), badPacketsDropped_, net_.lastServerSeq);
                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    return;
                }

                if (ss.pointsKind == SnakePointsKind::FullSegments)
                {
                    // New snake must always send full segments; also server can resend full occasionally if it wants
                    if (ss.pointsCount != ss.totalSegments)
                    {
                        badPacketsDropped_++;
                        Log()->Warning("[Net] Dropped partial snake FULL: pointsCount != totalSegments. entityID={} pointsCount={} totalSegments={} dropped={} seq={}",
                                       entry.entityID, ss.pointsCount, ss.totalSegments, badPacketsDropped_, net_.lastServerSeq);
                        net_.pendingFullRequest = true;
                        net_.pendingFullRequestAllSegments = true;
                        return;
                    }

                    UpsertSnakeFull(entry.entityID, ss, points, isNew);
                }
                else
                {
                    // Validation samples only (do not build unknown snakes!)
                    ApplySnakeValidationUpdate(entry.entityID, ss, points, isNew);
                }
            }
            else if (entry.type == EntityType::Food)
            {
                FoodState fs{};
                if (!reader.ReadPod(fs))
                {
                    badPacketsDropped_++;
                    Log()->Warning("[Net] Dropped partial update: failed to read FoodState. entityID={} dropped={} seq={}",
                                   entry.entityID, badPacketsDropped_, net_.lastServerSeq);
                    net_.pendingFullRequest = true;
                    net_.pendingFullRequestAllSegments = true;
                    return;
                }

                UpsertFood(entry.entityID, fs, isNew);
            }
            else
            {
                badPacketsDropped_++;
                Log()->Warning("[Net] Dropped partial update: unknown entity type. type={} entityID={} dropped={} seq={}",
                               static_cast<std::uint32_t>(entry.type), entry.entityID, badPacketsDropped_, net_.lastServerSeq);
                net_.pendingFullRequest = true;
                net_.pendingFullRequestAllSegments = true;
                return;
            }
        }
    }

    // ===================== helpers (non-static) =====================

    std::vector<sf::Vector2f> ReadSnakePoints(Utils::Legacy::Game::Net::ByteReader& r,
                                             const std::uint16_t count)
    {
        std::vector<sf::Vector2f> out;
        out.reserve(count);

        for (std::uint16_t i = 0; i < count; ++i)
        {
            sf::Vector2f v{};
            if (!r.ReadVector2f(v))
            {
                break;
            }

            out.push_back(v);
        }

        return out;
    }

    std::vector<sf::Vector2f> BuildExpectedSamplesByRadius(const std::list<sf::Vector2f>& segments,
                                                           const float minDist)
    {
        std::vector<sf::Vector2f> out;

        if (segments.empty())
        {
            return out;
        }

        std::vector<sf::Vector2f> segVec(segments.begin(), segments.end());
        const std::size_t n = segVec.size();

        out.reserve(n);

        sf::Vector2f last = segVec[0];
        out.push_back(last);

        for (std::size_t i = 1; i < n; ++i)
        {
            const auto& p = segVec[i];
            const float dx = p.x - last.x;
            const float dy = p.y - last.y;
            const float dist = std::hypot(dx, dy);

            if (dist >= minDist)
            {
                out.push_back(p);
                last = p;
            }
        }

        // ensure tail included
        if (n >= 2)
        {
            const auto& tail = segVec.back();
            if (out.empty() || (out.back().x != tail.x || out.back().y != tail.y))
            {
                out.push_back(tail);
            }
        }

        return out;
    }

    bool ValidateSamplesByRadius(const std::list<sf::Vector2f>& predicted,
                                 const std::vector<sf::Vector2f>& serverSamples,
                                 const float minDist,
                                 const float threshold)
    {
        if (serverSamples.empty())
        {
            return true;
        }

        const auto expected = BuildExpectedSamplesByRadius(predicted, minDist);

        if (expected.size() != serverSamples.size())
        {
            Utils::Log()->Debug("expected.size() != serverSamples.size() [{} {}]", expected.size(), serverSamples.size());
            return false;
        }

        std::size_t bad = 0;
        const std::size_t n = expected.size();

        const std::size_t allowed = std::max<std::size_t>(2, n / 10);

        for (std::size_t i = 0; i < n; ++i)
        {
            const float dx = expected[i].x - serverSamples[i].x;
            const float dy = expected[i].y - serverSamples[i].y;

            const float dist = std::hypot(dx, dy);

            if (dist > threshold)
            {
                bad++;
                if (bad > allowed)
                {
                    return false;
                }
            }
        }

        return true;
    }

} // namespace Core::App::Game