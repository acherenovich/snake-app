#pragma once

#include "logging.hpp"
#include "event_system2.hpp"

enum GlobalEvents {
    GlobalInitialise,
    GlobalInitialisePost,
    GlobalFrame,

    CallStop,

    GameInterfaceLoaded,
    ServerInterfaceLoaded,
};

struct Initializer {
    Utils::Log::Shared log;
    Utils::Event::System<GlobalEvents> & globalEvents;
};