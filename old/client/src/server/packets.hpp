#pragma once

#include <SFML/Network/Packet.hpp>
#include <SFML/System/Vector2.hpp>
#include <zlib.h>
#include <vector>
#include <stdexcept>

#include "server.hpp"

class ZipPacket : public sf::Packet
{
private:
    static constexpr std::size_t MaxPacketSize = 65 * 1024; // 65 кБ

    std::array<uint8_t, MaxPacketSize> compressedBuffer;   // Буфер для сжатых данных
    std::array<uint8_t, MaxPacketSize> uncompressedBuffer; // Буфер для разархивированных данных

public:
    const void* onSend(std::size_t& size) override
    {
        const void* srcData = getData();
        std::size_t srcSize = getDataSize();

        if (srcSize > MaxPacketSize)
        {
            throw std::runtime_error("Packet size exceeds maximum allowed size (65kB)");
        }

        uLongf compressedSize = compressedBuffer.size();

        // Выполняем сжатие с минимальным уровнем компрессии для ускорения
        int result = compress2(
            compressedBuffer.data(), &compressedSize,
            static_cast<const Bytef*>(srcData), srcSize,
            Z_BEST_SPEED);

        if (result != Z_OK)
        {
            throw std::runtime_error("Failed to compress data: " + std::to_string(result));
        }

        size = compressedSize;
        return compressedBuffer.data();
    }

    void onReceive(const void* data, std::size_t size) override
    {
        if (size > MaxPacketSize)
        {
            throw std::runtime_error("Received packet size exceeds maximum allowed size (65kB)");
        }

        uLongf uncompressedSize = uncompressedBuffer.size();

        // Выполняем разархивирование
        int result = uncompress(
            uncompressedBuffer.data(), &uncompressedSize,
            static_cast<const Bytef*>(data), size);

        if (result != Z_OK)
        {
            throw std::runtime_error("Failed to decompress data: " + std::to_string(result));
        }

        append(uncompressedBuffer.data(), uncompressedSize);
    }
};

sf::Packet& operator <<(sf::Packet& packet, const Message::Header& m);
sf::Packet& operator >>(sf::Packet& packet, Message::Header& m);

sf::Packet& operator <<(sf::Packet& packet, const Message::Login& m);
sf::Packet& operator >>(sf::Packet& packet, Message::Login& m);

sf::Packet& operator <<(sf::Packet& packet, const Message::Move& m);
sf::Packet& operator >>(sf::Packet& packet, Message::Move& m);

sf::Packet& operator <<(sf::Packet& packet, const Message::SessionIncoming& m);
sf::Packet& operator >>(sf::Packet& packet, Message::SessionIncoming& m);

sf::Packet& operator <<(sf::Packet& packet, const Message::DataUpdate& m);
sf::Packet& operator >>(sf::Packet& packet, Message::DataUpdate& m);

sf::Packet& operator <<(sf::Packet& packet, const Message::LeaderBoard& m);
sf::Packet& operator >>(sf::Packet& packet, Message::LeaderBoard& m);
