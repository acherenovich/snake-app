#include "connection.hpp"
#include "packets.hpp"

#include <utility>

#include "game.hpp"

Connection::Connection(Initializer init) :
    initializer(std::move(init))
{
    initializer.globalEvents.HookEvent(GlobalInitialise) = std::function([this]() {
        initializer.globalEvents.CallEvent(ServerInterfaceLoaded, this);
    });

    auto status = socket.bind(ServerPort, ServerIP);
    if(status == sf::Socket::Status::Error) {
        Log()->Error("Failed to bind address {}", ServerIP.toString());
        std::terminate();
    }

    socket.setBlocking(false);

    initializer.globalEvents.HookEvent(GlobalFrame) = std::function([this]() {
        Think();
    });
}

void Connection::Think()
{
    std::map<Message::Type, std::function<void(sf::Packet &, sf::IpAddress &, uint16_t)>> handlers;

    handlers[Message::Type::NetLogin] = [this](sf::Packet & packet, sf::IpAddress & ip, uint16_t clientPort) {
        Message::Login login;
        packet >> login;

        auto session = Session::CreateSession(login.name, ip, clientPort, socket);
        sessions[session->ID()] = session;

        Message::SessionIncoming sessionIncoming {.session = session->ID()};
        if(session->SendPacket(Message::Type::NetSessionIncoming, sessionIncoming) != sf::Socket::Status::Done) {
            Log()->Error("Could not send session data to user({})", session->ID());
        }
    };

    sf::Packet packet;
    std::optional<sf::IpAddress> address;
    uint16_t clientPort;

    while(true) {
        auto status = socket.receive(packet, address, clientPort);
        if(status != sf::Socket::Status::Done) {
            break;
        }

        if(!address) {
            Log()->Error("Null address");
            continue;
        }

        Message::Header header;
        packet >> header;

        if(!handlers.contains(header.type)) {
            Log()->Error("Unhandled packet incoming: {}", (uint32_t)header.type);
            continue;
        }

        handlers[header.type](header.data, address.value(), clientPort);
    }
}