#include "game_client.hpp"

#include <cmath>
#include <vector>

#include "utils.hpp"

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
        }

        // ===================== stale entity cleanup (твоя логика) =====================
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

                it = snakesByID_.erase(it);
                continue;
            }

            ++it;
        }
    }

    void GameClient::OnConnected()
    {
    }

    void GameClient::OnDisconnected()
    {
    }

    void GameClient::OnConnectionError(const std::string & error, const bool /*reconnect*/)
    {
        Log()->Error("Connection error: {}", error);
    }

    void GameClient::OnMessage(const std::vector<std::uint8_t>& data)
    {
        using namespace Utils::Legacy::Game::Net;

        MessageHeader header;
        if (!ParseHeader(std::span(data.data(), data.size()), header))
        {
            badPacketsDropped_++;
            net_.pendingFullRequest = true;
            return;
        }

        // safety: ensure payloadBytes fits buffer
        const std::size_t totalNeed = sizeof(MessageHeader) + static_cast<std::size_t>(header.payloadBytes);
        if (totalNeed > data.size())
        {
            badPacketsDropped_++;
            net_.pendingFullRequest = true;
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

        // sequence check for updates
        if (type == MessageType::FullUpdate || type == MessageType::PartialUpdate)
        {
            if (net_.hasSeq)
            {
                if (header.seq != net_.lastServerSeq + 1)
                {
                    net_.pendingFullRequest = true;
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

            const auto pos = s->GetPosition();
            const float dx = pos.x - playerPos.x;
            const float dy = pos.y - playerPos.y;

            if ((dx * dx + dy * dy) <= radiusSq)
            {
                nearestSnakes.insert(std::static_pointer_cast<Snake>(s));
            }
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
        net_.pendingFullRequestAllSegments = true; // IMPORTANT: player rebuild
        awaitingPlayerRebuild_ = true;
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

                if (entityID == playerEntityID_)
                {
                    clientSnake_.reset();
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

    void GameClient::UpsertSnake(const std::uint32_t entityID,
                                const Utils::Legacy::Game::Net::SnakeState& ss,
                                const std::vector<sf::Vector2f>& samples,
                                const bool isNew)
    {
        // -------- SANITY: protects from corrupted packets --------
        constexpr std::uint32_t maxReasonableExp = 5'000'000;
        constexpr std::uint32_t maxReasonableSegments = 20'000;

        if (ss.experience > maxReasonableExp || ss.totalSegments == 0 || ss.totalSegments > maxReasonableSegments)
        {
            badPacketsDropped_++;
            net_.pendingFullRequest = true;
            return;
        }

        if (samples.size() > static_cast<std::size_t>(ss.totalSegments))
        {
            badPacketsDropped_++;
            net_.pendingFullRequest = true;
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

        snake->NetSetHead({ ss.headX, ss.headY });
        snake->NetStepBody();

        snakesTTL_[entityID].lastSeenSeq = net_.lastServerSeq;

        // IMPORTANT: после ForceFullUpdate(allSegments) не валидируем player пока rebuild не завершён
        if (entityID == playerEntityID_ && awaitingPlayerRebuild_)
        {
            return;
        }

        if (samples.size() >= 2)
        {
            constexpr float driftThreshold = 120.0f;
            if (!ValidateSamples(snake->Segments(), samples, driftThreshold))
            {
                net_.pendingFullRequest = true;
            }
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
            net_.pendingFullRequest = true;
            return;
        }

        playerEntityID_ = fh.playerEntityID;

        // If we requested all segments, we expect player snake to arrive fully in this update.
        // We'll clear awaitingPlayerRebuild_ once we actually build player segments exactly.
        bool playerBuiltExact = false;

        while (!reader.End())
        {
            EntityEntryHeader entry{};
            if (!reader.ReadPod(entry))
            {
                break;
            }

            if (entry.type == EntityType::Snake)
            {
                SnakeState ss{};
                if (!reader.ReadPod(ss))
                {
                    badPacketsDropped_++;
                    net_.pendingFullRequest = true;
                    return;
                }

                // SANITY for full update
                if (ss.totalSegments == 0 || ss.sampleCount > ss.totalSegments)
                {
                    badPacketsDropped_++;
                    net_.pendingFullRequest = true;
                    return;
                }

                const auto samples = ReadSnakeSamples(reader, ss.sampleCount);
                if (samples.size() != ss.sampleCount)
                {
                    badPacketsDropped_++;
                    net_.pendingFullRequest = true;
                    return;
                }

                auto s = std::make_shared<EntitySnake>(0, sf::Vector2f(ss.headX, ss.headY));
                s->SetEntityID(entry.entityID);
                s->NetApplyExperience(ss.experience);

                // Build initial segments (exact when sampleCount == totalSegments)
                std::vector<sf::Vector2f> segs;
                segs.reserve(ss.totalSegments);

                if (!samples.empty())
                {
                    if (samples.size() == static_cast<std::size_t>(ss.totalSegments))
                    {
                        segs = samples;
                    }
                    else
                    {
                        for (std::size_t i = 0; i < ss.totalSegments; ++i)
                        {
                            if (i < samples.size())
                                segs.push_back(samples[i]);
                            else
                                segs.push_back(samples.back());
                        }
                    }
                }
                else
                {
                    for (std::size_t i = 0; i < ss.totalSegments; ++i)
                        segs.emplace_back(ss.headX, ss.headY);
                }

                // SANITY: avoid insane growth from corrupted ss.totalSegments
                if (segs.size() > 20000)
                {
                    badPacketsDropped_++;
                    net_.pendingFullRequest = true;
                    return;
                }

                s->NetSetFullSegments(segs);

                snakesByID_[entry.entityID] = s;
                snakes_.insert(s);

                snakesTTL_[entry.entityID].lastSeenSeq = net_.lastServerSeq;

                if (entry.entityID == playerEntityID_)
                {
                    clientSnake_ = s;

                    if (samples.size() == static_cast<std::size_t>(ss.totalSegments))
                    {
                        playerBuiltExact = true;
                    }
                }
            }
            else if (entry.type == EntityType::Food)
            {
                FoodState fs{};
                if (!reader.ReadPod(fs))
                {
                    badPacketsDropped_++;
                    net_.pendingFullRequest = true;
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
                // unknown type
                badPacketsDropped_++;
                net_.pendingFullRequest = true;
                return;
            }
        }

        if (awaitingPlayerRebuild_)
        {
            // if we requested all segments but didn't get exact player rebuild -> request again
            if (!playerBuiltExact)
            {
                net_.pendingFullRequest = true;
                net_.pendingFullRequestAllSegments = true;
            }
            else
            {
                awaitingPlayerRebuild_ = false;
            }
        }
    }

    void GameClient::ApplyPartialUpdate(Utils::Legacy::Game::Net::ByteReader& reader)
    {
        using namespace Utils::Legacy::Game::Net;

        while (!reader.End())
        {
            EntityEntryHeader entry{};
            if (!reader.ReadPod(entry))
            {
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
                    net_.pendingFullRequest = true;
                    return;
                }

                // SANITY: partial update should never claim insane samples
                if (ss.totalSegments == 0 || ss.sampleCount > ss.totalSegments || ss.sampleCount > 12)
                {
                    badPacketsDropped_++;
                    net_.pendingFullRequest = true;
                    return;
                }

                const auto samples = ReadSnakeSamples(reader, ss.sampleCount);
                if (samples.size() != ss.sampleCount)
                {
                    badPacketsDropped_++;
                    net_.pendingFullRequest = true;
                    return;
                }

                UpsertSnake(entry.entityID, ss, samples, isNew);
            }
            else if (entry.type == EntityType::Food)
            {
                FoodState fs{};
                if (!reader.ReadPod(fs))
                {
                    badPacketsDropped_++;
                    net_.pendingFullRequest = true;
                    return;
                }

                UpsertFood(entry.entityID, fs, isNew);
            }
            else
            {
                badPacketsDropped_++;
                net_.pendingFullRequest = true;
                return;
            }
        }
    }

    // ===================== helpers (non-static) =====================

    std::vector<sf::Vector2f> ReadSnakeSamples(Utils::Legacy::Game::Net::ByteReader& r,
                                               const std::uint8_t count)
    {
        std::vector<sf::Vector2f> out;
        out.reserve(count);

        for (std::uint8_t i = 0; i < count; ++i)
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

    std::vector<sf::Vector2f> BuildExpectedSampleIndices(const std::list<sf::Vector2f>& segments,
                                                         const std::size_t desiredCount)
    {
        std::vector<sf::Vector2f> result;

        const std::size_t n = segments.size();
        if (n == 0 || desiredCount == 0)
        {
            return result;
        }

        const std::size_t count = std::min(desiredCount, n);
        result.reserve(count);

        std::vector<sf::Vector2f> segVec(segments.begin(), segments.end());

        const std::size_t firstCount = std::min<std::size_t>(6, count);
        for (std::size_t i = 0; i < firstCount; ++i)
        {
            result.push_back(segVec[i]);
        }

        if (count <= firstCount)
        {
            return result;
        }

        const std::size_t remaining = count - firstCount;

        const std::size_t start = firstCount;
        const std::size_t end   = n - 1;

        if (start >= n)
        {
            return result;
        }

        const std::size_t range = end - start;

        if (remaining == 1)
        {
            result.push_back(segVec[end]);
            return result;
        }

        for (std::size_t k = 0; k < remaining; ++k)
        {
            const float t = static_cast<float>(k) / static_cast<float>(remaining - 1);
            const std::size_t idx = start + static_cast<std::size_t>(std::round(t * static_cast<float>(range)));

            if (idx < n)
            {
                result.push_back(segVec[idx]);
            }
        }

        return result;
    }

    bool ValidateSamples(const std::list<sf::Vector2f>& predicted,
                         const std::vector<sf::Vector2f>& serverSamples,
                         const float threshold)
    {
        if (serverSamples.empty())
        {
            return true;
        }

        const auto expected = BuildExpectedSampleIndices(predicted, serverSamples.size());

        if (expected.size() != serverSamples.size())
        {
            return false;
        }

        std::size_t bad = 0;

        for (std::size_t i = 0; i < expected.size(); ++i)
        {
            const float dx = expected[i].x - serverSamples[i].x;
            const float dy = expected[i].y - serverSamples[i].y;

            const float dist = std::hypot(dx, dy);

            if (dist > threshold)
            {
                bad++;
            }
        }

        return bad <= 2;
    }

} // namespace Core::App::Game