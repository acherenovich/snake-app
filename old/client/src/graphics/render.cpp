#include "render.hpp"

#include <utility>
#include <ranges>
#include <filesystem>

#include "game.hpp"
#include "game/math.hpp"


#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h>
#include <limits.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#include <limits.h>
#endif

std::string GetExecutablePath()
{
#ifdef _WIN32
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0)
    {
        throw std::runtime_error("GetModuleFileNameA failed");
    }
    return std::string(path);
#elif __linux__
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count == -1)
    {
        throw std::runtime_error("readlink failed");
    }
    return std::string(result, count);
#elif __APPLE__
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0)
    {
        throw std::runtime_error("_NSGetExecutablePath buffer too small");
    }
    return std::string(path);
#else
    throw std::runtime_error("Unsupported platform");
#endif
}

std::string GetExecutableDir()
{
    std::string exePath = GetExecutablePath();
    std::filesystem::path pathObj(exePath);
    return pathObj.parent_path().string();
}

Render::Render(Initializer init)
: initializer(std::move(init))
{
    std::filesystem::path exeDir = GetExecutableDir();
    std::filesystem::path fontPath = exeDir / "resources" / "tuffy.ttf";
    std::filesystem::path glowShaderPath = exeDir / "resources" / "glow.frag";
    std::filesystem::path snakeShaderPath = exeDir / "resources" / "snake.frag";

    if (!font.loadFromFile(fontPath.string()))
    {
        throw std::runtime_error("Failed to load font from " + fontPath.string());
    }

    if (!glowShader.loadFromFile(glowShaderPath.string(), sf::Shader::Fragment))
    {
        throw std::runtime_error("Failed to load glow shader from " + glowShaderPath.string());
    }

    if (!snakeShader.loadFromFile(snakeShaderPath.string(), sf::Shader::Fragment))
    {
        throw std::runtime_error("Failed to load snake shader from " + snakeShaderPath.string());
    }

    if (!whiteTexture.create(1, 1))
    {
        throw std::runtime_error("Failed to create white texture");
    }
    sf::Uint8 pixel[] = {255, 255, 255, 255};
    whiteTexture.update(pixel);

    window.setVerticalSyncEnabled(true);

    window.setView(view);

    initializer.globalEvents.HookEvent(GlobalInitialise) = std::function([this]() {
        initializer.globalEvents.CallEvent(GraphicsInterfaceLoaded, this);
    });

    initializer.globalEvents.HookEvent(GameInterfaceLoaded) = std::function([this](Interface::Game * interface_) {
        game = interface_;
    });

    initializer.globalEvents.HookEvent(GlobalFrame) = std::function([this](uint32_t frame_) {
        frame = frame_;
        UpdateScene();
    });
}

