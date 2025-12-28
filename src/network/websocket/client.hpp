#pragma once

#include "interfaces/client.hpp"

#include <websocket.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace Core::Network::Websocket {
    namespace Net = Utils::Net::Websocket;

    class WebsocketService;

    class WebsocketClient final :
        public BaseServiceInstance,
        public Net::ClientListener,
        public Interface::Client,
        public std::enable_shared_from_this<WebsocketClient>
    {
        struct JobHandler
        {
            std::chrono::steady_clock::time_point expireAt;
            MessageCallback callback;
        };

        std::chrono::steady_clock::time_point lastKeepAlive_ = std::chrono::steady_clock::now();

        Net::Client::Shared client_;
        std::unordered_map<std::string, std::vector<MessageCallback>> messageHandlers_;
        std::unordered_map<uint64_t, JobHandler> jobsHandlers_;
        std::vector<ConnectionStateCallback> connectionStateHandlers_;

        std::unordered_map<Net::Session::Shared, Client::Shared> clients_;

        ConnectionState state_ = ConnectionState_Connecting;
        std::string connectionError_;

        uint64_t sourceJobID_ = 1;
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

        uint64_t Send(const std::string & type, const boost::json::object & body, uint64_t targetJobID = 0) override;

        Utils::Task<Interface::Message::Shared> Request(const std::string & type, const boost::json::object & body, uint64_t timeout) override;

        void RegisterJobCallback(uint64_t jobID, const MessageCallback & callback, uint64_t timeout = 5000);

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