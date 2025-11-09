#include "packets.hpp"

sf::Packet& operator <<(sf::Packet& packet, const Message::Header& m)
{
    std::string data = {(const char *)m.data.getData(), m.data.getDataSize()};
    return packet << (uint32_t&)m.type << data;
}

sf::Packet& operator >>(sf::Packet& packet, Message::Header& m)
{
    std::string data;
    packet >> (uint32_t&)m.type >> data;
    m.data.append(data.c_str(), data.size());

    return packet;
}

sf::Packet& operator <<(sf::Packet& packet, const Message::Login& m)
{
    return packet << m.name;
}

sf::Packet& operator >>(sf::Packet& packet, Message::Login& m)
{
    return packet >> m.name;
}

sf::Packet& operator <<(sf::Packet& packet, const Message::Move& m)
{
    return packet << m.session << m.destination.x << m.destination.y;
}

sf::Packet& operator >>(sf::Packet& packet, Message::Move& m)
{
    return packet >> m.session >> m.destination.x >> m.destination.y;
}

sf::Packet& operator <<(sf::Packet& packet, const Message::SessionIncoming& m)
{
    return packet << m.session;
}

sf::Packet& operator >>(sf::Packet& packet, Message::SessionIncoming& m)
{
    return packet >> m.session;
}