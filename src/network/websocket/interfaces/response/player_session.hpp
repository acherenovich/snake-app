#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <cstdint>

#include <boost/json/object.hpp>
#include <boost/json/value.hpp>

#include "[base].hpp"

namespace Core::Network::Websocket::Response
{
    class PlayerSession final : public Base
    {
    public:
        explicit PlayerSession(const Interface::Message::Shared& message)
            : Base(message)
        {
            if (!Ok() || !HasBody())
                return;

            Parse(*Body()); // Body содержит весь root: { success, body? / message? }
        }

        [[nodiscard]] bool Success() const
        {
            return Error().empty() && success_.value_or(false);
        }

        [[nodiscard]] std::optional<std::uint64_t> PlayerId() const { return id_; }

        [[nodiscard]] std::optional<std::string_view> Login() const
        {
            if (login_.empty()) return std::nullopt;
            return login_;
        }

        [[nodiscard]] std::optional<std::uint64_t> Experience() const { return experience_; }

        [[nodiscard]] std::optional<std::string_view> PlayerType() const
        {
            if (playerType_.empty()) return std::nullopt;
            return playerType_;
        }

        [[nodiscard]] std::optional<std::string_view> Token() const
        {
            if (token_.empty()) return std::nullopt;
            return token_;
        }

    private:
        void Parse(const boost::json::object& root)
        {
            // success (обязателен всегда)
            const auto* successValue = Detail::Find(root, "success");
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

            // ошибка: body нет, есть message на корне
            if (!*success_)
            {
                const auto* messageValue = Detail::Find(root, "message");
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

                SetError(std::string{ messageValue->as_string().c_str() });
                return;
            }

            // успех: body обязателен
            const auto* bodyValue = Detail::Find(root, "body");
            if (!bodyValue || !bodyValue->is_object())
            {
                SetError("invalid_response: missing field 'body' (object) for success=true");
                return;
            }

            const auto& body = bodyValue->as_object();

            // body.player
            const auto* playerValue = Detail::Find(body, "player");
            if (!playerValue || !playerValue->is_object())
            {
                SetError("invalid_response: missing field 'player' (object)");
                return;
            }

            // body.player.model
            const auto* modelValue = Detail::Find(playerValue->as_object(), "model");
            if (!modelValue || !modelValue->is_object())
            {
                SetError("invalid_response: missing field 'model' (object)");
                return;
            }

            const auto& model = modelValue->as_object();

            if (!ExtractUInt(model, "id", id_))                    return SetInvalidField("id");
            if (!ExtractString(model, "login", login_))            return SetInvalidField("login");
            if (!ExtractUInt(model, "experience", experience_))    return SetInvalidField("experience");
            if (!ExtractString(model, "playerType", playerType_))  return SetInvalidField("playerType");
            if (!ExtractString(model, "token", token_))            return SetInvalidField("token");
        }

        void SetInvalidField(const std::string_view field)
        {
            SetError("invalid_response: invalid or missing field '" + std::string(field) + "'");
        }

        static bool ExtractString(const boost::json::object& obj,
                                  const std::string_view key,
                                  std::string& out)
        {
            const auto* v = Detail::Find(obj, key);
            if (!v || !v->is_string())
                return false;

            out = v->as_string().c_str();
            return true;
        }

        static bool ExtractUInt(const boost::json::object& obj,
                                const std::string_view key,
                                std::optional<std::uint64_t>& out)
        {
            const auto* v = Detail::Find(obj, key);
            if (!v)
                return false;

            if (v->is_uint64())
            {
                out = v->as_uint64();
                return true;
            }

            if (v->is_int64())
            {
                const auto val = v->as_int64();
                if (val < 0)
                    return false;

                out = static_cast<std::uint64_t>(val);
                return true;
            }

            return false;
        }

    private:
        std::optional<bool> success_{};

        std::optional<std::uint64_t> id_;
        std::optional<std::uint64_t> experience_;

        std::string login_;
        std::string playerType_;
        std::string token_;
    };
}