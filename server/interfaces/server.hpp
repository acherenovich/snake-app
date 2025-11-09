#pragma once

#include "event_system2.hpp"

#include <SFML/Network/Packet.hpp>
#include <SFML/System/Vector2.hpp>

enum ServerEvents {

};

namespace Message {
    enum Type : uint32_t {
        NetLogin,
        NetMove,
        NetDisconnect,

        NetSessionIncoming,
    };

    struct Header {
        Type type;
        sf::Packet data;
    };

    struct Login {
        std::string name;
    };

    struct Move {
        uint64_t session;

        sf::Vector2f destination;
    };

    struct SessionIncoming {
        uint64_t session;
    };
}

namespace Interface {
    class Server
    {
    public:
        virtual Utils::Event::System<ServerEvents> & Events() = 0;
    };
}