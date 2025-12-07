#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "graphics.hpp"

#include "common.hpp"

class Render: public Interface::Graphics {
    Utils::Event::System<GraphicsEvents> events;

    Initializer initializer;

    Interface::Game * game = {};

    uint32_t frame, serverFrame = 0;

    sf::Clock clock;
    sf::Font font;
    sf::Shader glowShader, snakeShader;
    sf::Texture whiteTexture;
    sf::RenderWindow window = {sf::VideoMode(static_cast<unsigned int>(Width), static_cast<unsigned int>(Height), 64), "Snake", sf::Style::Titlebar | sf::Style::Close};

    const sf::Vector2f size = {Width, Height};
    sf::Vector2f center = {Width / 2.f, Height / 2.f};
    sf::View view = {center, size};

    std::string nicknameString;
    bool isInputFocused;
    sf::RectangleShape inputField, playButton;

    struct DrawableItem
    {
        std::shared_ptr<sf::Drawable> drawable;
        sf::RenderStates renderStates;
        sf::Glsl::Vec4 baseColor;
        float time;
        float pulseSpeed;
    };
    std::vector<DrawableItem> drawable;

    float zoom = 1.0;
public:
    Render(Initializer  init);

    void RenderMenu() override;

    void SetServerFrame(uint32_t frame_) override;

    void AddSnake(const Interface::Entity::Snake::Shared & snake) override;

    void AddFood(const Interface::Entity::Food::Shared & food) override;

    void AddDebug(const Interface::Entity::Snake::Shared & snake) override;

    void AddLeaderboard(const std::map<uint32_t, std::string> & scores) override;

    sf::Vector2f GetCameraCenter() override;

    void SetCameraCenter(const sf::Vector2f & point) override;

    void SetZoom(float z) override;

    sf::Vector2f GetMousePosition() override;
private:
    void UpdateScene();

    void DrawGrid();
public:

    Utils::Event::System<GraphicsEvents> & Events() override { return events; }

    Utils::Log::Shared & Log() { return initializer.log; }
};
