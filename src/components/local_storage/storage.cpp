#include "storage.hpp"

#include <boost/json.hpp>

#include <fstream>
#include <iomanip>
#include <sstream>

namespace Core::Components::LocalStorage {

    namespace fs = std::filesystem;
    namespace json = boost::json;

    fs::path Storage::GetStorageDir()
    {
        return fs::temp_directory_path() / "LocalStorage";
    }

    std::string Storage::SanitizeKeyToFileName(const std::string & key)
    {
        std::ostringstream out;

        for (const unsigned char c : key)
        {
            const bool ok =
                (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                (c == '_') || (c == '-');

            if (ok)
            {
                out << static_cast<char>(c);
            }
            else
            {
                out << '_'
                    << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(c)
                    << std::nouppercase << std::dec;
            }
        }

        const auto result = out.str();
        return result.empty() ? "empty_key" : result;
    }

    fs::path Storage::GetPathForKey(const std::string & key) const
    {
        return storageDir_ / (SanitizeKeyToFileName(key) + ".json");
    }

    void Storage::Initialise()
    {
        std::lock_guard lock(mutex_);

        storageDir_ = GetStorageDir();

        std::error_code ec;
        fs::create_directories(storageDir_, ec);

        initialised_ = !ec && fs::exists(storageDir_);
    }

    void Storage::OnAllServicesLoaded()
    {
        IFace().Register<Interface::Storage>(shared_from_this());
    }

    void Storage::OnAllInterfacesLoaded()
    {
    }

    void Storage::ProcessTick()
    {
    }

    boost::json::value Storage::Get(const std::string & key)
    {
        std::lock_guard lock(mutex_);

        if (!initialised_ || key.empty())
            return { nullptr };

        const fs::path path = GetPathForKey(key);

        std::error_code ec;
        if (!fs::exists(path, ec) || ec)
            return { nullptr };

        std::ifstream in(path, std::ios::binary);
        if (!in.is_open())
            return { nullptr };

        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        try
        {
            return json::parse(content);
        }
        catch (...)
        {
            return { nullptr };
        }
    }

    bool Storage::Has(const std::string & key)
    {
        std::lock_guard lock(mutex_);

        if (!initialised_ || key.empty())
            return false;

        const fs::path path = GetPathForKey(key);

        std::error_code ec;
        const bool exists = fs::exists(path, ec);
        return exists && !ec;
    }

    void Storage::Save(const std::string & key, const boost::json::value & value)
    {
        std::lock_guard lock(mutex_);

        if (!initialised_ || key.empty())
            return;

        const fs::path finalPath = GetPathForKey(key);
        const fs::path tmpPath = finalPath.string() + ".tmp";

        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
            return;

        const std::string data = json::serialize(value);
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
        out.close();

        std::error_code ec;

        fs::remove(finalPath, ec);
        ec.clear();

        fs::rename(tmpPath, finalPath, ec);
        if (ec)
        {
            std::error_code ignore;
            fs::remove(tmpPath, ignore);
        }
    }

    void Storage::Delete(const std::string & key)
    {
        std::lock_guard lock(mutex_);

        if (!initialised_ || key.empty())
            return;

        const fs::path path = GetPathForKey(key);

        std::error_code ec;
        fs::remove(path, ec);
    }

}