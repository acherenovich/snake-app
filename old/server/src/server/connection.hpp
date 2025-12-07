#pragma once

#include "server.hpp"
#include "common.hpp"

#include "session.hpp"

#include <SFML/Network/UdpSocket.hpp>

class Connection: public Interface::Server {
    const sf::IpAddress ServerIP = sf::IpAddress::resolve("0.0.0.0").value();
    const uint16_t ServerPort = 3100;

    Utils::Event::System<ServerEvents> events;

    sf::UdpSocket socket;

    std::map<uint64_t, Session::Shared> sessions;

    Initializer initializer;
public:
    Connection(Initializer  init);

    void Think();

    Utils::Event::System<ServerEvents> & Events() override { return events; }

    Utils::Log::Shared & Log() { return initializer.log; }
};