void Render::RenderMenu()
{
    if (game->State() == GameLoading)
    {
        // Эффект загрузки

        // Текст "Loading..."
        auto loadingText = std::make_shared<sf::Text>();
        loadingText->setFont(font);
        loadingText->setString("Loading...");
        loadingText->setCharacterSize(48);
        loadingText->setFillColor(sf::Color::White);
        loadingText->setOutlineColor(sf::Color::Black);
        loadingText->setOutlineThickness(2);
        sf::FloatRect loadingBounds = loadingText->getLocalBounds();
        loadingText->setOrigin(loadingBounds.left + loadingBounds.width / 2.0f,
                               loadingBounds.top + loadingBounds.height / 2.0f);
        loadingText->setPosition(center.x, center.y + 70); // Смещаем текст ниже

        drawable.push_back({loadingText});

        // Вращающийся треугольник
        static sf::CircleShape loadingTriangle(50.f, 3); // Треугольник
        loadingTriangle.setFillColor(sf::Color::White);
        loadingTriangle.setOrigin(50.f, 50.f);
        loadingTriangle.setPosition(center);

        // Анимация вращения
        static sf::Clock rotationClock;
        float rotationSpeed = 100.f; // градусов в секунду
        float deltaTime = rotationClock.restart().asSeconds();
        loadingTriangle.rotate(rotationSpeed * deltaTime);

        auto loadingTriangleDrawable = std::make_shared<sf::CircleShape>(loadingTriangle);
        drawable.push_back({loadingTriangleDrawable});
    }
    else
    {
        // Поле ввода
        inputField.setSize(sf::Vector2f(400, 50));
        inputField.setFillColor(sf::Color::White);
        inputField.setOutlineThickness(2.0f);
        inputField.setOutlineColor(sf::Color::Blue);
        inputField.setOrigin(inputField.getSize() / 2.f);
        inputField.setPosition(center.x, center.y - 25);

        // Кнопка "Play"
        playButton.setSize(sf::Vector2f(200, 50));
        playButton.setFillColor(sf::Color(100, 250, 50));
        playButton.setOrigin(playButton.getSize() / 2.f);
        playButton.setPosition(center.x, center.y + 50);

        // Обновление цвета обводки поля ввода в зависимости от фокуса
        if (isInputFocused)
            inputField.setOutlineColor(sf::Color::Blue);
        else
            inputField.setOutlineColor(sf::Color::Black);

        // Логотип
        auto logoText = std::make_shared<sf::Text>();
        logoText->setFont(font);
        logoText->setString("Snake Game");
        logoText->setCharacterSize(64);
        logoText->setFillColor(sf::Color::White);
        logoText->setOutlineColor(sf::Color::Black);
        logoText->setOutlineThickness(2);
        sf::FloatRect logoBounds = logoText->getLocalBounds();
        logoText->setOrigin(logoBounds.left + logoBounds.width / 2.0f,
                            logoBounds.top + logoBounds.height / 2.0f);
        logoText->setPosition(center.x, center.y - 200);
        drawable.push_back({logoText});

        // Поле ввода
        auto inputFieldDrawable = std::make_shared<sf::RectangleShape>(inputField);
        drawable.push_back({inputFieldDrawable});

        // Текст никнейма
        auto nicknameText = std::make_shared<sf::Text>();
        nicknameText->setFont(font);
        nicknameText->setString(nicknameString + ((clock.getElapsedTime().asMilliseconds() % 1000 < 500 && isInputFocused) ? "_" : ""));
        nicknameText->setCharacterSize(24);
        nicknameText->setFillColor(sf::Color::Black);
        sf::FloatRect nicknameBounds = nicknameText->getLocalBounds();
        nicknameText->setOrigin(0, nicknameBounds.height / 2.0f);
        nicknameText->setPosition(inputField.getPosition().x - inputField.getSize().x / 2.f + 10.f, inputField.getPosition().y);
        drawable.push_back({nicknameText});

        // Кнопка "Play"
        auto playButtonDrawable = std::make_shared<sf::RectangleShape>(playButton);
        drawable.push_back({playButtonDrawable});

        // Текст на кнопке "Play"
        auto playButtonText = std::make_shared<sf::Text>();
        playButtonText->setFont(font);
        playButtonText->setString("Play");
        playButtonText->setCharacterSize(24);
        playButtonText->setFillColor(sf::Color::Black);
        sf::FloatRect playButtonBounds = playButtonText->getLocalBounds();
        playButtonText->setOrigin(playButtonBounds.left + playButtonBounds.width / 2.0f,
                                  playButtonBounds.top + playButtonBounds.height / 2.0f);
        playButtonText->setPosition(playButton.getPosition());
        drawable.push_back({playButtonText});
    }
}

void Render::SetServerFrame(uint32_t frame_)
{
    serverFrame = frame_;
}

