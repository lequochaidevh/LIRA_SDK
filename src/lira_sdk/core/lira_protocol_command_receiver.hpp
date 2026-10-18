#pragma once

#include "lira_protocol_include.hpp"
#include <cmath>
#include <cstdint>
#include <string>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <variant>

namespace lirasdk {

// class ServerComponentImpl;

class LiraProtocolCommandReceiver {
 public:
    using TaskDispatcher = std::function<void(std::function<void()>)>;

    explicit LiraProtocolCommandReceiver(TaskDispatcher user_thread_dispatcher = nullptr);
    // explicit LiraProtocolCommandReceiver(ServerComponentImpl& server_component_impl);
    ~LiraProtocolCommandReceiver();

    struct CommandInt {
        uint8_t    target_system_id{0};
        uint8_t    target_component_id{0};
        uint8_t    origin_system_id{0};
        uint8_t    origin_component_id{0};
        LIRA_FRAME frame = LIRA_FRAME_GLOBAL_RELATIVE_ALT;
        uint16_t   command{0};
        bool       current{false};
        bool       autocontinue{false};
        // Most of the "Reserved" values in LiraProtocol spec are NAN.
        struct Params {
            float   param1 = NAN;
            float   param2 = NAN;
            float   param3 = NAN;
            float   param4 = NAN;
            int32_t x      = 0;
            int32_t y      = 0;
            float   z      = NAN;
        } params{};

        CommandInt(const lira_protocol_message_t& message) {
            lira_msg_command_int_t command_int;
            lira_msg_command_int_decode(&command_int, message.payload);

            target_system_id    = command_int.target_system;
            target_component_id = command_int.target_component;
            origin_system_id    = message.sysid;
            origin_component_id = message.compid;
            command             = command_int.command;
            frame               = static_cast<LIRA_FRAME>(command_int.frame);
            current             = command_int.current;
            autocontinue        = command_int.autocontinue;
            params.param1       = command_int.param1;
            params.param2       = command_int.param2;
            params.param3       = command_int.param3;
            params.param4       = command_int.param4;
            params.x            = command_int.x;
            params.y            = command_int.y;
            params.z            = command_int.z;
        }
    };

    struct CommandLong {
        uint8_t  target_system_id{0};
        uint8_t  target_component_id{0};
        uint8_t  origin_system_id{0};
        uint8_t  origin_component_id{0};
        uint16_t command{0};
        uint8_t  confirmation{0};
        struct Params {
            float param1 = NAN;
            float param2 = NAN;
            float param3 = NAN;
            float param4 = NAN;
            float param5 = NAN;
            float param6 = NAN;
            float param7 = NAN;
        } params{};

        CommandLong() = default;

        CommandLong(const lira_protocol_message_t& message) {
            lira_msg_command_long_t command_long;
            lira_msg_command_long_decode(&command_long, message.payload);

            target_system_id    = command_long.target_system;
            target_component_id = command_long.target_component;
            origin_system_id    = message.sysid;
            origin_component_id = message.compid;
            command             = command_long.command;
            confirmation        = command_long.confirmation;
            params.param1       = command_long.param1;
            params.param2       = command_long.param2;
            params.param3       = command_long.param3;
            params.param4       = command_long.param4;
            params.param5       = command_long.param5;
            params.param6       = command_long.param6;
            params.param7       = command_long.param7;
        }
    };

    using LiraProtocolCommandIntHandler  = std::function<std::optional<lira_msg_command_ack_t>(const CommandInt&)>;
    using LiraProtocolCommandLongHandler = std::function<std::optional<lira_msg_command_ack_t>(const CommandLong&)>;

    void register_lira_protocol_command_handler(uint16_t cmd_id, const LiraProtocolCommandIntHandler& callback,
                                                const void* cookie);
    void register_lira_protocol_command_handler(uint16_t cmd_id, const LiraProtocolCommandLongHandler& callback,
                                                const void* cookie);

    void unregister_lira_protocol_command_handler(uint16_t cmd_id, const void* cookie);
    void unregister_all_lira_protocol_command_handlers(const void* cookie);

 private:
    // ServerComponentImpl& _server_component_impl;

    void receive_command_int(const lira_protocol_message_t& message);
    void receive_command_long(const lira_protocol_message_t& message);

    struct LiraProtocolCommandIntHandlerTableEntry {
        uint16_t                      cmd_id;
        LiraProtocolCommandIntHandler callback;
        const void*                   cookie;  // This is the identification to unregister.
    };

    struct LiraProtocolCommandLongHandlerTableEntry {
        uint16_t                       cmd_id;
        LiraProtocolCommandLongHandler callback;
        const void*                    cookie;  // This is the identification to unregister.
    };

    std::mutex                                            _lira_protocol_command_handler_table_mutex{};
    std::vector<LiraProtocolCommandIntHandlerTableEntry>  _lira_protocol_command_int_handler_table{};
    std::vector<LiraProtocolCommandLongHandlerTableEntry> _lira_protocol_command_long_handler_table{};

    bool _debugging{false};

    // Dummy
    uint8_t                                                     _own_system_id{1};     // GCS
    uint8_t                                                     _own_component_id{1};  // GCS
    TaskDispatcher                                              _user_thread_dispatcher{nullptr};
    std::function<bool(const lira_protocol_message_t& message)> _send_raw_msg_fn;
    friend class LiraReceiverTestHelper;
};

}  // namespace lirasdk
