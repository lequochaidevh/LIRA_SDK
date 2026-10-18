#include "lira_protocol_command_receiver.hpp"
#include "lirasdk_impl.hpp"
#include "log.hpp"
#include <cmath>
#include <future>
#include <memory>

namespace lirasdk {

// New decoupled constructor using dependency injection via a registration callback
LiraProtocolCommandReceiver::LiraProtocolCommandReceiver(TaskDispatcher user_thread_dispatcher)
    : _user_thread_dispatcher(user_thread_dispatcher) {
    //  Setup debugging state from environment variables
    if (const char* env_p = std::getenv("LIRASDK_COMMAND_DEBUGGING")) {
        if (std::string(env_p) == "1") {
            // LogDebug("Command debugging is on.");
            _debugging = true;
        }
    }
}

LiraProtocolCommandReceiver::~LiraProtocolCommandReceiver() {
    // _server_component_impl.unregister_all_lira_protocol_command_handlers(this);
    // Blocking, so that no message can be dispatched into us while we are destroyed.
    // _server_component_impl.unregister_all_lira_protocol_message_handlers_blocking(this);
}

void LiraProtocolCommandReceiver::receive_command_int(const lira_protocol_message_t& message) {
    LiraProtocolCommandReceiver::CommandInt cmd(message);

    if (_debugging) {
        LogDebug("Received command int {}", cmd.command);
    }

    if (cmd.target_component_id != _own_system_id && cmd.target_component_id != LIRA_COMP_ID_ALL) {
        if (_debugging) {
            LogDebug("Ignored command int to component {} instead of {}", cmd.target_component_id, _own_component_id);
        }
        return;
    }

    std::lock_guard<std::mutex> lock(_lira_protocol_command_handler_table_mutex);

    for (auto& handler : _lira_protocol_command_int_handler_table) {
        if (handler.cmd_id == cmd.command) {
            if (_debugging) {
                LogDebug("Handling command int {}", cmd.command);
            }

            // The client side can pack a COMMAND_ACK as a response to receiving the command.
            auto maybe_command_ack = handler.callback(cmd);
            if (maybe_command_ack && _send_raw_msg_fn) {
                lira_protocol_message_t response_message;
                lira_msg_command_ack_t  ack = maybe_command_ack.value();

                // Call the fast-packing C macro using flattened structural attributes
                lira_protocol_msg_command_ack_pack_chan(
                    _own_system_id,     // Source system identifier
                    _own_component_id,  // Source component identifier
                    0,                  // Target logical communication channel stream profile
                    &response_message,  // Output pointer capturing the raw formatted hardware wire block
                    ack.command,        // Echoed execution task identifier being replied
                    ack.result,         // Operational response outcome state (Success/Busy/Denied)
                    ack.progress,       // Command completion percentage evaluation status
                    ack.result_param2,  // Auxiliary data parameters returned from micro-controllers
                    message.sysid,      // Target recipient system destination (Reroute back to sender)
                    message.compid      // Target recipient component destination
                );

                _send_raw_msg_fn(response_message);

                if (_debugging) {
                    LogDebug("Acked command int {} with {}", cmd.command, maybe_command_ack.value().result);
                }
            }
        }
    }
}

void LiraProtocolCommandReceiver::receive_command_long(const lira_protocol_message_t& message) {
    LiraProtocolCommandReceiver::CommandLong cmd(message);

    if (_debugging) {
        LogDebug("Received command long {}", cmd.command);
    }

    if (cmd.target_component_id != _own_system_id && cmd.target_component_id != LIRA_COMP_ID_ALL) {
        if (_debugging) {
            LogDebug("Ignored command long to component {} instead of {}", cmd.target_component_id, _own_component_id);
        }
        return;
    }

    std::lock_guard<std::mutex> lock(_lira_protocol_command_handler_table_mutex);

    for (auto& handler : _lira_protocol_command_long_handler_table) {
        if (handler.cmd_id == cmd.command) {
            if (_debugging) {
                LogDebug("Handling command long {}", cmd.command);
            }

            // The client side can pack a COMMAND_ACK as a response to receiving the command.
            auto maybe_command_ack = handler.callback(cmd);
            if (maybe_command_ack && _send_raw_msg_fn) {
                lira_protocol_message_t response_message;
                lira_msg_command_ack_t  ack = maybe_command_ack.value();

                // Call the fast-packing C macro using flattened structural attributes
                lira_protocol_msg_command_ack_pack_chan(
                    _own_system_id,     // Source system identifier
                    _own_component_id,  // Source component identifier
                    0,                  // Target logical communication channel stream profile
                    &response_message,  // Output pointer capturing the raw formatted hardware wire block
                    ack.command,        // Echoed execution task identifier being replied
                    ack.result,         // Operational response outcome state (Success/Busy/Denied)
                    ack.progress,       // Command completion percentage evaluation status
                    ack.result_param2,  // Auxiliary data parameters returned from micro-controllers
                    message.sysid,      // Target recipient system destination (Reroute back to sender)
                    message.compid      // Target recipient component destination
                );

                _send_raw_msg_fn(response_message);

                if (_debugging) {
                    LogDebug("Acked command long {} with {}", cmd.command, maybe_command_ack.value().result);
                }
            }
        }
    }
}

void LiraProtocolCommandReceiver::register_lira_protocol_command_handler(uint16_t                             cmd_id,
                                                                         const LiraProtocolCommandIntHandler& callback,
                                                                         const void*                          cookie) {
    std::lock_guard<std::mutex> lock(_lira_protocol_command_handler_table_mutex);

    LiraProtocolCommandIntHandlerTableEntry entry = {cmd_id, callback, cookie};
    _lira_protocol_command_int_handler_table.push_back(entry);
}

void LiraProtocolCommandReceiver::register_lira_protocol_command_handler(uint16_t                              cmd_id,
                                                                         const LiraProtocolCommandLongHandler& callback,
                                                                         const void*                           cookie) {
    std::lock_guard<std::mutex> lock(_lira_protocol_command_handler_table_mutex);

    LiraProtocolCommandLongHandlerTableEntry entry = {cmd_id, callback, cookie};
    _lira_protocol_command_long_handler_table.push_back(entry);
}

void LiraProtocolCommandReceiver::unregister_lira_protocol_command_handler(uint16_t cmd_id, const void* cookie) {
    std::lock_guard<std::mutex> lock(_lira_protocol_command_handler_table_mutex);

    // COMMAND_INT
    for (auto it = _lira_protocol_command_int_handler_table.begin();
         it != _lira_protocol_command_int_handler_table.end();
         /* no ++it */) {
        if (it->cmd_id == cmd_id && it->cookie == cookie) {
            it = _lira_protocol_command_int_handler_table.erase(it);
        } else {
            ++it;
        }
    }

    // COMMAND_LONG
    for (auto it = _lira_protocol_command_long_handler_table.begin();
         it != _lira_protocol_command_long_handler_table.end();
         /* no ++it */) {
        if (it->cmd_id == cmd_id && it->cookie == cookie) {
            it = _lira_protocol_command_long_handler_table.erase(it);
        } else {
            ++it;
        }
    }
}

void LiraProtocolCommandReceiver::unregister_all_lira_protocol_command_handlers(const void* cookie) {
    std::lock_guard<std::mutex> lock(_lira_protocol_command_handler_table_mutex);

    // COMMAND_INT
    for (auto it = _lira_protocol_command_int_handler_table.begin();
         it != _lira_protocol_command_int_handler_table.end();
         /* no ++it */) {
        if (it->cookie == cookie) {
            it = _lira_protocol_command_int_handler_table.erase(it);
        } else {
            ++it;
        }
    }

    // COMMAND_LONG
    for (auto it = _lira_protocol_command_long_handler_table.begin();
         it != _lira_protocol_command_long_handler_table.end();
         /* no ++it */) {
        if (it->cookie == cookie) {
            it = _lira_protocol_command_long_handler_table.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace lirasdk