void Render::AddSnake(const Interface::Entity::Snake::Shared & snake)
{
    // Colors for the snake
    sf::Color headColor = sf::Color::Cyan;   // Head color
    sf::Color bodyColor = sf::Color::Red;    // Body color

    static float offset = 0.0f;
    const auto& segments = snake->Segments();
    auto headRadius = snake->GetRadius(true);
    auto bodyRadius = snake->GetRadius(false);

    float factor = 1.f;

    if(snake->IsKilled()) {
        auto frames = serverFrame - snake->FrameKilled();

        if(frames > SmoothDuration)
            return;

        factor = ((SmoothDuration - frames) / SmoothDuration);
    }
    else if(serverFrame - snake->FrameCreated() < SmoothDuration) {
        auto frames = serverFrame - snake->FrameCreated();
        factor = frames / SmoothDuration;
    }

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
        auto circle = std::make_shared<sf::CircleShape>(radius, 50); // 50 points for smoothness
        circle->setFillColor(color);
        circle->setOrigin(radius, radius);
        circle->setPosition(position);
//        circle->setOutlineColor(sf::Color::White);
//        circle->setOutlineThickness(2.5f * zoom);

        // Assign a white texture to generate texture coordinates
        circle->setTexture(&whiteTexture);

        // Apply the glow shader
        // Individual time offset for each segment to vary the glow
        float timeOffset = offset + num * 0.1f;
        float time = clock.getElapsedTime().asSeconds() + timeOffset;

        // Prepare render states with the shader
        sf::RenderStates states;
        states.shader = &glowShader;
        states.blendMode = sf::BlendAlpha;

        drawable.push_back({.drawable = circle});

        drawable.push_back({
                               .drawable = circle,
                               .renderStates = states,
                               .baseColor = sf::Glsl::Vec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, 1.f),
                               .time = time,
                               .pulseSpeed = 5.f
                           });

        num++;
    }

    offset += 0.05f;
}

void Render::AddFood(const Interface::Entity::Food::Shared& food)
{
    static float offset = 0.0f;
    auto color = food->GetColor();
    auto radius = food->GetRadius();

    float factor = 1.f;
    if(food->IsKilled()) {
        auto frames = serverFrame - food->FrameKilled();

        if(frames > SmoothDuration)
            return;

        factor = ((SmoothDuration - frames) / SmoothDuration);
    }
    else if(serverFrame - food->FrameCreated() < SmoothDuration) {
        auto frames = serverFrame - food->FrameCreated();
        factor = frames / SmoothDuration;
    }

    if(factor != 1.f) {
        radius *= factor;
        color.a *= factor;
    }

    // Отрисовка основного круга еды без шейдера
    auto mainBall = std::make_shared<sf::CircleShape>(radius, 50);
    mainBall->setFillColor(sf::Color{color.r, color.g, color.b, color.a});
    mainBall->setOrigin(radius, radius);
    mainBall->setPosition(food->GetPosition());

    // Добавляем основной круг в список для отрисовки
    drawable.push_back({mainBall, sf::RenderStates::Default, {}, 0.f, 0.f});

    // Создаём круг для эффекта свечения
    auto glowRadius = radius * 3.0f; // Увеличиваем радиус свечения
    auto glowBall = std::make_shared<sf::CircleShape>(glowRadius, 50);
    glowBall->setFillColor(sf::Color::White);
    glowBall->setOrigin(glowRadius, glowRadius);
    glowBall->setPosition(food->GetPosition());
    glowBall->setTexture(&whiteTexture);

    // Индивидуальные параметры для анимации
    float timeOffset = offset;
    float time = clock.getElapsedTime().asSeconds() + timeOffset;

    // Корректируем скорость пульсации для более плавной анимации
    float pulseSpeed = 1.0f + 0.5f * std::sin(offset);

    // Подготавливаем состояние отрисовки с шейдером
    sf::RenderStates states;
    states.shader = &glowShader;
    states.blendMode = sf::BlendAdd;

    // Сохраняем параметры униформ вместе с объектом
    DrawableItem item = DrawableItem{
        .drawable = glowBall,
        .renderStates = states,
        .baseColor = sf::Glsl::Vec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, 1.f),
        .time = time,
        .pulseSpeed = pulseSpeed
    };
    drawable.push_back(item);

    offset += 0.05f; // Уменьшаем шаг для более плавных изменений
}

