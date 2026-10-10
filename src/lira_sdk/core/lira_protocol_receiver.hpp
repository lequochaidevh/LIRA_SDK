#pragma once

#include <cstdint>
#include <mutex>
#include <array>
#include <memory>
#include <functional>

#include "log.hpp"

// Include our auto-generated pure C serialization headers
#include "lira_protocol_include.hpp"

namespace lirasdk {

// Map the missing envelope type directly onto your pure C structure
class LiraProtocolReceiver {
 private:
    using lira_protocol_message_t = lira_message_t;
    class CallbackWrapper {
     public:
        virtual ~CallbackWrapper()                   = default;
        virtual bool execute(const uint8_t* payload) = 0;
    };

    template <typename T>
    class TypedCallbackWrapper : public CallbackWrapper {
     public:
        explicit TypedCallbackWrapper(std::function<bool(const T&)> cb) : _cb(cb) {}
        bool execute(const uint8_t* payload) override {
            T decoded_struct;
            LiraMessageTraits<T>::decode(&decoded_struct, payload);
            return _cb(decoded_struct);
        }

     private:
        std::function<bool(const T&)> _cb;
    };

    std::mutex    _mutex{};
    lira_status_t _parser_status{};

 public:
    // Define the missing ParseResult enum explicitly
    enum class ParseResult {
        Incomplete,     // Stream byte received, but frame is still parsing
        MessageParsed,  // A valid packet frame just parsed successfully
        BadCrc,         // Checksum calculation verification failed
        UnknownMessage  // Decoded msgid doesn't match entries in our MessageSet database
    };

    LiraProtocolReceiver();
    ~LiraProtocolReceiver() = default;

    LiraProtocolReceiver(const LiraProtocolReceiver&) = delete;
    LiraProtocolReceiver& operator=(const LiraProtocolReceiver&) = delete;

    /**
     * @brief Modifying parse_bytes hook to return ParseResult and populate the message by reference
     */
    ParseResult parse_bytes(uint8_t c, lira_protocol_message_t& message);

    /**
     * @brief O(1) TYPE-SAFE REGISTRATION: Fast direct index insertion
     */
    template <typename MessageType>
    void register_callback(std::function<bool(const MessageType&)> callback) {
        std::lock_guard<std::mutex> lock(_mutex);

        uint32_t msgid = LiraMessageTraits<MessageType>::msgid;

        // Safety guard for array bounds
        if (msgid < LIRA_MAX_MESSAGE_ID) {
            _callbacks[msgid].push_back(std::make_unique<TypedCallbackWrapper<MessageType>>(callback));
        }
    }

    void route_verified_message(const lira_protocol_message_t& message);

    std::array<std::vector<std::unique_ptr<CallbackWrapper>>, LIRA_MAX_MESSAGE_ID> _callbacks{};
};

}  // namespace lirasdk