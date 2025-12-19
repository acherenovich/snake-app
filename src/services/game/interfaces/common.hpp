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

    template <class T = void>
    struct ActionResult
    {
        std::string error = "";
        bool success = false;
        T result{};
    };

    template <>
    struct ActionResult<void>
    {
        std::string error = "";
        bool success = false;

    };
}