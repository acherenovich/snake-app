#include "game/logic.hpp"
#include "server/connection.hpp"

#include "common.hpp"

int main()
{
    bool running = true;

    auto log = Utils::Logger("[SNAKE]");
    Utils::Event::System<GlobalEvents> events;

    events.HookEvent(CallStop) = std::function([&](){
        running = false;
    });

    Connection connection(
        {.log = log->CreateChild("[NET]"), .globalEvents = events});
    Logic logic(
        {.log = log->CreateChild("[LOGIC]"), .globalEvents = events});

    log->Debug("Start initialise process");

    events.CallEvent(GlobalInitialise);
    events.CallEvent(GlobalInitialisePost);

    constexpr int TickRate = 64;
    constexpr auto FrameDuration = std::chrono::milliseconds(1000 / TickRate);

    log->Debug("Running with ({}) tick rate", TickRate);
    while (running) {
        auto frame_start = std::chrono::steady_clock::now();

        events.CallEvent(GlobalFrame);

        auto frame_end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);

        if (elapsed < FrameDuration) {
            std::this_thread::sleep_for(FrameDuration - elapsed);
        }
        else {
            log->Warning("Too slow frame!");
        }
    }

    return 0;
}
