#pragma once

#include "server.hpp"
#include "session.hpp"

#include "common.hpp"

#include <SFML/Network/TcpListener.hpp>
#include <SFML/Network/TcpSocket.hpp>


class Connection: public Interface::Server {

    const sf::IpAddress ServerIP = sf::IpAddress("0.0.0.0");
    const uint16_t ServerPort = 3100;

    Utils::Event::System<ServerEvents> events;

    std::map<uint64_t, Session::Shared> sessions;
    sf::TcpListener listener;

    Initializer initializer;

    uint32_t frame = 0;
public:
    Connection(Initializer  init);

    void Connect(const std::string & name) override;

    Interface::Session::Shared GetSession(uint64_t id = 0) override;

    void Think();

    Utils::Event::System<ServerEvents> & Events() override { return events; }

    Utils::Log::Shared & Log() { return initializer.log; }
};