#pragma once

#include "interfaces/storage.hpp"

#include <filesystem>
#include <mutex>
#include <string>

namespace Core::Components::LocalStorage {

    class Storage final : public Interface::Storage, public std::enable_shared_from_this<Storage>
    {
    public:
        using Shared = std::shared_ptr<Storage>;

        void Initialise() override;
        void OnAllServicesLoaded() override;
        void OnAllInterfacesLoaded() override;
        void ProcessTick() override;

        [[nodiscard]] boost::json::value Get(const std::string & key) override;
        [[nodiscard]] bool Has(const std::string & key) override;
        void Save(const std::string & key, const boost::json::value & value) override;
        void Delete(const std::string & key) override;

    private:
        std::string GetServiceContainerName() const override
        {
            return "LocalStorage";
        }

    private:
        static std::filesystem::path GetStorageDir() ;
        std::filesystem::path GetPathForKey(const std::string & key) const;

        static std::string SanitizeKeyToFileName(const std::string & key);

    private:
        mutable std::mutex mutex_;
        std::filesystem::path storageDir_;
        bool initialised_{ false };
    };
}