void Render::AddDebug(const Interface::Entity::Snake::Shared & snake)
{
    // Получаем текущий центр и размер камеры
    sf::Vector2f cameraCenter = view.getCenter();
    sf::Vector2f cameraSize = view.getSize();

    // Позиция текста в левом верхнем углу относительно камеры
    sf::Vector2f textPosition = cameraCenter - (cameraSize / 2.f) + sf::Vector2f(10.f, 10.f);

    // Форматируем текст
    auto text = std::make_shared<sf::Text>();
    text->setFont(font);
    text->setCharacterSize(25 * zoom);
    text->setFillColor(sf::Color::White);
    text->setOutlineColor(sf::Color::Black);
    text->setOutlineThickness(1);
    text->setPosition(textPosition); // Позиция относительно камеры

    auto position = snake->GetPosition();
    auto exp = snake->GetExperience();
    // Устанавливаем текст координат
    std::ostringstream oss;
    oss << "X: " << position.x << ", Y: " << position.y << "\nExp: " << exp;
    text->setString(oss.str());

    // Добавляем текст в список объектов для отрисовки
    drawable.push_back({text});
}

void Render::AddLeaderboard(const std::map<uint32_t, std::string> & scores)
{
// Параметры лидерборда
    const float padding = 15.f * zoom;  // Отступы
    const float maxWidth = 250.f * zoom;       // Максимальная ширина текстового блока
    const size_t maxNameLength = 13;    // Максимальная длина имени
    const float lineHeight = 25.f * zoom;     // Высота строки
    const sf::Color bgColor(0, 0, 200, 150); // Цвет фона (чёрный с прозрачностью)
    const sf::Color textColor(255, 255, 255); // Цвет текста (белый)

    // Получаем текущий центр и размер камеры
    sf::Vector2f cameraCenter = view.getCenter();
    sf::Vector2f cameraSize = view.getSize();

    // Позиция фона относительно правого верхнего угла камеры
    sf::Vector2f backgroundPosition = cameraCenter + sf::Vector2f(cameraSize.x / 2.f - maxWidth - padding, -cameraSize.y / 2.f + padding);

    // Размер фона
    sf::Vector2f backgroundSize(maxWidth, lineHeight * (scores.size() + 1) + padding * 2.f);

    // Фон лидерборда
    auto background = std::make_shared<sf::RectangleShape>();
    background->setPosition(backgroundPosition);
    background->setSize(backgroundSize);
    background->setFillColor(bgColor);
    background->setOutlineColor(sf::Color::White);
    background->setOutlineThickness(2.f * zoom);

    drawable.push_back({background});

    // Текст для заголовка
    auto title = std::make_shared<sf::Text>();
    title->setFont(font);
    title->setCharacterSize(25 * zoom);
    title->setFillColor(textColor);
    title->setString("Leaderboard");
    title->setPosition(backgroundPosition + sf::Vector2f(padding, padding));
    drawable.push_back({title});

    // Текст для игроков
    float yOffset = padding + lineHeight; // Смещение для первой строки игроков
    for (auto [score, displayName] : scores | std::views::reverse)
    {
        // Обрезаем имя, если оно слишком длинное
        if (displayName.size() > maxNameLength)
        {
            displayName = displayName.substr(0, maxNameLength - 3) + "...";
        }

        std::ostringstream oss;
        oss << displayName << " - " << score;

        auto playerText = std::make_shared<sf::Text>();
        playerText->setFont(font);
        playerText->setCharacterSize(22 * zoom);
        playerText->setFillColor(textColor);
        playerText->setString(oss.str());
        playerText->setPosition(backgroundPosition + sf::Vector2f(padding, yOffset));

        drawable.push_back({playerText});

        yOffset += lineHeight; // Смещение для следующего игрока
    }
}

