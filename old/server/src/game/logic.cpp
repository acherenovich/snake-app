#include "logic.hpp"

#include <utility>
#include "math.hpp"

Logic::Logic(Initializer init)
    : initializer(std::move(init))
{
    initializer.globalEvents.HookEvent(GlobalInitialise) = std::function([this]() {
        initializer.globalEvents.CallEvent(GameInterfaceLoaded, this);
    });

    initializer.globalEvents.HookEvent(ServerInterfaceLoaded) = std::function([this](Interface::Server * interface) {
        server = interface;
    });

    initializer.globalEvents.HookEvent(GlobalFrame) = std::function([this]() {
        Think();
    });
}

void Logic::Think()
{
    static std::vector<sf::Vector2f> food;
    static std::list<sf::Vector2f> snake;
    if(snake.empty()) {
        for(int i = 0; i < 5; i++) {
            snake.emplace_back(600.f, 50.f * (float)i);
        }
    }
    if(food.empty()) {
        for(int i = 0; i < 5000; i++)
        {
            food.emplace_back(Math::GetRandomVector2f(AreaWidth, AreaHeight));
        }
    }

//    constexpr float step_distance = 50.f; // Расстояние между сегментами змейки
//    constexpr float speed = 10.f;          // Скорость передвижения головы
//
//    auto cursor = graphics->GetMousePosition();
//
//    auto & head = *snake.begin();
//    head = Math::MoveHeadToDestination(snake, cursor, speed, 45.f);
//    head = Math::RestrictToBounds(head, {AreaWidth, AreaHeight});
//
//    Math::MoveEverySegmentToTop(snake, step_distance);
//    graphics->SetCameraCenter(Math::CalculateCameraMove(graphics->GetCameraCenter(), head));
//
//
//    for(auto & point: food) {
//        if(Math::CheckCollision(point, head, Interface::Graphics::SnakeHeadRadius)) {
//            point = Math::GetRandomVector2f(AreaWidth, AreaHeight);
//            snake.insert(std::next(snake.begin()), *std::next(snake.begin()));
//        }
//    }
//
//    graphics->SetZoom(1.f + float(snake.size()) * 0.01f);
//
//    for(auto & point: food) {
//        graphics->AddFood(point);
//    }
//    // Отображаем змейку
//    graphics->AddSnake(snake);
}

//void Logic::Think()
//{
//    static std::list<sf::Vector2f> snake = {{600.f, 30.f}, {600.f, 60.f}, {600.f, 90.f}, {600.f, 120.f}, {600.f, 150.f}, {600.f, 180.f}};
//    constexpr float step_distance = 50.f; // Расстояние между сегментами змейки
//    constexpr float speed = 6.f;          // Скорость передвижения головы
//    static float angle = 0.f;             // Текущий угол движения
//    constexpr float radius = 300.f;        // Радиус окружности
//
//    // Двигаем голову
//    auto head = snake.begin();
//    head->x += speed * cos(angle);
//    head->y += speed * sin(angle);
//    angle += speed / radius; // Увеличиваем угол для окружности
//
//    // Обрабатываем остальные сегменты
//    auto prev = head;
//    for (auto iter = std::next(snake.begin()); iter != snake.end(); ++iter) {
//        sf::Vector2f& current = *iter;
//
//        // Считаем расстояние между текущим сегментом и предыдущим
//        float distance = std::hypot(prev->x - current.x, prev->y - current.y);
//
//        // Если расстояние больше допустимого, двигаем текущий сегмент ближе к предыдущему
//        if (distance > step_distance) {
//            float dx = prev->x - current.x;
//            float dy = prev->y - current.y;
//            float angle_to_prev = std::atan2(dy, dx);
//
//            current.x += (distance - step_distance) * cos(angle_to_prev);
//            current.y += (distance - step_distance) * sin(angle_to_prev);
//        }
//
//        prev = iter; // Текущий сегмент становится предыдущим для следующего
//    }
//
//    // Отображаем змейку
//    graphics->AddSnake(snake);
//}