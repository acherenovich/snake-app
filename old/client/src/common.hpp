#pragma once

#include <algorithm>
#include <cmath>

#include "logging.hpp"
#include "event_system2.hpp"

enum GlobalEvents {
    GlobalInitialise,
    GlobalInitialisePost,
    GlobalFrame,

    GlobalFrameSetup,
    CallStop,

    GameInterfaceLoaded,
    GraphicsInterfaceLoaded,
    ServerInterfaceLoaded,
};

struct Initializer {
    Utils::Log::Shared log;
    Utils::Event::System<GlobalEvents> & globalEvents;
};