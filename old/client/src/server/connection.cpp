#include "connection.hpp"
#include "packets.hpp"

#include <utility>

#include "game.hpp"

Connection::Connection(Initializer init) :
    initializer(std::move(init))
{
    initializer.globalEvents.HookEvent(GlobalInitialise) = std::function([this]() {
        initializer.globalEvents.CallEvent(ServerInterfaceLoaded, this);
    });


#ifdef BUILD_CLIENT
#else
    auto status = listener.listen(ServerPort);
    if(status != sf::Socket::Done) {
        Log()->Error("Failed to bind address {}", ServerIP.toString());
        std::terminate();
    }

    listener.setBlocking(false);
#endif


    initializer.globalEvents.HookEvent(GlobalFrame) = std::function([this](uint32_t frame_) {
        frame = frame_;
        Think();
    });
}

void Connection::Connect(const std::string & name)
{
    auto socket = std::make_shared<sf::TcpSocket>();
    sf::Socket::Status status = socket->connect(ServerIP, ServerPort);
    if (status != sf::Socket::Done)
    {
        Log()->Error("Failed to connect to server");
        std::terminate();
    }

    socket->setBlocking(false);

    auto session = Session::CreateSession(name, socket);
    session->SetID(0);
    sessions[0] = session;

    Message::Login login {.session = session->ID(), .name = name};
    if(session->SendPacket(Message::Type::NetLogin, login) != sf::Socket::Status::Done) {
        Log()->Error("Could not send login request");
    }
}

Interface::Session::Shared Connection::GetSession(uint64_t id)
{
    if(sessions.empty())
        return {};

    if(!sessions.contains(id))
        return {};

    return sessions[id];
}

void Connection::Think()
{
    std::map<Message::Type, std::function<void(sf::Packet &, const Session::Shared & session)>> handlers;

    handlers[Message::Type::NetLogin] = [this](sf::Packet & packet, const Session::Shared & session) {
        Message::Login login;
        packet >> login;

        Message::SessionIncoming sessionIncoming {.session = session->ID(), .serverFrame = frame};
        if(session->SendPacket(Message::Type::NetSessionIncoming, sessionIncoming) != sf::Socket::Status::Done) {
            Log()->Error("Could not send session data to user({})", session->ID());
        }

        session->SetName(login.name);

        events.CallEvent(SessionConnected, session);
    };

    handlers[Message::Type::NetMove] = [this](sf::Packet & packet, const Session::Shared & session) {
        Message::Move move;
        packet >> move;

        events.CallEvent(MoveRequest, session, move.destination);
    };

    handlers[Message::Type::NetSessionIncoming] = [this](sf::Packet & packet, const Session::Shared & session) {
        Message::SessionIncoming incoming {};
        packet >> incoming;

        session->SetID(incoming.session);

        initializer.globalEvents.CallEvent(GlobalFrameSetup, incoming.serverFrame);
        events.CallEvent(SessionConnected, GetSession());
    };

    handlers[Message::Type::NetDataUpdate] = [this](sf::Packet & packet, const Session::Shared & session) {
        Message::DataUpdate update {};
        packet >> update;

        events.CallEvent(DataUpdate, GetSession(), update);
    };

    handlers[Message::Type::NetLeaderboard] = [this](sf::Packet & packet, const Session::Shared & session) {
        Message::LeaderBoard data {};
        packet >> data;

        events.CallEvent(LeaderboardUpdate, data);
    };

#ifdef BUILD_CLIENT

#else
    auto newClient = std::make_shared<sf::TcpSocket>();
    if (listener.accept(*newClient) == sf::Socket::Done)
    {
        newClient->setBlocking(false);

        auto session = Session::CreateSession("connected", newClient);
        sessions[session->ID()] = session;

        Log()->Debug("New client connected: {}", newClient->getRemoteAddress().toString());
    }
#endif

    for (auto & [sessionID, session] : sessions)
    {
        if(session->IsDisconnected())
            continue;

        while (true)
        {
            ZipPacket packet;
            auto status = session->Socket()->receive(packet);

            if (status == sf::Socket::Done)
            {
                Message::Header header;
                packet >> header;

                if (handlers.find(header.type) != handlers.end())
                {
                    handlers[header.type](header.data, session);
                } else
                {
                    Log()->Error("Unhandled packet type from client: {}", static_cast<uint32_t>(header.type));
                }
            }
            else if (status == sf::Socket::Disconnected)
            {
                session->SetDisconnected();
                Log()->Debug("Client disconnected: {}", session->Socket()->getRemoteAddress().toString());
                break;
            }
            else
            {
                break;
            }
        }
    }
}