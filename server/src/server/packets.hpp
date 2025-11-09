#pragma once

#include <SFML/Network/Packet.hpp>
#include <SFML/System/Vector2.hpp>

#include "server.hpp"

sf::Packet& operator <<(sf::Packet& packet, const Message::Header& m);
sf::Packet& operator >>(sf::Packet& packet, Message::Header& m);

sf::Packet& operator <<(sf::Packet& packet, const Message::Login& m);
sf::Packet& operator >>(sf::Packet& packet, Message::Login& m);

sf::Packet& operator <<(sf::Packet& packet, const Message::Move& m);
sf::Packet& operator >>(sf::Packet& packet, Message::Move& m);

sf::Packet& operator <<(sf::Packet& packet, const Message::SessionIncoming& m);
sf::Packet& operator >>(sf::Packet& packet, Message::SessionIncoming& m);