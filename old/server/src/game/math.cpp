#include "math.hpp"

#include <cmath>
#include <random>

namespace Math {
    sf::Vector2f MoveHeadToDestination(std::list<sf::Vector2f> &segments, const sf::Vector2f &dest, float limit, float max_turn_angle)
    {
        if (segments.size() < 2)
            return {}; // У змейки недостаточно сегментов для вычисления направления

        auto head = *segments.begin();
        auto second = *(++segments.begin()); // Второй сегмент змейки

        // Вычисляем текущее направление змейки
        float current_dx = head.x - second.x;
        float current_dy = head.y - second.y;
        float current_angle = std::atan2(current_dy, current_dx); // Угол в радианах

        // Вычисляем направление к цели
        float target_dx = dest.x - head.x;
        float target_dy = dest.y - head.y;
        float target_angle = std::atan2(target_dy, target_dx); // Угол в радианах

        // Нормализуем разницу углов в диапазон [-π, π]
        float angle_diff = target_angle - current_angle;
        if (angle_diff > M_PI)
            angle_diff -= 2 * M_PI;
        if (angle_diff < -M_PI)
            angle_diff += 2 * M_PI;

        // Ограничиваем разницу углов до допустимого диапазона
        float max_turn_angle_radians = max_turn_angle * M_PI / 180.f;
        if (std::abs(angle_diff) > max_turn_angle_radians)
        {
            // Ограничиваем изменение угла
            angle_diff = (angle_diff > 0 ? 1.f : -1.f) * max_turn_angle_radians;
        }

        // Вычисляем новый угол движения
        float new_angle = current_angle + angle_diff;

        // Рассчитываем направление движения головы
        float dx = std::cos(new_angle);
        float dy = std::sin(new_angle);

//        printf("angle_diff: %.2f | current_angle: %.2f | dx: %.2f | dy: %.2f\n", angle_diff, current_angle, dx, dy);

        // Ограничиваем расстояние движения головы
        head.x += limit * dx;
        head.y += limit * dy;

        return head;
    }

    void MoveEverySegmentToTop(std::list<sf::Vector2f> & segments, float limit)
    {
        auto prev = *segments.begin();
        for (auto iter = std::next(segments.begin()); iter != segments.end(); ++iter) {
            sf::Vector2f& current = *iter;

            // Считаем расстояние между текущим сегментом и предыдущим
            float distance = std::hypot(prev.x - current.x, prev.y - current.y);

            // Если расстояние больше допустимого, двигаем текущий сегмент ближе к предыдущему
            if (distance > limit) {
                float dx = prev.x - current.x;
                float dy = prev.y - current.y;
                float angle_to_prev = std::atan2(dy, dx);

                current.x += (distance - limit) * cos(angle_to_prev);
                current.y += (distance - limit) * sin(angle_to_prev);
            }

            prev = *iter; // Текущий сегмент становится предыдущим для следующего
        }
    }

    sf::Vector2f GetRandomVector2f(float mx, float my)
    {
        static std::random_device rd;
        static std::mt19937       rng(rd());

        auto x = std::uniform_real_distribution<float>(0.f, mx)(rng);
        auto y = std::uniform_real_distribution<float>(0.f, my)(rng);

        return {x, y};
    }

    bool CheckCollision(const sf::Vector2f & a, const sf::Vector2f & b, float radius)
    {
        float distance = std::hypot(a.x - b.x, a.y - b.y);
        return (distance <= radius);
    }

    sf::Vector2f RestrictToBounds(const sf::Vector2f& position, const sf::Vector2f& fieldSize)
    {
        sf::Vector2f restrictedPosition = position;

        // Ограничиваем позицию в пределах игрового поля
        restrictedPosition.x = std::clamp(restrictedPosition.x, 0.f, fieldSize.x);
        restrictedPosition.y = std::clamp(restrictedPosition.y, 0.f, fieldSize.y);

        return restrictedPosition;
    }

    sf::Vector2f CalculateCameraMove(sf::Vector2f center, const sf::Vector2f& head)
    {
        if(center.x > head.x)
            center.x -= center.x - head.x;
        else
            center.x += head.x - center.x;


        if(center.y > head.y)
            center.y -= center.y - head.y;
        else
            center.y += head.y - center.y;

        return center;
    }
}