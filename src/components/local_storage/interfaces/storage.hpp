#pragma once

#include <boost/json/value.hpp>

#include "common.hpp"

namespace Core::Components::LocalStorage {
    namespace Interface {
        class Storage: public BaseServiceInterface, public BaseServiceInstance
        {
        public:
            using Shared =  std::shared_ptr<Storage>;

            [[nodiscard]] virtual boost::json::value Get(const std::string & key) = 0;

            [[nodiscard]] virtual bool Has(const std::string & key) = 0;

            virtual void Save(const std::string & key, const boost::json::value & value) = 0;

            virtual void Delete(const std::string & key) = 0;
        };
    }
}
