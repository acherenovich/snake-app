#include "playing.hpp"
#include "legacy_game_math.hpp"

#include <cmath>
#include <filesystem>

static float Len(const sf::Vector2f& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

static sf::Vector2f Normalize(const sf::Vector2f& v)
{
    const float l = Len(v);
    if (l <= 0.00001f) return { 1.f, 0.f };
    return { v.x / l, v.y / l };
}

static sf::Vector2f Perp(const sf::Vector2f& v)
{
    return { -v.y, v.x };
}

static sf::Color MulAlpha(sf::Color c, float k)
{
    k = std::clamp(k, 0.f, 1.f);
    c.a = static_cast<sf::Uint8>(static_cast<float>(c.a) * k);
    return c;
}

static sf::Color AddAlpha(sf::Color c, int add)
{
    int a = static_cast<int>(c.a) + add;
    a = std::clamp(a, 0, 255);
    c.a = static_cast<sf::Uint8>(a);
    return c;
}

namespace Core::App::Render::Pages {

    [[maybe_unused]] [[gnu::used]] Utils::Service::Loader::Add<Playing> PlayingPage(PagesLoader());

    void Playing::Initialise()
    {
        Log()->Debug("Initializing Playing page");

        const sf::Vector2f panelSize { 320.f, 330.f };

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

        std::filesystem::path glowShaderPath = "assets/resources/glow.frag";
        std::filesystem::path snakeShaderPath = "assets/resources/snake.frag";

        if (!glowShader.loadFromFile(glowShaderPath.string(), sf::Shader::Fragment))
        {
            throw std::runtime_error("Failed to load glow shader from " + glowShaderPath.string());
        }

        if (!snakeShader.loadFromFile(snakeShaderPath.string(), sf::Shader::Fragment))
        {
            throw std::runtime_error("Failed to load snake shader from " + snakeShaderPath.string());
        }

        std::filesystem::path blurShaderPath = "assets/resources/liquid_blur.frag";
        if (!blurShader_.loadFromFile(blurShaderPath.string(), sf::Shader::Fragment))
        {
            throw std::runtime_error("Failed to load blur shader from " + blurShaderPath.string());
        }

        if (!whiteTexture.create(1, 1))
        {
            throw std::runtime_error("Failed to create white texture");
        }
        sf::Uint8 pixel[] = {255, 255, 255, 255};
        whiteTexture.update(pixel);
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

        window.clear(sf::Color(6, 7, 10, 255));

        const auto gameClient = gameController_->GetCurrentGameClient();
        if (!gameClient)
            return;

        frame_ = gameClient->GetServerFrame();

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

        if (event.type == sf::Event::KeyPressed && event.key.scancode == sf::Keyboard::Scancode::R)
        {
            gameClient->ForceFullUpdateRequest();
        }

        if (event.type == sf::Event::KeyPressed && event.key.scancode == sf::Keyboard::Scancode::Escape)
        {
            gameController_->ExitToMenu();
        }
    }

    void Playing::DrawGrid(sf::RenderWindow& window)
    {
        const sf::Vector2f fieldCenter = Utils::Legacy::Game::AreaCenter;
        const float fieldRadius = Utils::Legacy::Game::AreaRadius;

        // ==========================
        // Checkerboard floor (dark)
        // ==========================
        const float tile = 220.f;                // размер клетки шахматки (в world units)
        const float fadeEdge = tile * 1.25f;     // допуск на край круга (чтобы не было дыр)

        // видимая область в мире (по текущему view_)
        const sf::Vector2f viewCenter = view_.getCenter();
        const sf::Vector2f viewSize = view_.getSize();
        const float minX = viewCenter.x - viewSize.x * 0.5f;
        const float maxX = viewCenter.x + viewSize.x * 0.5f;
        const float minY = viewCenter.y - viewSize.y * 0.5f;
        const float maxY = viewCenter.y + viewSize.y * 0.5f;

        // клип по bounding box круга, чтобы лишнего не генерить
        const float clipMinX = std::max(minX, fieldCenter.x - fieldRadius - tile);
        const float clipMaxX = std::min(maxX, fieldCenter.x + fieldRadius + tile);
        const float clipMinY = std::max(minY, fieldCenter.y - fieldRadius - tile);
        const float clipMaxY = std::min(maxY, fieldCenter.y + fieldRadius + tile);

        const float startX = std::floor(clipMinX / tile) * tile;
        const float endX   = std::ceil (clipMaxX / tile) * tile;
        const float startY = std::floor(clipMinY / tile) * tile;
        const float endY   = std::ceil (clipMaxY / tile) * tile;

        const sf::Color cA(12, 14, 20, 255);
        const sf::Color cB(16, 18, 26, 255);

        sf::VertexArray quads(sf::Quads);
        quads.clear();

        // мягкие “швы” (очень лёгкие линии)
        sf::VertexArray seams(sf::Lines);
        seams.clear();
        const sf::Color seamCol(255, 255, 255, 8);

        for (float y = startY; y < endY; y += tile)
        {
            for (float x = startX; x < endX; x += tile)
            {
                const sf::Vector2f center { x + tile * 0.5f, y + tile * 0.5f };
                const sf::Vector2f d = center - fieldCenter;

                // грубый клип по кругу (берём тайлы около края тоже)
                if (Len(d) > fieldRadius + fadeEdge)
                    continue;

                const int ix = static_cast<int>(std::floor(x / tile));
                const int iy = static_cast<int>(std::floor(y / tile));
                const bool odd = ((ix + iy) & 1) != 0;

                sf::Color col = odd ? cA : cB;

                // лёгкая виньетка по расстоянию от центра (даёт объём)
                const float dist01 = std::clamp(Len(center - fieldCenter) / fieldRadius, 0.f, 1.f);
                // темнее ближе к краю
                const float dark = 1.f - dist01 * 0.20f;
                col.r = static_cast<sf::Uint8>(static_cast<float>(col.r) * dark);
                col.g = static_cast<sf::Uint8>(static_cast<float>(col.g) * dark);
                col.b = static_cast<sf::Uint8>(static_cast<float>(col.b) * dark);

                // quad
                quads.append(sf::Vertex({ x,         y        }, col));
                quads.append(sf::Vertex({ x + tile,  y        }, col));
                quads.append(sf::Vertex({ x + tile,  y + tile }, col));
                quads.append(sf::Vertex({ x,         y + tile }, col));

                // seams (очень слабые линии, чисто чтобы “сетка” читалась)
                // вертикальная справа
                seams.append(sf::Vertex({ x + tile, y }, seamCol));
                seams.append(sf::Vertex({ x + tile, y + tile }, seamCol));
                // горизонтальная снизу
                seams.append(sf::Vertex({ x, y + tile }, seamCol));
                seams.append(sf::Vertex({ x + tile, y + tile }, seamCol));
            }
        }

        window.draw(quads);
        window.draw(seams);

        // ==========================
        // Mask ring (covers chessboard corners outside the circle)
        // Inner edge = fieldRadius, outer edge = fieldRadius + maskThickness
        // ==========================
        {
            // сколько “наружу” перекрываем (в world units)
            // должно быть >= максимального вылета углов тайла за окружность
            const float maskThickness = tile * 1.30f;

            // радиус задаём так, чтобы внутренняя граница была ровно fieldRadius:
            // inner = R - T/2 => R = fieldRadius + T/2
            const float r = fieldRadius + maskThickness * 0.5f;

            sf::CircleShape mask(r, 220);
            mask.setOrigin(r, r);
            mask.setPosition(fieldCenter);
            mask.setFillColor(sf::Color::Transparent);

            // цвет “вне поля” (сделай как у твоего общего фона, но темнее шахматки)
            mask.setOutlineColor(sf::Color(6, 7, 10, 255));
            mask.setOutlineThickness(maskThickness);

            window.draw(mask);
        }

        // ==========================
        // Border: energy ring + main ring
        // ==========================
        const float baseThickness = 16.f * zoom_; // держим “пиксельный” размер примерно постоянным

        // glow layers (additive)
        for (int i = 0; i < 5; ++i)
        {
            const float t = static_cast<float>(i);
            const float thick = baseThickness + (22.f * zoom_) + t * (18.f * zoom_);
            sf::CircleShape glow(fieldRadius, 160);
            glow.setOrigin(fieldRadius, fieldRadius);
            glow.setPosition(fieldCenter);
            glow.setFillColor(sf::Color::Transparent);
            glow.setOutlineThickness(thick);

            // чуть “малиновый” glow
            sf::Color gc(255, 60, 80, 0);
            gc.a = static_cast<sf::Uint8>(70 - i * 12);
            glow.setOutlineColor(gc);

            sf::RenderStates st;
            st.blendMode = sf::BlendAdd;
            window.draw(glow, st);
        }

        // main ring
        sf::CircleShape circle(fieldRadius, 200);
        circle.setOrigin(fieldRadius, fieldRadius);
        circle.setPosition(fieldCenter);
        circle.setFillColor(sf::Color::Transparent);
        circle.setOutlineColor(sf::Color(255, 80, 90, 230));
        circle.setOutlineThickness(baseThickness);

        window.draw(circle);
    }


    void Playing::DrawSnake(sf::RenderWindow& window,
                        const Utils::Legacy::Game::Interface::Entity::Snake::Shared& snake)
    {
        const auto& segments = snake->Segments();
        const std::size_t segCount = segments.size();
        if (segCount == 0)
            return;

        auto ClampU8 = [](int v) -> sf::Uint8 { return static_cast<sf::Uint8>(std::clamp(v, 0, 255)); };

        auto MulAlphaLocal = [](sf::Color c, float k) -> sf::Color
        {
            k = std::clamp(k, 0.f, 1.f);
            c.a = static_cast<sf::Uint8>(static_cast<float>(c.a) * k);
            return c;
        };

        auto ScaleRGB = [&](sf::Color c, float k) -> sf::Color
        {
            k = std::clamp(k, 0.f, 2.f);
            c.r = ClampU8(static_cast<int>(std::round(c.r * k)));
            c.g = ClampU8(static_cast<int>(std::round(c.g * k)));
            c.b = ClampU8(static_cast<int>(std::round(c.b * k)));
            return c;
        };

        auto Len = [](sf::Vector2f v) -> float { return std::sqrt(v.x*v.x + v.y*v.y); };

        auto NormalizeLocal = [&](sf::Vector2f v) -> sf::Vector2f
        {
            const float l = Len(v);
            if (l < 0.0001f) return {1.f, 0.f};
            return { v.x / l, v.y / l };
        };

        // === sizes + smooth spawn/kill ===
        float headRadius = snake->GetRadius(true);
        float bodyRadius = snake->GetRadius(false);

        float factor = 1.f;
        if (snake->IsKilled())
        {
            const auto frames = frame_ - snake->FrameKilled();
            if (frames > SmoothDuration)
                return;
            factor = (SmoothDuration - frames) / SmoothDuration;
        }
        else if (frame_ - snake->FrameCreated() < SmoothDuration)
        {
            const auto frames = frame_ - snake->FrameCreated();
            factor = frames / SmoothDuration;
        }

        headRadius *= factor;
        bodyRadius *= factor;

        // === adaptive segment rendering (less when bigger) ===
        constexpr int kMaxRender = 220;
        constexpr int kMinRender = 90;

        int target = static_cast<int>(segCount);
        if (segCount > static_cast<std::size_t>(kMaxRender))
        {
            const float k = std::sqrt(static_cast<float>(kMaxRender) / static_cast<float>(segCount));
            target = static_cast<int>(std::round(static_cast<float>(kMaxRender) * k));
        }
        target = std::clamp(target, kMinRender, kMaxRender);

        const std::size_t stride = (segCount <= static_cast<std::size_t>(target))
            ? 1
            : static_cast<std::size_t>((segCount + static_cast<std::size_t>(target) - 1) / static_cast<std::size_t>(target));

        // меньше "дыр" при stride
        const float thicknessBoost = (stride <= 1) ? 1.f : std::min(1.f + 0.10f * static_cast<float>(stride - 1), 2.0f);

        // === HEAD is FRONT() (begin), direction from destination ===
        const sf::Vector2f headPos = *segments.begin();

        sf::Vector2f dir { 1.f, 0.f };
        {
            const sf::Vector2f dest = snake->GetDestination();
            dir = NormalizeLocal(dest - headPos);
        }
        const sf::Vector2f n { -dir.y, dir.x };

        // === darker palette (no overbright), subtle stripes ===
        // base dark “ink-blue” body + slightly greener head
        const sf::Color bodyBase = sf::Color(40, 70, 120, 255);
        const sf::Color headBase = sf::Color(55, 110, 105, 255);

        const float time0 = clock.getElapsedTime().asSeconds();

        // --- helper: draw geometry (soft pass / sharp pass) ---
        auto DrawGeometry = [&](sf::RenderTarget& targetRT, bool softPass)
        {
            std::size_t idx = 0;

            for (auto it = segments.rbegin(); it != segments.rend(); ++it, ++idx)
            {
                const bool isHead = (it == std::prev(segments.rend())); // front() in r-iteration
                const bool forceDraw = isHead || (idx == 0);

                if (!forceDraw && (stride > 1) && (idx % stride != 0))
                    continue;

                const sf::Vector2f pos = *it;

                float r = (isHead ? headRadius : bodyRadius) * thicknessBoost;

                // subtle stripe factor (dark range)
                const float stripe = 0.78f + 0.07f * std::sin(time0 * 1.2f + static_cast<float>(idx) * 0.33f);

                sf::Color col = isHead ? headBase : bodyBase;
                col = ScaleRGB(col, stripe);
                col = MulAlphaLocal(col, factor);

                // soft pass: only filled circles (for blur)
                if (softPass)
                {
                    sf::CircleShape c(r * 1.04f, 32);
                    c.setOrigin(r * 1.04f, r * 1.04f);
                    c.setPosition(pos);
                    // чуть светлее, чтобы блюр читался, но НЕ пересвечивал
                    sf::Color soft = col;
                    soft = ScaleRGB(soft, 1.10f);
                    soft.a = static_cast<sf::Uint8>(std::min(200.f, 140.f * factor + 60.f));
                    c.setFillColor(soft);
                    targetRT.draw(c);
                    continue;
                }

                // sharp pass: аккуратное тело + слабая тень
                // 1) weaker shadow
                {
                    const float sr = r * 1.05f;
                    sf::CircleShape sh(sr, 28);
                    sh.setOrigin(sr, sr);
                    sh.setPosition(pos + sf::Vector2f(r * 0.08f, r * 0.12f));
                    sh.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(std::min(55.f, 25.f + 30.f * factor))));
                    targetRT.draw(sh);
                }

                // 2) base body (darker) + very thin outline
                sf::CircleShape circle(r, 36);
                circle.setOrigin(r, r);
                circle.setPosition(pos);
                circle.setFillColor(col);

                const float outline = std::max(0.8f * zoom_, 0.03f * r);
                circle.setOutlineThickness(outline);
                circle.setOutlineColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(std::min(110.f, 60.f + 50.f * factor))));
                targetRT.draw(circle);

                // 3) tiny highlight (very subtle, no overglow)
                {
                    sf::CircleShape hi(r * 0.42f, 26);
                    hi.setOrigin(r * 0.42f, r * 0.42f);

                    sf::Vector2f hOff = isHead
                        ? (-dir * (r * 0.18f) - n * (r * 0.14f))
                        : sf::Vector2f(-r * 0.14f, -r * 0.14f);

                    hi.setPosition(pos + hOff);

                    sf::Color hc(255, 255, 255, static_cast<sf::Uint8>(std::clamp(10.f + 10.f * factor, 0.f, 22.f)));
                    hi.setFillColor(hc);

                    sf::RenderStates st;
                    st.blendMode = sf::BlendAlpha;
                    targetRT.draw(hi, st);
                }

                // 4) shimmer shader (toned down)
                circle.setTexture(&whiteTexture);

                const float time = time0 + static_cast<float>(idx) * 0.045f;
                snakeShader.setUniform("time", time);

                // baseColor darker + lower alpha so it doesn't blow out
                snakeShader.setUniform("baseColor", sf::Glsl::Vec4(
                    (col.r / 255.f) * 0.85f,
                    (col.g / 255.f) * 0.85f,
                    (col.b / 255.f) * 0.85f,
                    (col.a / 255.f) * 0.65f
                ));

                sf::RenderStates states;
                states.shader = &snakeShader;
                states.blendMode = sf::BlendAlpha;
                targetRT.draw(circle, states);

                // 5) eyes (high contrast, outline)
                if (isHead)
                {
                    const float eyeR = r * 0.18f;
                    const sf::Vector2f eyeBase = pos + dir * (r * 0.18f);

                    const sf::Vector2f e1 = eyeBase + n * (r * 0.28f);
                    const sf::Vector2f e2 = eyeBase - n * (r * 0.28f);

                    // outline ring
                    {
                        sf::CircleShape ring(eyeR * 1.18f, 24);
                        ring.setOrigin(eyeR * 1.18f, eyeR * 1.18f);
                        ring.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(std::min(200.f, 160.f * factor + 40.f))));
                        ring.setPosition(e1); targetRT.draw(ring);
                        ring.setPosition(e2); targetRT.draw(ring);
                    }

                    // sclera
                    sf::CircleShape eye(eyeR, 22);
                    eye.setOrigin(eyeR, eyeR);
                    eye.setFillColor(sf::Color(235, 235, 235, static_cast<sf::Uint8>(std::min(255.f, 220.f * factor + 35.f))));
                    eye.setPosition(e1); targetRT.draw(eye);
                    eye.setPosition(e2); targetRT.draw(eye);

                    // pupil
                    const float pupR = eyeR * 0.42f;
                    sf::CircleShape pup(pupR, 18);
                    pup.setOrigin(pupR, pupR);
                    pup.setFillColor(sf::Color(10, 10, 10, static_cast<sf::Uint8>(std::min(255.f, 230.f * factor + 25.f))));

                    const sf::Vector2f pupOff = dir * (eyeR * 0.75f);
                    pup.setPosition(e1 + pupOff); targetRT.draw(pup);
                    pup.setPosition(e2 + pupOff); targetRT.draw(pup);

                    // spec dot
                    const float dotR = pupR * 0.28f;
                    sf::CircleShape dot(dotR, 12);
                    dot.setOrigin(dotR, dotR);
                    dot.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(std::min(180.f, 120.f * factor + 60.f))));
                    dot.setPosition(e1 + pupOff + sf::Vector2f(-dotR * 0.6f, -dotR * 0.6f)); targetRT.draw(dot);
                    dot.setPosition(e2 + pupOff + sf::Vector2f(-dotR * 0.6f, -dotR * 0.6f)); targetRT.draw(dot);
                }
            }
        };

        // === Ensure blur RTs (half-res for liquid look + performance) ===
        const sf::Vector2u win = window.getSize();
        const sf::Vector2u want {
            std::max(1u, win.x / 2u),
            std::max(1u, win.y / 2u)
        };

        if (blurRTSize_ != want)
        {
            blurRTSize_ = want;
            snakeSoftRT_.create(blurRTSize_.x, blurRTSize_.y);
            blurPingRT_.create(blurRTSize_.x, blurRTSize_.y);
            blurPongRT_.create(blurRTSize_.x, blurRTSize_.y);

            // smooth upscale looks better for “liquid”
            snakeSoftRT_.setSmooth(true);
            blurPingRT_.setSmooth(true);
            blurPongRT_.setSmooth(true);
        }

        // === 1) render soft mass into snakeSoftRT_ (world view) ===
        snakeSoftRT_.setView(view_);
        snakeSoftRT_.clear(sf::Color(0, 0, 0, 0));
        DrawGeometry(snakeSoftRT_, /*softPass=*/true);
        snakeSoftRT_.display();

        // sprite from softRT
        sf::Sprite softSpr(snakeSoftRT_.getTexture());
        softSpr.setPosition(0.f, 0.f);
        softSpr.setScale(
            static_cast<float>(win.x) / static_cast<float>(blurRTSize_.x),
            static_cast<float>(win.y) / static_cast<float>(blurRTSize_.y)
        );

        // === 2) blur pass H -> blurPingRT_ (screen space) ===
        blurPingRT_.setView(blurPingRT_.getDefaultView());
        blurPingRT_.clear(sf::Color(0, 0, 0, 0));
        blurShader_.setUniform("texture", sf::Shader::CurrentTexture);
        blurShader_.setUniform("direction", sf::Glsl::Vec2(1.f / static_cast<float>(blurRTSize_.x), 0.f));

        {
            sf::Sprite s(snakeSoftRT_.getTexture());
            s.setScale(1.f, 1.f);
            sf::RenderStates st;
            st.shader = &blurShader_;
            st.blendMode = sf::BlendAlpha;
            blurPingRT_.draw(s, st);
        }
        blurPingRT_.display();

        // === 3) blur pass V -> blurPongRT_ ===
        blurPongRT_.setView(blurPongRT_.getDefaultView());
        blurPongRT_.clear(sf::Color(0, 0, 0, 0));
        blurShader_.setUniform("texture", sf::Shader::CurrentTexture);
        blurShader_.setUniform("direction", sf::Glsl::Vec2(0.f, 1.f / static_cast<float>(blurRTSize_.y)));

        {
            sf::Sprite s(blurPingRT_.getTexture());
            s.setScale(1.f, 1.f);
            sf::RenderStates st;
            st.shader = &blurShader_;
            st.blendMode = sf::BlendAlpha;
            blurPongRT_.draw(s, st);
        }
        blurPongRT_.display();

        // === 4) draw blurred underlay to window (screen space), then sharp snake (world) ===
        const sf::View oldView = window.getView();

        // blurred underlay
        window.setView(window.getDefaultView());
        {
            sf::Sprite blurSpr(blurPongRT_.getTexture());
            blurSpr.setPosition(0.f, 0.f);
            blurSpr.setScale(
                static_cast<float>(win.x) / static_cast<float>(blurRTSize_.x),
                static_cast<float>(win.y) / static_cast<float>(blurRTSize_.y)
            );

            // two layers: soft alpha + tiny additive (liquid glow, but dark)
            blurSpr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(std::clamp(70.f * factor, 0.f, 90.f))));
            window.draw(blurSpr, sf::RenderStates(sf::BlendAlpha));

            blurSpr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(std::clamp(22.f * factor, 0.f, 35.f))));
            window.draw(blurSpr, sf::RenderStates(sf::BlendAdd));
        }

        // sharp geometry on top
        window.setView(oldView);
        DrawGeometry(window, /*softPass=*/false);
    }

    void Playing::DrawFood(sf::RenderWindow& window,
                       const Utils::Legacy::Game::Interface::Entity::Food::Shared& food)
    {
        auto colorBase = food->GetColor();
        sf::Color color = {colorBase.a, colorBase.r, colorBase.g, colorBase.b};
        float radius = food->GetRadius();

        float factor = 1.f;
        if (food->IsKilled())
        {
            const auto frames = frame_ - food->FrameKilled();
            if (frames > SmoothDuration)
                return;
            factor = (SmoothDuration - frames) / SmoothDuration;
        }
        else if (frame_ - food->FrameCreated() < SmoothDuration)
        {
            const auto frames = frame_ - food->FrameCreated();
            factor = frames / SmoothDuration;
        }

        radius *= factor;
        color = MulAlpha(color, factor);

        const sf::Vector2f pos = food->GetPosition();

        // 1) Shadow
        {
            sf::CircleShape sh(radius * 1.20f, 30);
            sh.setOrigin(radius * 1.20f, radius * 1.20f);
            sh.setPosition(pos + sf::Vector2f(radius * 0.22f, radius * 0.32f));
            sh.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(std::min(140.f, 40.f + 70.f * factor))));
            window.draw(sh);
        }

        // 2) Base ball (слегка темнее по краю через outline)
        sf::CircleShape mainBall(radius, 40);
        mainBall.setOrigin(radius, radius);
        mainBall.setPosition(pos);
        mainBall.setFillColor(color);

        const float outline = std::max(1.2f * zoom_, radius * 0.08f);
        mainBall.setOutlineThickness(outline);
        mainBall.setOutlineColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(std::min(140, 80 + static_cast<int>(outline)))));

        window.draw(mainBall);

        // 3) Highlight
        {
            sf::CircleShape hi(radius * 0.48f, 32);
            hi.setOrigin(radius * 0.48f, radius * 0.48f);
            hi.setPosition(pos + sf::Vector2f(-radius * 0.22f, -radius * 0.24f));
            hi.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(18 + 18 * factor)));
            window.draw(hi);
        }

        // 4) Glow ring (additive shader)
        const float glowRadius = radius * 3.1f;
        sf::CircleShape glowBall(glowRadius, 50);
        glowBall.setOrigin(glowRadius, glowRadius);
        glowBall.setPosition(pos);
        glowBall.setFillColor(sf::Color::White);
        glowBall.setTexture(&whiteTexture);

        const float t = clock.getElapsedTime().asSeconds();
        const float time = t + (pos.x * 0.0007f + pos.y * 0.0005f); // детерминированный сдвиг по позиции
        const float pulseSpeed = 0.9f + 0.35f * std::sin(time * 0.8f);

        glowShader.setUniform("time", time);
        glowShader.setUniform("baseColor", sf::Glsl::Vec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, 1.f));
        glowShader.setUniform("pulseSpeed", pulseSpeed);

        sf::RenderStates states;
        states.shader = &glowShader;
        states.blendMode = sf::BlendAdd;

        window.draw(glowBall, states);
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