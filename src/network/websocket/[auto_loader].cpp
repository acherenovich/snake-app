#include "[core_loader].hpp"
#include "client.hpp"

namespace Core::Network::Websocket {
    [[maybe_unused]] __used Utils::Service::Loader::Add<WebsocketClient>NetworkWebsocketClient(ServicesLoader());
} // namespace Core::Servers::Websocket