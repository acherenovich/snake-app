#include "logic.hpp"
#include "server/packets.hpp"

#include <utility>
#include <set>
#include "math.hpp"

const sf::Vector2f Interface::Game::AreaCenter = sf::Vector2f(Interface::Game::AreaRadius, Interface::Game::AreaRadius);

Logic::Logic(Initializer init)
    : initializer(std::move(init))
{
    initializer.globalEvents.HookEvent(GlobalInitialise) = std::function([this]() {
        initializer.globalEvents.CallEvent(GameInterfaceLoaded, this);
    });

    initializer.globalEvents.HookEvent(ServerInterfaceLoaded) = std::function([this](Interface::Server * interface_) {
        server = interface_;
    });


    initializer.globalEvents.HookEvent(GraphicsInterfaceLoaded) = std::function([this](Interface::Graphics * interface_) {
        graphics = interface_;
    });

    initializer.globalEvents.HookEvent(GlobalInitialisePost) = std::function([this]() {
        OnAllLoaded();
    });
}

void Logic::OnAllLoaded()
{
    if(!server)
    {
        Log()->Error("Failed to initialise interfaces!");
        return;
    }

    initializer.globalEvents.HookEvent(GlobalFrame) = std::function([this](uint32_t frame_) {
        frame = frame_;
        Think();
    });

    server->Events().HookEvent(SessionConnected) = std::function([this](const Interface::Session::Shared & session){
        Log()->Debug("Session({}) successfully connected", session->ID());
#ifdef BUILD_CLIENT
        gameState = GameState::GameLoading;
#else
        AddSnake(session);
        SendFullSessionUpdate(session);
#endif
    });

    server->Events().HookEvent(DataUpdate) = std::function([this](const Interface::Session::Shared & session, Message::DataUpdate & update){
        if(gameState == GameState::GameLoading) {
            gameState = GameState::GamePlaying;
            Log()->Msg("Start playing!");
        }

        ReceiveFullSessionUpdate(session, update);
    });

    server->Events().HookEvent(LeaderboardUpdate) = std::function([this](Message::LeaderBoard & update){
        leaderboard = update.leaderboard;
    });


#ifdef BUILD_CLIENT
    if(!graphics)
    {
        Log()->Error("Failed to initialise interfaces!");
        return;
    }

//    server->Connect("player1");
#else
    server->Events().HookEvent(MoveRequest) = std::function([this](const Interface::Session::Shared & session, sf::Vector2f dest){
        SetDestination(session, dest);
        SendFullSessionUpdate(session);
    });
#endif
}

void Logic::Think()
{
#ifdef BUILD_CLIENT
    if(gameState == GameState::GameMenu || gameState == GameState::GameLoading) {
        graphics->RenderMenu();

        return;
    }


    if(gameState != GameState::GamePlaying)
        return;

    auto session = server->GetSession();
    if(!session) {
        Log()->Error("Think(): Session does not exits");
        return;
    }

    auto sessionID = session->ID();

    if(!snakes.contains(sessionID)) {
        Log()->Error("Think(): Snake does not exits");
        return;
    }

    auto snake = snakes[sessionID];

    graphics->SetCameraCenter(Math::CalculateCameraMove(graphics->GetCameraCenter(), snake->GetPosition()));

    auto cursor = graphics->GetMousePosition();

    SendMoveDestination(session, cursor);

    if(!snake->IsKilled())
        graphics->SetZoom(snake->GetZoom());


    for(auto & food: foods) {
        graphics->AddFood(food);
    }

    for(auto & [targetSession, targetSnake]: snakes)
    {
        graphics->AddSnake(targetSnake);
    }

    graphics->AddLeaderboard(leaderboard);

    graphics->AddDebug(snake);
#else
    GenerateLeaderboard();
    GenerateFoods();

    std::set<Entity::Snake::Shared> killList;

    for(auto & [targetSession, targetSnake]: snakes)
    {
        if(targetSnake->IsKilled())
        {
            if(targetSnake->CanRespawn(frame))
            {
                auto session = server->GetSession(targetSession);
                if(session)
                    AddSnake(session);
            }

            continue;
        }

        if(!MoveSnake(targetSnake))
            killList.insert(targetSnake);
    }

    CheckCollision(killList);
    KillSnakes(killList);

#endif
}

void Logic::GenerateLeaderboard()
{
    if(frame % 64)
        return;

    leaderboard.clear();

    for(auto & [_, snake]: snakes)
    {
        leaderboard[snake->GetExperience()] = snake->GetName();
    }

    // Оставляем только топ-5 игроков
    if(leaderboard.size() > 5)
    {
        auto it = leaderboard.end();
        std::advance(it, -5);

        // Удаляем все элементы до этого итератора
        leaderboard.erase(leaderboard.begin(), it);
    }

    Message::LeaderBoard data = {.leaderboard = leaderboard};

    sf::Packet packet;
    packet << data;

    for(auto & [sessionID, _]: snakes)
    {
        auto session = server->GetSession(sessionID);
        if(session)
        {
            session->SendPacket(Message::NetLeaderboard, packet);
        }
    }
}

void Logic::GenerateFoods()
{
    for(auto i = foods.size(); i < FoodCount; i++)
    {
        foods.emplace_back(std::make_shared<Entity::Food>(frame, Math::GetRandomVector2fInSphere(AreaCenter, AreaRadius - 10.f)));
    }
}

