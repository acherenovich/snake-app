#include "[core_loader].hpp"
#include "logging.hpp"
#include "coroutine.hpp"

#include <array>
#include <filesystem>
#include <thread>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#else
    #include <unistd.h>
#endif

namespace {
    std::filesystem::path ExecutableDirectory()
    {
#if defined(_WIN32)
        std::array<char, MAX_PATH> buffer{};
        const DWORD size = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (size == 0 || size == buffer.size())
            return std::filesystem::current_path();
        return std::filesystem::path(buffer.data()).parent_path();
#elif defined(__APPLE__)
        std::array<char, 4096> buffer{};
        uint32_t size = static_cast<uint32_t>(buffer.size());
        if (_NSGetExecutablePath(buffer.data(), &size) != 0)
            return std::filesystem::current_path();
        return std::filesystem::weakly_canonical(buffer.data()).parent_path();
#else
        std::array<char, 4096> buffer{};
        const ssize_t size = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
        if (size <= 0)
            return std::filesystem::current_path();
        buffer[static_cast<std::size_t>(size)] = '\0';
        return std::filesystem::path(buffer.data()).parent_path();
#endif
    }
}

int main()
{
    std::filesystem::current_path(ExecutableDirectory());

    const auto log = Utils::Logging::Logger::Create("CORE");
    Utils::SetDefaultLogger(log);

    Core::BaseServiceContainer baseContainer{ log };

    auto & loader = Core::ServicesLoader();

    loader.PreInitialise(baseContainer);
    loader.Initialise();
    loader.OnAllServicesLoaded();
    loader.OnAllInterfacesLoaded();

    log->Msg("Core services initialised");

    while (Core::AppRunning().load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        loader.ProcessTick();
        Utils::GetTaskManager().ClearFinishedTasks();
    }

    return 0;
}

// #include <logging.hpp>
// #include <websocket.hpp>
//
// #include <thread>
// #include <chrono>
//
// #include <boost/json/serialize.hpp>
//
// using namespace Utils::Net::Websocket;
//
// class MyServerListener final : public Listener
// {
// public:
//     void OnSessionConnected(const Session::Shared& session) override
//     {
//         session->Log()->Debug("Client connected: {}:{}",
//                              session->RemoteAddress(),
//                              session->RemotePort());
//     }
//
//     void OnSessionDisconnected(const Session::Shared& session) override
//     {
//         session->Log()->Debug("Client disconnected: {}:{}",
//                              session->RemoteAddress(),
//                              session->RemotePort());
//     }
//
//     void OnMessage(const Session::Shared& session,
//                    const std::vector<std::uint8_t>& data) override
//     {
//         session->Log()->Debug("Bytes from {}: size={}",
//                              session->RemoteAddress(),
//                              data.size());
//     }
//
//     void OnMessage(const Session::Shared& session,
//                    std::string_view text) override
//     {
//         session->Log()->Debug("Text from {}: {}",
//                              session->RemoteAddress(),
//                              text);
//
//         session->Close();
//         // echo обратно клиенту
//         // session->Send(std::string("echo: ") + std::string(text));
//     }
//
//     void OnMessage(const Session::Shared& session,
//                    const boost::json::value& jsonValue) override
//     {
//         session->Log()->Debug("Json from {}: {}",
//                              session->RemoteAddress(),
//                              boost::json::serialize(jsonValue));
//     }
// };
//
// class MyClientListener final : public ClientListener
// {
// public:
//     explicit MyClientListener(const Utils::Logging::Logger::Shared& logger)
//         : logger_(logger)
//     {}
//
//     void OnConnected() override
//     {
//         logger_->Debug("[CLIENT] Connected");
//     }
//
//     void OnDisconnected() override
//     {
//         logger_->Debug("[CLIENT] Disconnected");
//     }
//
//     void OnMessage(const std::vector<std::uint8_t>& data) override
//     {
//         logger_->Debug("[CLIENT] Bytes message, size={}", data.size());
//     }
//
//     void OnMessage(std::string_view text) override
//     {
//         logger_->Debug("[CLIENT] Text message: {}", text);
//     }
//
//     void OnMessage(const boost::json::value& jsonValue) override
//     {
//         logger_->Debug("[CLIENT] Json message: {}",
//                        boost::json::serialize(jsonValue));
//     }
//
// private:
//     Utils::Logging::Logger::Shared logger_;
// };
//
// int main()
// {
//     // CORE логгер
//     auto coreLog = Utils::Logging::Logger::Create("CORE");
//     Utils::SetDefaultLogger(coreLog);
//
//     coreLog->Error("Hello error!");
//     coreLog->Warning("Hello warning!");
//     coreLog->Debug("Hello debug!");
//     coreLog->Msg("Hello msg!");
//
//     // ----- сервер -----
//     ServerConfig serverConfig;
//     serverConfig.address   = "0.0.0.0";
//     serverConfig.port      = 9002;
//     serverConfig.mode      = Mode::Text;
//     serverConfig.ioThreads = 4;
//     // serverConfig.useTls = true/false и т.п. по желанию
//
//     auto serverListener = std::make_shared<MyServerListener>();
//     auto server         = Server::Create(serverConfig, serverListener, coreLog);
//
//     // ----- клиент -----
//     ClientConfig clientConfig;
//     clientConfig.host            = "127.0.0.1";
//     clientConfig.port            = 9002;
//     clientConfig.path            = "/";          // если у тебя есть путь
//     clientConfig.mode            = Mode::Text;
//     clientConfig.ioThreads       = 1;
//     clientConfig.useTls          = false;        // или true + tls настройки
//     clientConfig.autoReconnect   = true;
//     clientConfig.reconnectDelayMs = 1000;
//
//     auto clientLogger   = coreLog->CreateChild("CLIENT-EVENTS");
//     auto clientListener = std::make_shared<MyClientListener>(clientLogger);
//     auto client         = Client::Create(clientConfig, clientListener, coreLog);
//
//     // ----- простой игровой цикл -----
//     std::uint64_t tick = 0;
//     bool sentTestMessage = false;
//
//     while (true)
//     {
//         server->ProcessTick();
//         client->ProcessTick();
//
//         // через ~1 секунду после старта попробуем отправить сообщение
//         if (!sentTestMessage && tick > 100)
//         {
//             client->Log()->Debug("[CLIENT] Sending test message");
//             client->Send(std::string_view("hello from client!"));
//             sentTestMessage = true;
//         }
//
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//         ++tick;
//     }
// }
