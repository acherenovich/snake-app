#include "client.hpp"
#include "message.hpp"

#include <chrono>

namespace Core::Network::Websocket {
    using namespace std::chrono_literals;

    void WebsocketClient::Initialise()
    {
        Net::ClientConfig config;
        config.host   = "0.0.0.0";
        config.port      = 9100;
        config.mode      = Net::Mode::Json;
        config.ioThreads = 2;
        config.useTls    = false;

        client_ = Net::Client::Create(config, shared_from_this(), Log());

        Log()->Debug("Server created on {}:{}", config.host, config.port);
    }

    void WebsocketClient::OnAllServicesLoaded()
    {
        Log()->Debug("OnAllServicesLoaded()");

        IFace().Register<Interface::Client>(shared_from_this());
    }

    void WebsocketClient::OnAllInterfacesLoaded()
    {
        Log()->Debug("OnAllInterfacesLoaded()");
    }

    void WebsocketClient::ProcessTick()
    {
        client_->ProcessTick();
    }

    void WebsocketClient::OnConnected()
    {
        Log()->Debug("Client connected:");

        SetState(ConnectionState_Connected);
    }

    void WebsocketClient::OnDisconnected()
    {
        Log()->Debug("Client disconnected");

        SetState(ConnectionState_Connecting);
    }

    void WebsocketClient::OnConnectionError(const std::string &error, bool reconnect)
    {
        connectionError_ = error;
        SetState(ConnectionState_ConnectingFailed);
    }

    void WebsocketClient::OnMessage(const boost::json::value & jsonValue)
    {
        if (!jsonValue.is_object()) {
            Log()->Warning("Incoming JSON is not an object");
            return;
        }

        const boost::json::object & obj = jsonValue.as_object();

        const boost::json::value * typeValue = obj.if_contains("type");
        if (typeValue == nullptr || !typeValue->is_string()) {
            Log()->Warning("Incoming JSON has no string 'type' field");
            return;
        }

        const std::string type = std::string(typeValue->as_string());

        const auto it = messageHandlers_.find(type);
        if (it == messageHandlers_.end()) {
            Log()->Warning("No handler registered for type '{}'", type);
            return;
        }

        const boost::json::value * messageValue = obj.if_contains("message");
        if (messageValue == nullptr || !messageValue->is_object()) {
            Log()->Warning("Incoming JSON has no object 'message' field");
            return;
        }

        const auto message = Message::Create(this, type, messageValue->get_object());

        for (const auto & handler : it->second)
            handler(message);
    }

    [[nodiscard]] WebsocketClient::ConnectionState WebsocketClient::GetConnectionState() const
    {
        return state_;
    }

    [[nodiscard]] std::string WebsocketClient::GetConnectionError() const
    {
        return connectionError_;
    }

    void WebsocketClient::Send(const std::string & type, const boost::json::object & body)
    {
        if (IsConnected())
        {

        }
    }

    void WebsocketClient::RegisterMessage(const std::string & type, const MessageCallback & callback)
    {
        messageHandlers_[type].push_back(callback);
    }

    void WebsocketClient::RegisterConnectionStateCallback(const ConnectionStateCallback & callback)
    {
        connectionStateHandlers_.push_back(callback);
    }

    void WebsocketClient::SetState(const ConnectionState & state)
    {
        const auto prevState = state_;

        state_ = state;

        for (const auto & handler : connectionStateHandlers_)
            handler(prevState, state);
    }

} // namespace Core::Servers::Websocket