#pragma once

#include "[core_loader].hpp"
#include "double_map.hpp"

namespace Core::App::Game
{
    enum MainState
    {
        MainState_Connecting,
        MainState_Login,
        MainState_Menu,
        MainState_JoiningSession,
    };

    static DoubleMap<MainState> MainStateMap {{
        {MainState_Connecting, "connecting"},
        {MainState_Login, "login"},
        {MainState_Menu, "menu"},
        {MainState_JoiningSession, "joining_session"},
    }};
}