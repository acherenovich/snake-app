#pragma once

#include <random>
#include <cstdint>

#include <SFML/Network/UdpSocket.hpp>
#include <utility>
#include "server.hpp"
#include "packets.hpp"

class Session {
    uint64_t id;
    std::string name;

    sf::IpAddress ip;
    uint16_t port;

    sf::UdpSocket & socket;
public:
    using Shared = std::shared_ptr<Session>;

    Session(std::string name, sf::IpAddress ip, uint16_t port, sf::UdpSocket & socket)
    : name(std::move(name)), ip(ip), port(port), socket(socket)
    {
        id = GetRandomSessionID();
    }

    static uint64_t GetRandomSessionID()
    {
        static std::random_device rd;
        static std::mt19937_64 generator(rd());
        static std::uniform_int_distribution<uint64_t> distribution(0, UINT64_MAX);

        return distribution(generator);
    }
public:
    uint64_t ID() const {
        return id;
    }

    template<class T>
    [[nodiscard]] sf::Socket::Status SendPacket(Message::Type type, T message)
    {
        sf::Packet body;
        body << message;

        Message::Header header;
        header.type = type;
        header.data = body;

        sf::Packet packet;
        packet << header;

        return socket.send(packet, ip, port);
    }

    static Shared CreateSession(const std::string& name_, sf::IpAddress ip_, uint16_t port_, sf::UdpSocket & socket_) {
        return std::make_shared<Session>(name_, ip_, port_, socket_);
    }
};