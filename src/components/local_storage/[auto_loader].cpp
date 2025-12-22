#include "[core_loader].hpp"
#include "storage.hpp"

namespace Core::Components::LocalStorage {
    [[maybe_unused]] [[gnu::used]] Utils::Service::Loader::Add<Storage>ComponentLocalStorage(ServicesLoader());
} // namespace Core::Servers::Websocket