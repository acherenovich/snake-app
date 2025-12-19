#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <boost/json/object.hpp>
#include <boost/json/value.hpp>

#include "../client.hpp"

namespace Core::Network::Websocket
{
    namespace Response::Detail
    {
        inline const boost::json::value* Find(const boost::json::object& obj, const std::string_view key)
        {
            return obj.if_contains(key);
        }

        inline std::optional<bool> GetBool(const boost::json::object& obj, const std::string_view key)
        {
            const auto* v = Find(obj, key);
            if (!v || !v->is_bool())
                return std::nullopt;
            return v->as_bool();
        }

        inline std::optional<std::string_view> GetStringView(const boost::json::object& obj, const std::string_view key)
        {
            const auto* v = Find(obj, key);
            if (!v || !v->is_string())
                return std::nullopt;
            return v->as_string().c_str();
        }

        inline std::string TypeName(const boost::json::value& v)
        {
            if (v.is_null())   return "null";
            if (v.is_bool())   return "bool";
            if (v.is_int64())  return "int64";
            if (v.is_uint64()) return "uint64";
            if (v.is_double()) return "double";
            if (v.is_string()) return "string";
            if (v.is_array())  return "array";
            if (v.is_object()) return "object";
            return "unknown";
        }
    }

    namespace Response
    {
        class Base
        {
        public:
            explicit Base(const Interface::Message::Shared& message)
                : message_(message)
            {
                if (!message_)
                {
                    error_ = "invalid_response: message is null";
                    return;
                }

                try
                {
                    body_ = &message_->GetBody();
                }
                catch (...)
                {
                    error_ = "failed_to_parse_response: exception";
                }
            }

            virtual ~Base() = default;

            Base(const Base&) = delete;
            Base& operator=(const Base&) = delete;

            Base(Base&&) = default;
            Base& operator=(Base&&) = default;

            [[nodiscard]] bool Ok() const { return error_.empty(); }
            [[nodiscard]] std::string Error() const { return error_; }

        protected:
            [[nodiscard]] const boost::json::object* Body() const
            {
                return body_;
            }

            [[nodiscard]] bool HasBody() const
            {
                return body_ != nullptr;
            }

            void SetError(std::string error)
            {
                if (error_.empty())
                    error_ = std::move(error);
            }

        private:
            Interface::Message::Shared message_;
            boost::json::object* body_ = nullptr;
            std::string error_;
        };
    }
}