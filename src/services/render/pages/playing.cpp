#include "playing.hpp"
#include "legacy_game_math.hpp"

#include <cmath>

namespace Core::App::Render::Pages {

    [[maybe_unused]] [[gnu::used]] Utils::Service::Loader::Add<Playing> PlayingPage(PagesLoader());

    void Playing::Initialise()
    {
        Log()->Debug("Initializing Playing page");

        const sf::Vector2f panelSize { 320.f, 420.f };

        // позиция для Block задаётся по ЦЕНТРУ (как у кнопки)
        const sf::Vector2f panelCenter {
            20.f + panelSize.x * 0.5f,
            Height - 20.f - panelSize.y * 0.5f
        };

        ui.debugPanel = UI::Components::Block::Create({
            .position = panelCenter,
            .size = panelSize,
            .enabled = true
        });

        UI::Components::Block::Style normal{};
        normal.background = sf::Color(10, 12, 18, 200);
        normal.borderThickness = 2.f;
        normal.borderColor = sf::Color(255, 255, 255, 20);
        normal.radius = UI::Components::Block::BorderRadius::Uniform(12.f);
        normal.radiusQuality = 12;

        UI::Components::Block::Style hover = normal;
        hover.borderColor = sf::Color(255, 255, 255, 35);

        ui.debugPanel->SetStyles(normal, hover, normal, normal);

        ui.debugText = UI::Components::Text::Create({
            .text = "Debug info",
            .font = "assets/fonts/Roboto-Regular.ttf",
            .characterSize = 12,
            .position = {
                panelCenter.x - panelSize.x * 0.5f + 12.f,
                panelCenter.y - panelSize.y * 0.5f + 10.f
            },
            .hAlign = UI::Components::Text::HAlign::Left,
            .vAlign = UI::Components::Text::VAlign::Top,
            .color = sf::Color(220, 220, 220)
        });
    }

    void Playing::OnAllInterfacesLoaded()
    {
        gameController_ = IFace().Get<GameController>();
    }

    void Playing::UpdateScene()
    {
        auto& window = GetController()->Window();
        if (!window.isOpen())
            return;

        const auto gameClient = gameController_->GetCurrentGameClient();
        if (!gameClient)
            return;

        const auto playerSnake = gameClient->GetPlayerSnake();
        if (!playerSnake)
            return;

        SetZoom(playerSnake->GetZoom());
        SetCameraCenter(playerSnake->GetPosition());

        // ==========================
        // World render (camera view)
        // ==========================
        window.setView(view_);
        DrawGrid(window);

        for (const auto& food: gameClient->GetNearestFoods())
        {
            DrawFood(window, food);
        }

        DrawSnake(window, playerSnake);
        for (const auto& snake: gameClient->GetNearestVictims())
        {
            DrawSnake(window, snake);
        }



        // ==========================
        // UI render (screen space)
        // ==========================
        window.setView(window.getDefaultView());

        // camera center in world coords
        const auto cam = GetCameraCenter();

        // mouse world coords
        const auto mouseWorld = GetMousePosition(window);

        playerSnake->SetDestination(mouseWorld);

        // update debug text
        const auto debug = gameClient->GetDebugInfo();
        auto FormatBytes = [](const std::uint32_t bytes)
        {
            if (bytes < 1024)
                return std::to_string(bytes) + " B";

            if (bytes < 1024 * 1024)
                return std::to_string(bytes / 1024) + " KB";

            return std::to_string(bytes / (1024 * 1024)) + " MB";
        };

        auto Bool = [](const bool v)
        {
            return v ? "true" : "false";
        };

        std::string text;
        text.reserve(1024);

        text += "Camera: " + std::to_string(static_cast<int>(cam.x)) + ", " + std::to_string(static_cast<int>(cam.y)) + "\n";
        text += "Mouse:  " + std::to_string(static_cast<int>(mouseWorld.x)) + ", " + std::to_string(static_cast<int>(mouseWorld.y)) + "\n";

        text += "Zoom:   " + std::to_string(zoom_) + "\n";

        text += "Exp:    " + std::to_string(playerSnake->GetExperience()) + "\n";
        text += "Size:   " + std::to_string(playerSnake->Segments().size()) + "\n";

        text += "\n=== Net ===\n";
        text += "Seq:         " + std::to_string(debug.lastServerSeq) + "\n";
        text += "BadPackets:  " + std::to_string(debug.badPacketsDropped) + "\n";

        text += "PendingFull: " + std::string(Bool(debug.pendingFullRequest)) + "\n";
        text += "AllSegments: " + std::string(Bool(debug.pendingFullRequestAllSegments)) + "\n";
        text += "AwaitRebuild:" + std::string(Bool(debug.awaitingPlayerRebuild)) + "\n";

        text += "\n=== Packets ===\n";
        text += "Full:    " + FormatBytes(debug.lastFullPacketBytes) + " (payload " + FormatBytes(debug.lastFullPayloadBytes) + ")\n";
        text += "Partial: " + FormatBytes(debug.lastPartialPacketBytes) + " (payload " + FormatBytes(debug.lastPartialPayloadBytes) + ")\n";

        text += "\n=== Entities ===\n";
        text += "Foods:   " + std::to_string(debug.foodsCount) + "\n";
        text += "Snakes:  " + std::to_string(debug.snakesCount) + "\n";
        text += "PlayerID:" + std::to_string(debug.playerEntityID) + "\n";

        ui.debugText->SetText(text);


        ui.debugPanel->Update(window);
        ui.debugText->Update();

        for (auto* d : ui.debugPanel->Drawables())
            window.draw(*d);

        for (auto* d : ui.debugText->Drawables())
            window.draw(*d);
    }

