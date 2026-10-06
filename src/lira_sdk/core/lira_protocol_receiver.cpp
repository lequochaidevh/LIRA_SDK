#include "lira_protocol_receiver.hpp"
#include <iostream>

namespace lirasdk {

LiraProtocolReceiver::LiraProtocolReceiver() {
    _parser_status.parse_state  = LIRA_PARSE_STATE_UNINIT;
    _parser_status.packet_idx   = 0;
    _parser_status.expected_len = 0;
}

LiraProtocolReceiver::ParseResult LiraProtocolReceiver::parse_bytes(uint8_t c, lira_protocol_message_t& message) {
    // Pipe character into auto-generated state machine decoder
    if (lira_parse_char(c, &message, &_parser_status)) {
        // Optional: Perform CRC validation here. If validation fails -> return ParseResult::BadCrc;

        // Validate if the message ID exists in our dictionary registry
        if (lira_get_message_length(message.msgid) == 0xFFFF) {
            return ParseResult::UnknownMessage;
        }

        // Message is fully verified and constructed!
        route_verified_message(message);

        return ParseResult::MessageParsed;
    }

    return ParseResult::Incomplete;
}

void LiraProtocolReceiver::route_verified_message(const lira_protocol_message_t& message) {
    switch (static_cast<lira_message_id_t>(message.msgid)) {
        case LIRA_MSG_ID_HEARTBEAT: {
            lira_msg_heartbeat_t heartbeat_payload;
            lira_msg_heartbeat_decode(&heartbeat_payload, message.payload);
            if (_heartbeat_callback) _heartbeat_callback(heartbeat_payload);
            break;
        }
        case LIRA_MSG_ID_GPS_RAW_INT: {
            lira_msg_gps_raw_int_t gps_payload;
            lira_msg_gps_raw_int_decode(&gps_payload, message.payload);
            if (_gps_callback) _gps_callback(gps_payload);
            break;
        }
        default:
            break;
    }
}

void LiraProtocolReceiver::register_heartbeat_callback(heartbeat_callback_t callback) {
    std::lock_guard<std::mutex> lock(_mutex);
    _heartbeat_callback = std::move(callback);
}

void LiraProtocolReceiver::register_gps_callback(gps_callback_t callback) {
    std::lock_guard<std::mutex> lock(_mutex);
    _gps_callback = std::move(callback);
}

}  // namespace lirasdk