sf::Vector2f Render::GetCameraCenter()
{
    return center * zoom;
}

void Render::SetCameraCenter(const sf::Vector2f & point)
{
    center = point / zoom;

    view.setSize(size * zoom);
    view.setCenter(center * zoom);

    window.setView(view);
}

void Render::SetZoom(float z)
{
    zoom = z;
}

sf::Vector2f Render::GetMousePosition()
{
    sf::Vector2i mousePosition = sf::Mouse::getPosition(window);

    return (sf::Vector2f(mousePosition.x * 1.f, mousePosition.y * 1.f) + center - size / 2.f) * zoom;
}

void Render::UpdateScene()
{
    if (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            float cameraSpeed = 10.f; // Скорость движения камеры (пиксели/секунду)

            // Обработка событий
            switch (event.type)
            {
                case sf::Event::Closed:
                    window.close();
                    Log()->Debug("Close window");
                    initializer.globalEvents.CallEvent(CallStop);
                    break;

                case sf::Event::KeyPressed:
                    if (event.key.code == sf::Keyboard::Escape)
                    {
                        window.close();
                        Log()->Debug("Close window");
                        initializer.globalEvents.CallEvent(CallStop);
                    }
                    else if (event.key.code == sf::Keyboard::Space)
                    {
                        // Добавьте логику для обработки Space
                    }
                    break;
                case sf::Event::TextEntered:
                    if (game->State() == GameMenu && isInputFocused)
                    {
                        if (event.text.unicode < 128)
                        {
                            if (event.text.unicode == 8) // Backspace
                            {
                                if (!nicknameString.empty())
                                    nicknameString.pop_back();
                            }
                            else if (event.text.unicode >= 32 && event.text.unicode <= 126) // Печатаемые символы
                            {
                                if (nicknameString.length() < 20)
                                    nicknameString += static_cast<char>(event.text.unicode);
                            }
                            else if (event.text.unicode == 13) // Enter
                            {
                                game->StartGame(nicknameString);
                            }
                        }
                    }
                    break;
                case sf::Event::MouseButtonPressed:
                    if (game->State() == GameMenu && event.mouseButton.button == sf::Mouse::Left)
                    {
                        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

                        if (inputField.getGlobalBounds().contains(mousePos))
                        {
                            isInputFocused = true;
                        }
                        else if (playButton.getGlobalBounds().contains(mousePos))
                        {
                            game->StartGame(nicknameString);
                        }
                        else
                        {
                            isInputFocused = false;
                        }
                    }
                    break;

                default:
                    break;
            }
        }

        // Отрисовка
        window.clear(sf::Color(27, 24, 82)); // Фон
        DrawGrid();

        for (auto& item : drawable)
        {
            if (item.renderStates.shader)
            {
                if (item.renderStates.shader == &glowShader)
                {
                    glowShader.setUniform("baseColor", item.baseColor);
                    glowShader.setUniform("time", item.time);
                    glowShader.setUniform("pulseSpeed", item.pulseSpeed);
                }
                else if (item.renderStates.shader == &snakeShader)
                {
                    float time = clock.getElapsedTime().asSeconds();
                    snakeShader.setUniform("time", time);
                    snakeShader.setUniform("baseColor", item.baseColor);
                }
            }

            window.draw(*item.drawable, item.renderStates);
        }

        window.display();
    }

    drawable.clear();
}

void Render::DrawGrid()
{
    const float gridSize = 150.f;             // Размер одной клетки
    const float borderThickness = 15.f * zoom; // Толщина границы

    const sf::Vector2f fieldCenter = Interface::Game::AreaCenter; // Центр игрового поля
    const float fieldRadius = Interface::Game::AreaRadius;        // Радиус игрового поля

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