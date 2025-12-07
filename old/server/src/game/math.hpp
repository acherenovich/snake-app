#pragma once

#include <SFML/System/Vector2.hpp>
#include <list>

namespace Math {
    sf::Vector2f MoveHeadToDestination(std::list<sf::Vector2f> & segments, const sf::Vector2f & dest, float limit = 0.f, float max_turn_angle = 30.f);

    void MoveEverySegmentToTop(std::list<sf::Vector2f> & segments, float limit = 10.f);

    sf::Vector2f GetRandomVector2f(float min, float max);

    bool CheckCollision(const sf::Vector2f & a, const sf::Vector2f & b, float radius = 10.f);

    sf::Vector2f RestrictToBounds(const sf::Vector2f& position, const sf::Vector2f& fieldSize);

    sf::Vector2f CalculateCameraMove(sf::Vector2f center, const sf::Vector2f& head);
}