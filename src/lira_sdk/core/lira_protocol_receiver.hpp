#pragma once

#include <cstdint>
#include <mutex>
#include <functional>

// Include our auto-generated pure C serialization headers
#include "lira_protocol_include.hpp"

namespace lirasdk {

// Map the missing envelope type directly onto your pure C structure
using lira_protocol_message_t = lira_message_t;

class LiraProtocolReceiver {
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

    // Subscribers hooks
    using heartbeat_callback_t = std::function<void(const lira_msg_heartbeat_t&)>;
    using gps_callback_t       = std::function<void(const lira_msg_gps_raw_int_t&)>;

    void register_heartbeat_callback(heartbeat_callback_t callback);
    void register_gps_callback(gps_callback_t callback);

 private:
    void route_verified_message(const lira_protocol_message_t& message);

    std::mutex    _mutex{};
    lira_status_t _parser_status{};

    heartbeat_callback_t _heartbeat_callback{nullptr};
    gps_callback_t       _gps_callback{nullptr};
};

}  // namespace lirasdk