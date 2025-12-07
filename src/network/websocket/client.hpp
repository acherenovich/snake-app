#pragma once

#include "interfaces/client.hpp"
#include "client.hpp"

#include <websocket.hpp>

#include <memory>
#include <string>
#include <unordered_map>

#include "interfaces/client.hpp"

namespace Core::Network::Websocket {
    namespace Net = Utils::Net::Websocket;

    class WebsocketService;

    class WebsocketClient final :
        public BaseServiceInstance,
        public Net::ClientListener,
        public Interface::Client,
        public std::enable_shared_from_this<WebsocketClient>
    {
        Net::Client::Shared client_;
        std::unordered_map<std::string, std::vector<MessageCallback>> messageHandlers_;
        std::vector<ConnectionStateCallback> connectionStateHandlers_;

        std::unordered_map<Net::Session::Shared, Client::Shared> clients_;

        ConnectionState state_ = ConnectionState_Connecting;
        std::string connectionError_;
    public:
        using Shared    = std::shared_ptr<WebsocketClient>;

        WebsocketClient() = default;
        ~WebsocketClient() override = default;

    protected:
        void Initialise() override;
        void OnAllServicesLoaded() override;
        void OnAllInterfacesLoaded() override;
        void ProcessTick() override;
    public:
        void OnConnected() override;
        void OnDisconnected() override;

        void OnConnectionError(const std::string &error, bool reconnect) override;

        void OnMessage(const boost::json::value & json) override;

    public:
        [[nodiscard]] ConnectionState GetConnectionState() const override;

        [[nodiscard]] std::string GetConnectionError() const override;

        void Send(const std::string & type, const boost::json::object & body) override;

        void RegisterMessage(const std::string & type, const MessageCallback & callback) override;

        void RegisterConnectionStateCallback(const ConnectionStateCallback & callback) override;

        void SetState(const ConnectionState & state);

        [[nodiscard]] Net::Client::Shared & Client()
        {
            return client_;
        }

    private:

        std::string GetServiceContainerName() const override
        {
            return "NET-WS";
        }
    };

} // namespace Core::Servers::Websocket