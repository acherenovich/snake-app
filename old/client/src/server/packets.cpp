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
    return packet << m.session << m.name;
}

sf::Packet& operator >>(sf::Packet& packet, Message::Login& m)
{
    return packet >> m.session >> m.name;
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
    return packet << m.session << m.serverFrame;
}

sf::Packet& operator >>(sf::Packet& packet, Message::SessionIncoming& m)
{
    return packet >> m.session >> m.serverFrame;
}

sf::Packet& operator <<(sf::Packet& packet, const Message::DataUpdate& m)
{
    // Сериализация змей
    packet << static_cast<uint32_t>(m.snakes.size());
    for (const auto& snake : m.snakes)
    {
        packet << snake.session;
        packet << snake.name;
        packet << snake.experience;
        packet << snake.frameCreated << snake.frameKilled;

        // Сериализация сегментов через смещения
        packet << static_cast<uint32_t>(snake.segments.size());
        sf::Vector2f prevPosition(0.f, 0.f); // Начальная позиция
        for (const auto& segment : snake.segments)
        {
            sf::Vector2f delta = segment - prevPosition; // Смещение относительно предыдущего сегмента
            packet << static_cast<int16_t>(delta.x);    // Используем int16_t для экономии места
            packet << static_cast<int16_t>(delta.y);
            prevPosition = segment; // Обновляем текущую позицию
        }
    }

    // Сериализация еды
    packet << static_cast<uint32_t>(m.foods.size());
    for (const auto& food : m.foods)
    {
        uint32_t color = (food.color.r << 24) | (food.color.g << 16) | (food.color.b << 8) | food.color.a;
        packet << color; // Упаковываем цвет в одно 32-битное значение
        packet << food.power;
        packet << static_cast<int16_t>(food.position.x);
        packet << static_cast<int16_t>(food.position.y); // Уменьшаем размер позиции, если координаты в небольшом диапазоне
        packet << food.frameCreated << food.frameKilled;

    }

    packet << m.serverFrame;

    return packet;
}

sf::Packet& operator >>(sf::Packet& packet, Message::DataUpdate& m)
{
    uint32_t snakeCount;
    packet >> snakeCount;

    m.snakes.clear();
    m.snakes.reserve(snakeCount);

    for (uint32_t i = 0; i < snakeCount; ++i)
    {
        Message::DataUpdate::Snake snake;

        packet >> snake.session;
        packet >> snake.name;
        packet >> snake.experience;
        packet >> snake.frameCreated >> snake.frameKilled;

        uint32_t segmentCount;
        packet >> segmentCount;

        snake.segments.clear();
        sf::Vector2f prevPosition(0.f, 0.f); // Начальная позиция
        for (uint32_t j = 0; j < segmentCount; ++j)
        {
            int16_t deltaX, deltaY;
            packet >> deltaX >> deltaY;
            sf::Vector2f segment = prevPosition + sf::Vector2f(deltaX, deltaY);
            snake.segments.push_back(segment);
            prevPosition = segment; // Обновляем текущую позицию
        }

        m.snakes.push_back(snake);
    }

    uint32_t foodCount;
    packet >> foodCount;

    m.foods.clear();
    m.foods.reserve(foodCount);

    for (uint32_t i = 0; i < foodCount; ++i)
    {
        Message::DataUpdate::Food food;

        uint32_t color;
        packet >> color;
        food.color.r = (color >> 24) & 0xFF;
        food.color.g = (color >> 16) & 0xFF;
        food.color.b = (color >> 8) & 0xFF;
        food.color.a = color & 0xFF;

        packet >> food.power;

        int16_t posX, posY;
        packet >> posX >> posY;
        food.position.x = posX;
        food.position.y = posY;

        packet >> food.frameCreated >> food.frameKilled;


        m.foods.push_back(food);
    }

    packet >> m.serverFrame;

    return packet;
}

sf::Packet& operator <<(sf::Packet& packet, const Message::LeaderBoard& m)
{
    uint32_t count = m.leaderboard.size();

    packet << count;

    for(auto & [exp, name]: m.leaderboard)
        packet << exp << name;

    return packet;
}

sf::Packet& operator >>(sf::Packet& packet, Message::LeaderBoard& m)
{
    uint32_t count;

    packet >> count;

    for(int i = 0; i < count; i++) {
        uint32_t exp = 0;
        std::string name;

        packet >> exp >> name;
        m.leaderboard[exp] = name;
    }

    return packet;
}