#pragma once

#include <random>
#include <cstdint>

#include <SFML/Network/TcpSocket.hpp>
#include <utility>
#include "server.hpp"
#include "packets.hpp"

class Session: public Interface::Session {
    uint64_t id;
    std::string name;

    std::shared_ptr<sf::TcpSocket> socket;
    bool disconnected = false;
public:
    using Shared = std::shared_ptr<Session>;

    Session(std::string name, std::shared_ptr<sf::TcpSocket> socket)
    : name(std::move(name)), socket(std::move(socket))
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
    [[nodiscard]] uint64_t ID() const override {
        return id;
    }

    void SetID(uint64_t id_) {
        id = id_;
    }

    template<class T>
    [[nodiscard]] sf::Socket::Status SendPacket(Message::Type type, T message)
    {
        sf::Packet body;
        body << message;

        Message::Header header;
        header.type = type;
        header.data = body;

        ZipPacket packet;
        packet << header;

        return socket->send(packet);
    }

    sf::Socket::Status SendPacket(Message::Type type, sf::Packet & body) override {
        Message::Header header;
        header.type = type;
        header.data = body;

        ZipPacket packet;
        packet << header;

        return socket->send(packet);
    }

    std::shared_ptr<sf::TcpSocket> & Socket() {
        return socket;
    }

    void SetDisconnected(bool d = true)
    {
        disconnected = d;
    }

    bool IsDisconnected()
    {
        return disconnected;
    }

    std::string GetName() const override {
        return name;
    }

    void SetName(const std::string & name_) override {
        name = name_;
    }

    static Shared CreateSession(const std::string& name_, std::shared_ptr<sf::TcpSocket> socket_) {
        return std::make_shared<Session>(name_, std::move(socket_));
    }
};