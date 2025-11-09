#pragma once

#include <list>

#include "event_system2.hpp"

#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Network/Socket.hpp>

enum ServerEvents {
    SessionConnected,
    MoveRequest,
    DataUpdate,
    LeaderboardUpdate,
};

namespace Message {
    enum Type : uint32_t {
        NetLogin,
        NetMove,
        NetDisconnect,

        NetSessionIncoming,
        NetDataUpdate,

        NetLeaderboard,
    };

    struct Header {
        Type type;
        sf::Packet data;
    };

    struct Login {
        uint64_t session;
        std::string name;
    };

    struct Move {
        uint64_t session;

        sf::Vector2f destination;
    };

    struct SessionIncoming {
        uint64_t session;
        uint32_t serverFrame;
    };

    struct DataUpdate {
        struct Color {
            uint8_t r = 255;
            uint8_t g = 255;
            uint8_t b = 255;
            uint8_t a = 255;
        };

        struct Snake {
            uint64_t session;
            std::string name;
            uint32_t experience;
            std::list<sf::Vector2f> segments;
            uint32_t frameCreated, frameKilled;
        };

        struct Food {
            Color color;
            uint8_t power = 1;
            sf::Vector2f position;
            uint32_t frameCreated, frameKilled;
        };

        std::vector<Snake> snakes;
        std::vector<Food> foods;

        uint32_t serverFrame;
    };

    struct LeaderBoard {
        std::map<uint32_t, std::string> leaderboard;
    };
}

namespace Interface {
    class Session {
    public:
        using Shared = std::shared_ptr<Session>;

        [[nodiscard]] virtual uint64_t ID() const = 0;

        virtual sf::Socket::Status SendPacket(Message::Type type, sf::Packet & body) = 0;

        virtual std::string GetName() const = 0;

        virtual void SetName(const std::string & name_) = 0;
    };

    class Server
    {
    public:
        virtual Utils::Event::System<ServerEvents> & Events() = 0;

        virtual void Connect(const std::string & name) = 0;

        virtual Session::Shared GetSession(uint64_t id = 0) = 0;
    };
}