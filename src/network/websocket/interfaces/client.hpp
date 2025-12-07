#pragma once

#include "[core_loader].hpp"

#include <boost/json.hpp>

namespace Core::Network::Websocket {
    namespace Interface {
        class Message: public BaseServiceContainer
        {
        public:
            using Shared = std::shared_ptr<Message>;

            ~Message() override = default;

            [[nodiscard]] virtual const std::string & GetType() const = 0;

            [[nodiscard]] virtual boost::json::object & GetBody() = 0;

            [[nodiscard]] virtual std::string ToString() const = 0;
        };

        class Client: public BaseServiceInterface
        {
        public:
            enum ConnectionState
            {
                ConnectionState_Connecting,
                ConnectionState_ConnectingFailed,
                ConnectionState_Connected,
            };
        public:
            using Shared = std::shared_ptr<Client>;

            virtual ~Client() = default;

            [[nodiscard]] bool IsConnected() const
            {
                return GetConnectionState() == ConnectionState_Connected;
            }

            [[nodiscard]] virtual ConnectionState GetConnectionState() const = 0;

            [[nodiscard]] virtual std::string GetConnectionError() const = 0;

            virtual void Send(const std::string & type, const boost::json::object & body) = 0;

            using MessageCallback = std::function<void(const Message::Shared &)>;
            virtual void RegisterMessage(const std::string & type, const MessageCallback & callback) = 0;

            using ConnectionStateCallback = std::function<void(const ConnectionState &, const ConnectionState &)>;
            virtual void RegisterConnectionStateCallback(const ConnectionStateCallback & callback) = 0;
        };
    }
}
