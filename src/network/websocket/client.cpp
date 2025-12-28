#include "client.hpp"
#include "message.hpp"
#include "headers.hpp"

#include <chrono>

#include "utils.hpp"

namespace Core::Network::Websocket {
    using namespace std::chrono_literals;

    void WebsocketClient::Initialise()
    {
        auto host = Utils::Env("WS_HOST");
        if (host.empty())
            host = "127.0.0.1";
        Net::ClientConfig config;
        config.host      =  host;
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

        const auto now = std::chrono::steady_clock::now();

        for (auto it = jobsHandlers_.begin(); it != jobsHandlers_.end(); )
        {
            if (it->second.expireAt <= now)
            {
                auto callback = std::move(it->second.callback);

                it = jobsHandlers_.erase(it);

                callback({});
            }
            else
            {
                ++it;
            }
        }
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
        Log()->Debug("Message: {}", serialize(jsonValue));

        if (!jsonValue.is_object()) {
            Log()->Warning("Incoming JSON is not an object");
            return;
        }

        const boost::json::object & obj = jsonValue.as_object();

        const boost::json::value * headersValue = obj.if_contains("headers");
        if (headersValue == nullptr || !headersValue->is_object()) {
            Log()->Warning("Incoming JSON has no string 'headers' field");
            return;
        }

        const auto headers = Headers::Create(this, headersValue->as_object());

        const boost::json::value * typeValue = obj.if_contains("type");
        if (typeValue == nullptr || !typeValue->is_string()) {
            Log()->Warning("Incoming JSON has no string 'type' field");
            return;
        }

        const auto type = std::string(typeValue->as_string());

        const boost::json::value * messageValue = obj.if_contains("message");
        if (messageValue == nullptr || !messageValue->is_object()) {
            Log()->Warning("Incoming JSON has no object 'message' field");
            return;
        }

        const auto message = Message::Create(this, headers, type, messageValue->get_object());

        if (const auto targetJobID = headers->GetTargetJobID(); jobsHandlers_.contains(targetJobID))
        {
            jobsHandlers_[targetJobID].callback(message);
            jobsHandlers_.erase(targetJobID);
            return;
        }

        if (const auto it = messageHandlers_.find(type); it != messageHandlers_.end()) {
            for (const auto & handler : it->second)
                handler(message);

            return;
        }

        Log()->Warning("No handler registered for type '{}'", type);
    }

    [[nodiscard]] WebsocketClient::ConnectionState WebsocketClient::GetConnectionState() const
    {
        return state_;
    }

    [[nodiscard]] std::string WebsocketClient::GetConnectionError() const
    {
        return connectionError_;
    }

    uint64_t WebsocketClient::Send(const std::string & type, const boost::json::object & body, uint64_t targetJobID)
    {
        if (!IsConnected())
        {
            Log()->Warning("Send in disconnected state!");
            return 0;
        }

        const auto sourceJobID = sourceJobID_++;

        boost::json::object headers = {
            {"sourceJobId", sourceJobID},
            {"targetJobId", targetJobID},
        };

        boost::json::object message = {
            {"type", type},
            {"headers", headers},
            {"message", body},
        };

        client_->Send(message);

        return sourceJobID;
    }

    Utils::Task<Interface::Message::Shared> WebsocketClient::Request(const std::string & type, const boost::json::object & body, uint64_t timeout)
    {
        if (!IsConnected())
        {
            Log()->Warning("Request in disconnected state!");
            co_return {};
        }

        const auto sourceJobID = Send(type, body);

        Message::Shared response;

        co_await Utils::AwaitablePromiseTask([=, this, &response](const Utils::TaskResolver & resolver)  {
            RegisterJobCallback(sourceJobID, [=, &response](const Interface::Message::Shared & message) {
                response = Message::Impl(message);
                resolver->Resolve();
            }, timeout);
        });

        if (!response)
        {
            Log()->Error("Request '{}' timeout", type);
            co_return {};
        }

        co_return response;
    }

    void WebsocketClient::RegisterJobCallback(uint64_t jobID, const MessageCallback & callback, uint64_t timeout)
    {
        const auto now = std::chrono::steady_clock::now();

        jobsHandlers_[jobID] = JobHandler{
            .expireAt = now + std::chrono::milliseconds{ timeout },
            .callback = callback
        };
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