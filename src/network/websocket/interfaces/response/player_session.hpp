#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <boost/json/object.hpp>
#include <boost/json/value.hpp>

#include "[base].hpp"

namespace Core::Network::Websocket
{
    namespace Response
    {
        class PlayerSessionLogin final : public Base
        {
        public:
            explicit PlayerSessionLogin(const Interface::Message::Shared& message)
                : Base(message)
            {
                if (!Ok() || !HasBody())
                    return;

                Parse(*Body());
            }

            [[nodiscard]] bool Success() const
            {
                return success_.value_or(false);
            }

            [[nodiscard]] std::optional<std::string_view> Token() const
            {
                if (token_.empty())
                    return std::nullopt;
                return std::string_view{ token_ };
            }

        private:
            void Parse(const boost::json::object& body)
            {
                const auto* successValue = Detail::Find(body, "success");
                if (!successValue)
                {
                    SetError("invalid_response: missing field 'success'");
                    return;
                }

                if (!successValue->is_bool())
                {
                    SetError("invalid_response: field 'success' expected bool, got " + Detail::TypeName(*successValue));
                    return;
                }

                success_ = successValue->as_bool();

                if (*success_)
                {
                    const auto* tokenValue = Detail::Find(body, "token");
                    if (!tokenValue)
                    {
                        SetError("invalid_response: missing field 'token' for success=true");
                        return;
                    }

                    if (!tokenValue->is_string())
                    {
                        SetError("invalid_response: field 'token' expected string, got " + Detail::TypeName(*tokenValue));
                        return;
                    }

                    token_ = tokenValue->as_string().c_str();
                }
                else
                {
                    // message появляется только при ошибке и является текстом ошибки
                    const auto* messageValue = Detail::Find(body, "message");
                    if (!messageValue)
                    {
                        SetError("invalid_response: missing field 'message' for success=false");
                        return;
                    }

                    if (!messageValue->is_string())
                    {
                        SetError("invalid_response: field 'message' expected string, got " + Detail::TypeName(*messageValue));
                        return;
                    }

                    // кладём server error text в base error_
                    SetError(std::string{ messageValue->as_string().c_str() });
                }
            }

        private:
            std::optional<bool> success_{};
            std::string token_;
        };
    }
}