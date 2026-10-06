#pragma once

#include "lira_protocol_include.hpp"
#include "lirasdk.hpp"

#include <string>
#include <cstdint>
#include <optional>
#include <memory>
#include <vector>

// Forward declarations to avoid including MessageSet.h in header
namespace lira {
using BufferParser = lira_status_t;  // Your parser state tracking struct

enum class MessageResult { Success, FieldError, UnknownMessage };

class Message {
 public:
    // Nested Header Class to satisfy: message.header().systemId()
    class Header {
     public:
        Header(uint8_t sysid, uint8_t compid) : _sysid(sysid), _compid(compid) {}
        [[nodiscard]] uint8_t systemId() const { return _sysid; }
        [[nodiscard]] uint8_t componentId() const { return _compid; }

     private:
        uint8_t _sysid;
        uint8_t _compid;
    };

    // Default constructor and constructor wrapping our pure C frame layout
    Message() = default;
    explicit Message(uint32_t msgid, uint8_t sysid, uint8_t compid, const uint8_t* payload_data, uint8_t len)
        : _msgid(msgid), _sysid(sysid), _compid(compid) {
        if (len > 0 && payload_data != nullptr) {
            _payload.assign(payload_data, payload_data + len);
        }
    }

    // Satisfies: message.id()
    [[nodiscard]] uint32_t id() const { return _msgid; }

    // Satisfies: message.header()
    [[nodiscard]] Header header() const { return Header(_sysid, _compid); }

    // Satisfies: message.name()
    [[nodiscard]] std::string name() const {
        switch (_msgid) {
            case 1:
                return "HEARTBEAT";
            case 33:
                return "GPS_RAW_INT";
            default:
                return "UNKNOWN_MESSAGE";
        }
    }

    // Satisfies: message.finalizedSize()
    [[nodiscard]] uint32_t finalizedSize() const { return static_cast<uint32_t>(_payload.size()); }

    // Satisfies: message.data()
    [[nodiscard]] const uint8_t* data() const { return _payload.data(); }

    /**
     * @brief Satisfies: message.get("target_system", target_system_id)
     */
    MessageResult get(const std::string& field_name, uint8_t& value) const {
        if (_payload.empty()) {
            return MessageResult::FieldError;
        }

        if (field_name == "target_system") {
            value = _payload[0];
            return MessageResult::Success;
        } else if (field_name == "target_component") {
            value = (_payload.size() > 1) ? _payload[1] : 0;
            return MessageResult::Success;
        }

        return MessageResult::FieldError;
    }

 private:
    uint32_t             _msgid{0};
    uint8_t              _sysid{0};
    uint8_t              _compid{0};
    std::vector<uint8_t> _payload{};
};

}  // namespace lira

// Mirror it over to your main SDK integration wrapper definitions namespace alias map
namespace Lirasdk {
using LiraProtocolMessage = lira::Message;
}

namespace Json {
class Value;
}

namespace lirasdk {

// Forward declaration for thread-safe MessageSet operations
class LirasdkImpl;

class LiraDistributingReceiver {
 public:
    explicit LiraDistributingReceiver(LirasdkImpl& lirasdk_impl);
    ~LiraDistributingReceiver() = default;  // Need explicit destructor for unique_ptr with incomplete type

    const Lirasdk::LiraProtocolMessage& get_last_message() const { return _last_message; }
    const std::optional<lira::Message>& get_last_lira_distributing_message() const {
        return _last_lira_distributing_message;
    }

    void set_new_datagram(char* datagram, unsigned datagram_len);

    bool parse_message();

    // Message creation for sending
    std::optional<lira::Message> create_message(const std::string& message_name) const;

    // Helper methods for message ID/name conversion
    std::optional<std::string> message_id_to_name(uint32_t id) const;
    std::optional<int>         message_name_to_id(const std::string& name) const;

    // Load custom XML message definitions
    bool load_custom_xml(const std::string& xml_content);

    // JSON conversion (made public for use in message interception)
    std::string lira_distributing_message_to_json(const lira::Message& msg) const;

 private:
    LirasdkImpl&                                _lirasdk_impl;  // For thread-safe MessageSet access
    mutable std::unique_ptr<lira::BufferParser> _buffer_parser;
    Lirasdk::LiraProtocolMessage                _last_message;
    std::optional<lira::Message> _last_lira_distributing_message;  // Separate lira_distributing message for integration

    // Accumulation buffer for connections where messages can span multiple reads.
    //
    // This bound is only a safety valve against unbounded growth; the parser normally
    // drains the buffer (it consumes complete messages and skips garbage before the magic
    // byte), so at most one partial message is ever pending. It must therefore be at least
    // as large as a single read (the 2048-byte connection receive buffers) plus one full
    // message, otherwise set_new_datagram() would drop the front of a large read *before*
    // parsing and silently lose the messages in it.
    static constexpr size_t ACCUMULATION_BUFFER_SIZE = 2048 + 280;  // 280 is the max frame length of LiraV2
    std::vector<uint8_t>    _accumulation_buffer;

    bool _debugging = false;

    // Helper methods
    bool parse_message_from_buffer();
};

}  // namespace lirasdk
