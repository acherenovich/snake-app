#include "controller.hpp"

#include "network/websocket/interfaces/response/player_session.hpp"


namespace Core::App::Game
{
    void Controller::Initialise()
    {

    }

    void Controller::OnAllServicesLoaded()
    {
        IFace().Register<Interface::Controller>(shared_from_this());
    }

    void Controller::OnAllInterfacesLoaded()
    {
        client_ = IFace().Get<Client>();

        client_->RegisterConnectionStateCallback([this](const ConnectionState & old, const ConnectionState & current) {
            connectionState_ = current;

            if (connectionState_ == ConnectionState::ConnectionState_Connected)
            {
                SetMainState(MainState_Login);
            }
        });
    }

    void Controller::ProcessTick()
    {
    }

    MainState Controller::GetMainState() const
    {
        return mainState_;
    }

    void Controller::SetMainState(const MainState & state)
    {
        mainState_ = state;
    }

    Utils::Task<ActionResult<>> Controller::PerformLogin(std::string login, std::string password, bool save)
    {
        const boost::json::object request = {
            {"login", login},
            {"password", password},
        };

        const auto message = co_await client_->Request("player_session::login", request);
        if (!message)
            co_return {.error = "timeout"};

        Network::Websocket::Response::PlayerSessionLogin response(message);
        if (!response.Success())
        {
            co_return {.error = response.Error()};
        }

        co_return {.success = true};
    }

    Utils::Task<ActionResult<>> Controller::PerformLogin(std::string token)
    {
        const boost::json::object request = {
            {"token", token},
        };

        const auto message = co_await client_->Request("player_session::login", request);
        if (!message)
            co_return {.error = "timeout"};

        Network::Websocket::Response::PlayerSessionLogin response(message);
        if (!response.Success())
        {
            co_return {.error = response.Error()};
        }

        co_return {.success = true};
    }

    Utils::Task<ActionResult<>> Controller::PerformRegister(std::string login, std::string password)
    {
        const boost::json::object request = {
            {"login", login},
            {"password", password},
        };

        const auto message = co_await client_->Request("player_session::register", request);
        if (!message)
            co_return {.error = "timeout"};

        co_return {};
    }
}