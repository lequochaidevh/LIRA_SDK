#include "lira_protocol_receiver.hpp"

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
    if (message.msgid >= LIRA_MAX_MESSAGE_ID) {
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    const auto&                 wrappers = _callbacks[message.msgid];
    for (const auto& wrapper : wrappers) {
        if (wrapper->execute(message.payload)) return;  // Callback list shutdown soon
        // Todo make enum result
    }
}

}  // namespace lirasdk