void Logic::CheckCollision(std::set<Entity::Snake::Shared> & killList)
{
    for (auto& [session, snake] : snakes)
    {
        if(killList.contains(snake) || snake->IsKilled())
            continue;

        auto check = [&](const std::shared_ptr<Entity::Food>& food)
        {
            if(food->IsKilled()) {
                if(frame - food->FrameKilled() > 64)
                    return true;

                return false;
            }

            if (Math::CheckCollision(food->GetPosition(), snake->GetPosition(), snake->GetRadius(true) + food->GetRadius()))
            {
                snake->AddExperience(food->GetPower());
                food->Kill(frame);
            }
            return false;
        };

        foods.erase(std::remove_if(foods.begin(), foods.end(), check), foods.end());

        // Check collision with other snakes
        for(auto& [_, target] : snakes) {
            if(snake == target || killList.contains(target) || target->IsKilled())
                continue;

            if (Math::CheckCollision(snake->GetPosition(), target->GetPosition(), snake->GetRadius(true) + target->GetRadius(true)))
            {
                if(target->GetExperience() >= snake->GetExperience()) {
                    killList.insert(snake);
                    break;
                }
            }

            for(auto & part: target->Segments())
            {
                if (Math::CheckCollision(snake->GetPosition(), part, snake->GetRadius(true) + target->GetRadius(false)))
                {
                    killList.insert(snake);
                    break;
                }
            }

            if(killList.contains(snake))
                break;
        }
    }
}

void Logic::KillSnakes(const std::set<Entity::Snake::Shared>& list)
{
    for (auto& snake : list)
    {
        auto & segments = snake->Segments();

        // Преобразуем список сегментов в вектор для удобного доступа по индексу
        std::vector<sf::Vector2f> segmentVector(segments.begin(), segments.end());

        int numFoods = snake->GetExperience() / 3;
        int numSegments = static_cast<int>(segmentVector.size());

        for (int i = 0; i < numFoods; i++)
        {
            // Выбираем случайный индекс сегмента
            int randomIndex = Math::GetRandomInt(0, numSegments - 1);

            // Получаем позицию выбранного сегмента
            sf::Vector2f segmentPosition = segmentVector[randomIndex];

            // Генерируем случайную позицию в небольшом радиусе вокруг сегмента
            float spawnRadius = randomIndex == 0 ? snake->GetRadius(true) : snake->GetRadius(false);
            sf::Vector2f foodPosition = Math::GetRandomVector2fInSphere(segmentPosition, spawnRadius);

            // Создаём новый объект еды в сгенерированной позиции
            foods.emplace_back(std::make_shared<Entity::Food>(frame, foodPosition, 3));
        }
    }

    for(auto & [sessionID, snake]: snakes)
    {
        if (list.contains(snake))
        {
            snake->Kill(frame);
        }
    }
}

Entity::Snake::Shared Logic::AddSnake(const Interface::Session::Shared & session)
{
    auto sessionID = session->ID();

    auto start = Math::GetRandomVector2fInSphere(AreaCenter, AreaRadius - 10.f);

    auto snake = std::make_shared<Entity::Snake>(frame, start);
    snakes[sessionID] = snake;
    snake->SetName(session->GetName());

    return snake;
}

void Logic::SetDestination(const Interface::Session::Shared & session, const sf::Vector2f & dest)
{
    auto sessionID = session->ID();

    if(!snakes.contains(sessionID)) {
        Log()->Error("SetDestination(): Snake for session [{}] does not exists", session->ID());
        return;
    }

    auto snake = snakes[sessionID];
    snake->SetDestination(dest);
}

bool Logic::MoveSnake(const Entity::Snake::Shared & snake)
{
    auto head = snake->TryMove();
    if(Math::CheckBoundsCollision(head, AreaRadius - snake->GetRadius(true)))
        return false;

    snake->AcceptMove(head);
    return true;
}

void Logic::SendFullSessionUpdate(const Interface::Session::Shared & session)
{
    auto sessionID = session->ID();
    if(!snakes.contains(sessionID)) {
        Log()->Error("Snake for session [{}] does not exists", session->ID());
        return;
    }

    auto snake = snakes[sessionID];

    auto center = snake->GetPosition();

    Message::DataUpdate dataUpdate;

    for(auto & [targetSessionID, targetSnake]: snakes) {
        if(snake->CanSee(targetSnake)) {
            auto data = targetSnake->GetDataUpdate();
            data.session = targetSessionID;

            dataUpdate.snakes.push_back(data);
        }
    }

    for(auto & food: foods) {
        if(snake->CanSee(food)) {
            auto data = food->GetDataUpdate();
            dataUpdate.foods.push_back(data);
        }
    }

    dataUpdate.serverFrame = frame;

    sf::Packet packet;
    packet << dataUpdate;

    auto status = session->SendPacket(Message::NetDataUpdate, packet);
    if(status != sf::Socket::Status::Done){
        Log()->Error("SendFullSessionUpdate({}): error {} size {}", sessionID, (int)status, packet.getDataSize());
    }
}

void Logic::ReceiveFullSessionUpdate(const Interface::Session::Shared & session, Message::DataUpdate & update)
{
    foods.clear();
    snakes.clear();

    for(auto & snakeData: update.snakes) {
        auto snake = std::make_shared<Entity::Snake>();
        snake->SetDataUpdate(snakeData);
        snakes[snakeData.session] = snake;
    }

    for(auto & foodData: update.foods) {
        auto food = std::make_shared<Entity::Food>();
        food->SetDataUpdate(foodData);
        foods.push_back(food);
    }

#ifdef BUILD_CLIENT
    graphics->SetServerFrame(update.serverFrame);
#endif
}

void Logic::SendMoveDestination(const Interface::Session::Shared & session, const sf::Vector2f & dest)
{
    Message::Move move = {.session = session->ID(), .destination = dest};

    sf::Packet packet;
    packet << move;

    session->SendPacket(Message::NetMove, packet);
}

void Logic::StartGame(const std::string & name)
{
    gameState = GameLoading;
    server->Connect(name);
}

GameState Logic::State()
{
    return gameState;
}