    void Playing::HandleEvent(sf::Event &event, sf::RenderWindow &window)
    {
        const auto gameClient = gameController_->GetCurrentGameClient();
        if (!gameClient)
            return;

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R)
        {
            gameClient->ForceFullUpdateRequest();
        }
    }

    void Playing::DrawGrid(sf::RenderWindow & window)
    {
        const float gridSize = 150.f;             // Размер одной клетки
        const float borderThickness = 15.f * zoom_; // Толщина границы

        const sf::Vector2f fieldCenter = Utils::Legacy::Game::AreaCenter; // Центр игрового поля
        const float fieldRadius = Utils::Legacy::Game::AreaRadius;        // Радиус игрового поля

        // Создаём массив линий для сетки
        sf::VertexArray grid(sf::PrimitiveType::Lines);

        // Вычисляем границы сетки на основе игрового поля
        float minX = fieldCenter.x - fieldRadius;
        float maxX = fieldCenter.x + fieldRadius;
        float minY = fieldCenter.y - fieldRadius;
        float maxY = fieldCenter.y + fieldRadius;

        // Округляем границы до кратных размерам сетки
        float startX = std::floor(minX / gridSize) * gridSize;
        float endX = std::ceil(maxX / gridSize) * gridSize;
        float startY = std::floor(minY / gridSize) * gridSize;
        float endY = std::ceil(maxY / gridSize) * gridSize;

        // Вертикальные линии
        for (float x = startX; x <= endX; x += gridSize)
        {
            // Вычисляем пересечения вертикальной линии с кругом
            float deltaX = x - fieldCenter.x;
            if (std::abs(deltaX) > fieldRadius)
                continue; // Линия не пересекает круг

            float dy = std::sqrt(fieldRadius * fieldRadius - deltaX * deltaX);

            float lineStartY = fieldCenter.y - dy;
            float lineEndY = fieldCenter.y + dy;

            grid.append(sf::Vertex(sf::Vector2f(x, lineStartY), sf::Color(100, 100, 100)));
            grid.append(sf::Vertex(sf::Vector2f(x, lineEndY), sf::Color(100, 100, 100)));
        }

        // Горизонтальные линии
        for (float y = startY; y <= endY; y += gridSize)
        {
            // Вычисляем пересечения горизонтальной линии с кругом
            float deltaY = y - fieldCenter.y;
            if (std::abs(deltaY) > fieldRadius)
                continue; // Линия не пересекает круг

            float dx = std::sqrt(fieldRadius * fieldRadius - deltaY * deltaY);

            float lineStartX = fieldCenter.x - dx;
            float lineEndX = fieldCenter.x + dx;

            grid.append(sf::Vertex(sf::Vector2f(lineStartX, y), sf::Color(100, 100, 100)));
            grid.append(sf::Vertex(sf::Vector2f(lineEndX, y), sf::Color(100, 100, 100)));
        }

        // Отрисовка сетки
        window.draw(grid);

        // Отрисовка границы круга с увеличенным количеством точек
        sf::CircleShape circle(fieldRadius, 100); // Устанавливаем 100 точек для плавности
        circle.setOrigin(fieldRadius, fieldRadius);
        circle.setPosition(fieldCenter);
        circle.setFillColor(sf::Color::Transparent);
        circle.setOutlineColor(sf::Color::Red);
        circle.setOutlineThickness(borderThickness);

        window.draw(circle);
    }

    void Playing::DrawSnake(sf::RenderWindow & window, const Utils::Legacy::Game::Interface::Entity::Snake::Shared & snake)
    {
        sf::Color headColor = sf::Color::Cyan;   // Head color
        sf::Color bodyColor = sf::Color::Red;    // Body color

        static float offset = 0.0f;
        const auto& segments = snake->Segments();
        auto headRadius = snake->GetRadius(true);
        auto bodyRadius = snake->GetRadius(false);

        float factor = 1.f;

        // if(snake->IsKilled()) {
        //     auto frames = serverFrame - snake->FrameKilled();
        //
        //     if(frames > SmoothDuration)
        //         return;
        //
        //     factor = ((SmoothDuration - frames) / SmoothDuration);
        // }
        // else if(serverFrame - snake->FrameCreated() < SmoothDuration) {
        //     auto frames = serverFrame - snake->FrameCreated();
        //     factor = frames / SmoothDuration;
        // }

        if(factor != 1.f)
        {
            headRadius *= factor;
            bodyRadius *= factor;

            headColor.a *= factor;
            bodyColor.a *= factor;
        }

        int num = 0;

        for (auto iter = segments.rbegin(); iter != segments.rend(); ++iter)
        {
            sf::Vector2f position = *iter;
            float radius = bodyRadius;
            sf::Color color = bodyColor;

            // If it's the head segment
            if (iter == std::prev(segments.rend()))
            {
                radius = headRadius;
                color = headColor;
            }

            // Create the circle shape for this segment
            sf::CircleShape circle{radius, 50}; // 50 points for smoothness
            circle.setFillColor(color);
            circle.setOrigin(radius, radius);
            circle.setPosition(position);
            circle.setOutlineColor(sf::Color::White);
            circle.setOutlineThickness(2.5f);

            window.draw(circle);

            // // Assign a white texture to generate texture coordinates
            // circle->setTexture(&whiteTexture);
            //
            // // Apply the glow shader
            // // Individual time offset for each segment to vary the glow
            // float timeOffset = offset + num * 0.1f;
            // float time = clock.getElapsedTime().asSeconds() + timeOffset;
            //
            // // Prepare render states with the shader
            // sf::RenderStates states;
            // states.shader = &glowShader;
            // states.blendMode = sf::BlendAlpha;
            //
            // drawable.push_back({.drawable = circle});
            //
            // drawable.push_back({
            //                        .drawable = circle,
            //                        .renderStates = states,
            //                        .baseColor = sf::Glsl::Vec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, 1.f),
            //                        .time = time,
            //                        .pulseSpeed = 5.f
            //                    });

            num++;
        }
    }

    void Playing::DrawFood(sf::RenderWindow & window, const Utils::Legacy::Game::Interface::Entity::Food::Shared & food)
    {
        static float offset = 0.0f;
        auto color = food->GetColor();
        auto radius = food->GetRadius();

        float factor = 1.f;
        // if(food->IsKilled()) {
        //     auto frames = serverFrame - food->FrameKilled();
        //
        //     if(frames > SmoothDuration)
        //         return;
        //
        //     factor = ((SmoothDuration - frames) / SmoothDuration);
        // }
        // else if(serverFrame - food->FrameCreated() < SmoothDuration) {
        //     auto frames = serverFrame - food->FrameCreated();
        //     factor = frames / SmoothDuration;
        // }

        if(factor != 1.f) {
            radius *= factor;
            color.a *= factor;
        }

        // Отрисовка основного круга еды без шейдера
        sf::CircleShape mainBall(radius, 50);
        mainBall.setFillColor(sf::Color{color.r, color.g, color.b, color.a});
        mainBall.setOrigin(radius, radius);
        mainBall.setPosition(food->GetPosition());

        window.draw(mainBall);
        // Добавляем основной круг в список для отрисовки
        // drawable.push_back({mainBall, sf::RenderStates::Default, {}, 0.f, 0.f});
        //
        // // Создаём круг для эффекта свечения
        // auto glowRadius = radius * 3.0f; // Увеличиваем радиус свечения
        // auto glowBall = std::make_shared<sf::CircleShape>(glowRadius, 50);
        // glowBall->setFillColor(sf::Color::White);
        // glowBall->setOrigin(glowRadius, glowRadius);
        // glowBall->setPosition(food->GetPosition());
        // glowBall->setTexture(&whiteTexture);
        //
        // // Индивидуальные параметры для анимации
        // float timeOffset = offset;
        // float time = clock.getElapsedTime().asSeconds() + timeOffset;
        //
        // // Корректируем скорость пульсации для более плавной анимации
        // float pulseSpeed = 1.0f + 0.5f * std::sin(offset);
        //
        // // Подготавливаем состояние отрисовки с шейдером
        // sf::RenderStates states;
        // states.shader = &glowShader;
        // states.blendMode = sf::BlendAdd;
        //
        // // Сохраняем параметры униформ вместе с объектом
        // DrawableItem item = DrawableItem{
        //     .drawable = glowBall,
        //     .renderStates = states,
        //     .baseColor = sf::Glsl::Vec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, 1.f),
        //     .time = time,
        //     .pulseSpeed = pulseSpeed
        // };
        // drawable.push_back(item);

        offset += 0.05f; // Уменьшаем шаг для более плавных изменений
    }

    sf::Vector2f Playing::GetCameraCenter()
    {
        return center_ * zoom_;
    }

    void Playing::SetCameraCenter(const sf::Vector2f & point)
    {
        center_ = point / zoom_;

        view_.setSize(size_ * zoom_);
        view_.setCenter(center_ * zoom_);
    }

    void Playing::SetZoom(float z)
    {
        zoom_ = z;
    }

    sf::Vector2f Playing::GetMousePosition(sf::RenderWindow & window)
    {
        sf::Vector2i mousePosition = sf::Mouse::getPosition(window);

        return (sf::Vector2f(mousePosition.x * 1.f, mousePosition.y * 1.f) + center_ - size_ / 2.f) * zoom_;
    }


} // namespace Core::App::Render::Pages