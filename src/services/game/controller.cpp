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
        localStorage_ = IFace().Get<Storage>();

        client_->RegisterConnectionStateCallback([this](const ConnectionState & old, const ConnectionState & current) {
            connectionState_ = current;

            if (connectionState_ == ConnectionState::ConnectionState_Connecting)
            {
                SetMainState(MainState_Connecting);
            }

            if (connectionState_ == ConnectionState::ConnectionState_Connected)
            {
                SetMainState(MainState_Login);
            }
        });
    }

    void Controller::ProcessTick()
    {
        if (gameClient_)
            gameClient_->ProcessTick();
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

        Network::Websocket::Response::PlayerSession response(message);
        if (!response.Success())
        {
            co_return {.error = response.Error()};
        }

        if (save)
            localStorage_->Save("snake_user_token", boost::json::string(response.Token().value()));
        else
            localStorage_->Delete("snake_user_token");

        profile_.Login(std::string(response.Login().value_or("")), response.PlayerId().value_or(0), response.Experience().value_or(0));
        SetMainState(MainState_Menu);

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

        Network::Websocket::Response::PlayerSession response(message);
        if (!response.Success())
        {
            co_return {.error = response.Error()};
        }

        localStorage_->Save("snake_user_token", boost::json::string(response.Token().value()));

        profile_.Login(std::string(response.Login().value_or("")), response.PlayerId().value_or(0), response.Experience().value_or(0));
        SetMainState(MainState_Menu);

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

        Network::Websocket::Response::PlayerSession response(message);
        if (!response.Success())
        {
            co_return {.error = response.Error()};
        }

        localStorage_->Save("snake_user_token", boost::json::string(response.Token().value()));

        profile_.Login(std::string(response.Login().value_or("")), response.PlayerId().value_or(0), response.Experience().value_or(0));
        SetMainState(MainState_Menu);

        co_return {.success = true};
    }

    void Controller::Logout()
    {
        localStorage_->Delete("snake_user_token");
        client_->Send("player_session::logout", {});
        SetMainState(MainState_Login);
        profile_.Logout();
    }

    Utils::Task<ActionResult<Controller::Stats>> Controller::GetStats()
    {
        if (!client_->IsConnected())
            co_return {.success = false};

        const auto message = co_await client_->Request("player_session::stats", {});
        if (!message)
            co_return {.error = "timeout"};

        auto & json = message->GetBody();

        Stats stats;

        try {
            for (auto sessionJson: json["body"].as_object()["sessions"].as_array())
            {
                auto sessionVal = sessionJson.as_object();

                auto & [sessionID_, players_] = stats.sessions_.emplace_back();
                sessionID_= sessionVal["id"].as_int64();
                players_ = sessionVal["players"].as_int64();
            }
        }
        catch (...)
        {
            co_return {.success = false};
        }

        co_return {.success = true, .result = stats};
    }


    Utils::Task<ActionResult<>> Controller::JoinSession(uint32_t sessionID)
    {
        SetMainState(MainState_Playing);

        gameClient_ = GameClient::Create(this, sessionID);

        co_return {.success = true};
    }

    Interface::GameClient::Shared Controller::GetCurrentGameClient() const
    {
        return gameClient_;
    }

    const Controller::Profile & Controller::GetProfile() const
    {
        return profile_;
    }

    void Controller::ExitToMenu()
    {
        gameClient_ = {};
        SetMainState(MainState_Menu);
    }
}
