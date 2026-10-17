#include "lira_protocol_command_sender.hpp"
#include "lira_protocol_address.hpp"
#include "log.hpp"
// #include "system_impl.hpp"
#include "unused.hpp"
#include <cmath>
#include <future>
#include <memory>
namespace lirasdk {

LiraProtocolCommandSender::LiraProtocolCommandSender(asio::io_context& io_ctx, TaskDispatcher user_thread_dispatcher)
    : _io_context(io_ctx), _user_thread_dispatcher(user_thread_dispatcher) {
    if (const char* env_p = std::getenv("LIRASDK_COMMAND_DEBUGGING")) {
        if (std::string(env_p) == "1") {
            std::cout << "[CmdSender] Command debugging is on.\n";
            _command_debugging = true;
        }
    }
    //  TODO SysImpl day8
}

LiraProtocolCommandSender::~LiraProtocolCommandSender() {
    if (_command_debugging) {
        LogDebug("CommandSender destroyed");
    }
    // Blocking, so that nothing can be dispatched into us while we are being destroyed.
    for (const auto& work : _work_queue) {  // _system_impl
        if (work && work->timeout_timer) {
            asio::error_code ec;
            work->timeout_timer->cancel(ec);
        }
    }
}

LiraProtocolCommandSender::Result LiraProtocolCommandSender::send_command(
    const LiraProtocolCommandSender::CommandInt& command, unsigned retries) {
    // We wrap the async call with a promise and future.
    auto prom = std::make_shared<std::promise<Result>>();
    auto res  = prom->get_future();

    queue_command_async(
        command,
        [prom](Result result, float progress) {
            UNUSED(progress);
            // We can only fulfill the promise once in C++11.
            // Therefore, we have to ignore the IN_PROGRESS state and wait
            // for the final result.
            if (result != Result::InProgress) {
                prom->set_value(result);
            }
        },
        retries);

    // Block now to wait for result.
    return res.get();
}

LiraProtocolCommandSender::Result LiraProtocolCommandSender::send_command(
    const LiraProtocolCommandSender::CommandLong& command, unsigned retries) {
    // We wrap the async call with a promise and future.
    auto prom = std::make_shared<std::promise<Result>>();
    auto res  = prom->get_future();

    queue_command_async(
        command,
        [prom](Result result, float progress) {
            UNUSED(progress);
            // We can only fulfill the promise once in C++11.
            // Therefore, we have to ignore the IN_PROGRESS state and wait
            // for the final result.
            if (result != Result::InProgress) {
                prom->set_value(result);
            }
        },
        retries);

    return res.get();
}

void LiraProtocolCommandSender::queue_command_async(const CommandInt& command, const CommandResultCallback& callback,
                                                    unsigned retries) {
    if (_command_debugging) {
        LogDebug("COMMAND_INT {} to send to {}, {}", static_cast<int>(command.command),
                 static_cast<int>(command.target_system_id), static_cast<int>(command.target_component_id));
    }

    auto new_work            = std::make_shared<Work>();
    new_work->timeout_s      = 0.5;  // _system_impl.timeout_s();
    new_work->command        = command;
    new_work->identification = identification_from_command(command);
    new_work->callback       = callback;
    new_work->retries_to_do  = retries;
    asio::post(_io_context, [this, new_work]() {
        for (const auto& work : _work_queue) {
            if (work->identification == new_work->identification && new_work->callback == nullptr) {
                if (_command_debugging) {
                    LogDebug("Dropping command {} that is already being sent",
                             static_cast<int>(new_work->identification.command));
                }
                return;
            }
        }
        _work_queue.push_back(new_work);
        // Unconditional, unlike the other work queues: do_work() here walks the whole queue
        // and sends everything not sent yet, so a command queued behind another one still
        // needs it to run.
        do_work();
    });
}

void LiraProtocolCommandSender::queue_command_async(const CommandLong& command, const CommandResultCallback& callback,
                                                    unsigned retries) {
    if (_command_debugging) {
        LogDebug("COMMAND_LONG {} to send to {}, {}", static_cast<int>(command.command),
                 static_cast<int>(command.target_system_id), static_cast<int>(command.target_component_id));
    }

    auto new_work            = std::make_shared<Work>();
    new_work->timeout_s      = 0.5;  // _system_impl.timeout_s();
    new_work->command        = command;
    new_work->identification = identification_from_command(command);
    new_work->callback       = callback;
    new_work->time_started   = std::chrono::steady_clock::now();  // _system_impl.get_time().steady_time();
    new_work->retries_to_do  = retries;
    asio::post(_io_context, [this, new_work]() {
        for (const auto& work : _work_queue) {
            if (work->identification == new_work->identification && new_work->callback == nullptr) {
                if (_command_debugging) {
                    LogDebug("Dropping command {} that is already being sent",
                             static_cast<int>(new_work->identification.command));
                }
                return;
            }
        }
        _work_queue.push_back(new_work);
        // Unconditional, unlike the other work queues: do_work() here walks the whole queue
        // and sends everything not sent yet, so a command queued behind another one still
        // needs it to run.
        do_work();
    });
}

void LiraProtocolCommandSender::receive_command_ack(const lira_protocol_message_t& message) {
    lira_msg_command_ack_t command_ack;
    lira_msg_command_ack_decode(&command_ack, message.payload);

    if ((command_ack.target_system && command_ack.target_system != _own_system_id) ||
        (command_ack.target_component && command_ack.target_component != _own_component_id)) {
        if (_command_debugging) {
            LogDebug("Ignoring command ack for command {} from {}{}{} to {}{}{}", static_cast<int>(command_ack.command),
                     static_cast<int>(message.sysid), '/', static_cast<int>(message.compid),
                     static_cast<int>(command_ack.target_system), '/', static_cast<int>(command_ack.target_component));
        }
        return;
    }

    for (auto it = _work_queue.begin(); it != _work_queue.end(); ++it) {
        const auto& work = *it;

        if (!work) {
            LogErr("No work available! (should not happen #1)");
            return;
        }

        if (work->identification.command != command_ack.command ||
            (work->identification.target_system_id != 0 && work->identification.target_system_id != message.sysid) ||
            (work->identification.target_component_id != 0 &&
             work->identification.target_component_id != message.compid)) {
            if (_command_debugging) {
                auto elapsed_s =
                    std::chrono::duration<double>(std::chrono::steady_clock::now() - work->time_started).count();
                LogDebug("Command ack for {} (from: {}/{}) does not match command {} (to: {}/{}) after {} s",
                         command_ack.command, message.sysid, message.compid, work->identification.command,
                         work->identification.target_system_id, work->identification.target_component_id,
                         static_cast<int>(elapsed_s));
            }
            continue;
        }

        if (_command_debugging) {
            auto elapsed_s =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - work->time_started).count();
            LogDebug("Received command ack for {} with result {} after {} s", command_ack.command,
                     static_cast<int>(command_ack.result), static_cast<int>(elapsed_s));
        }

        CommandResultCallback    temp_callback = work->callback;
        std::pair<Result, float> temp_result{Result::UnknownError, NAN};

        switch (command_ack.result) {
            case LIRA_RESULT_ACCEPTED:
                if (work->timeout_timer) {
                    asio::error_code ec;
                    work->timeout_timer->cancel(ec);
                }

                temp_result = {Result::Success, 1.0f};
                _work_queue.erase(it);
                break;

            case LIRA_RESULT_DENIED:
                if (_command_debugging) {
                    LogDebug("Command denied ({}).", work->identification.command);
                    if (work->identification.command == 512) {
                        LogDebug("(message {})", work->identification.maybe_param1);
                    }
                }
                if (work->timeout_timer) {
                    asio::error_code ec;
                    work->timeout_timer->cancel(ec);
                }
                temp_result = {Result::Denied, NAN};
                _work_queue.erase(it);
                break;

            case LIRA_RESULT_UNSUPPORTED:
                if (_command_debugging) {
                    LogDebug("Command unsupported ({}).", work->identification.command);
                }
                if (work->timeout_timer) {
                    asio::error_code ec;
                    work->timeout_timer->cancel(ec);
                }

                temp_result = {Result::Unsupported, NAN};
                _work_queue.erase(it);
                break;

            case LIRA_RESULT_TEMPORARILY_REJECTED:
                if (_command_debugging) {
                    LogDebug("Command temporarily rejected ({}).", work->identification.command);
                }
                if (work->timeout_timer) {
                    asio::error_code ec;
                    work->timeout_timer->cancel(ec);
                }

                temp_result = {Result::TemporarilyRejected, NAN};
                _work_queue.erase(it);
                break;

            case LIRA_RESULT_FAILED:
                if (_command_debugging) {
                    LogDebug("Command failed ({}).", work->identification.command);
                }
                if (work->timeout_timer) {
                    asio::error_code ec;
                    work->timeout_timer->cancel(ec);
                }

                temp_result = {Result::Failed, NAN};
                _work_queue.erase(it);
                break;

            case LIRA_RESULT_IN_PROGRESS:
                if (_command_debugging) {
                    if (static_cast<int>(command_ack.progress) != 255) {
                        LogDebug("progress: {} % ({}).", static_cast<int>(command_ack.progress),
                                 work->identification.command);
                    }
                }
                // If we get a progress update, we can raise the timeout
                // to something higher because we know the initial command
                // has arrived.
                if (work->timeout_timer) {
                    work->timeout_timer->expires_after(std::chrono::seconds(3));
                    work->timeout_timer->async_wait([this, ident = work->identification](const asio::error_code& ec) {
                        if (!ec) {
                            this->receive_timeout(ident);
                        }
                    });
                }

                temp_result = {Result::InProgress, static_cast<float>(command_ack.progress) / 100.0f};
                break;

            case LIRA_RESULT_CANCELLED:
                if (_command_debugging) {
                    LogDebug("Command cancelled ({}).", work->identification.command);
                }
                if (work->timeout_timer) {
                    asio::error_code ec;
                    work->timeout_timer->cancel(ec);
                }
                temp_result = {Result::Cancelled, NAN};
                _work_queue.erase(it);
                break;

            default:
                LogWarn("Received unknown ack.");
                break;
        }

        if (temp_callback != nullptr) {
            call_callback(temp_callback, temp_result.first, temp_result.second);
        }

        // This command may have been blocking another queued one with the same command id,
        // which do_work() skips while the first is in flight.
        asio::post(_io_context, [this] { do_work(); });

        return;
    }

    if (_command_debugging) {
        LogDebug("Received ack from {}{}{} for not-existing command: {}! Ignoring...", static_cast<int>(message.sysid),
                 '/', static_cast<int>(message.compid), static_cast<int>(command_ack.command));
    } else {
        LogWarn("Received ack for not-existing command: {}! Ignoring...", static_cast<int>(command_ack.command));
    }
}

void LiraProtocolCommandSender::receive_timeout(const CommandIdentification& identification) {
    if (_command_debugging) {
        LogDebug("Got timeout!");
    }
    bool                     found_command = false;
    CommandResultCallback    temp_callback = nullptr;
    std::pair<Result, float> temp_result{Result::UnknownError, NAN};

    for (auto it = _work_queue.begin(); it != _work_queue.end(); ++it) {
        const auto& work = *it;

        if (!work) {
            LogErr("No work available! (should not happen #2)");
            return;
        }

        if (work->identification != identification) {
            continue;
        }

        found_command = true;

        if (work->retries_to_do > 0) {
            // We're not sure the command arrived, let's retransmit.
            if (_command_debugging) {
                auto elapsed_s =
                    std::chrono::duration<double>(std::chrono::steady_clock::now() - work->time_started).count();
                LogWarn("sending again after {} s, retries to do: {}  ({}).", static_cast<int>(elapsed_s),
                        work->retries_to_do, work->identification.command);

                if (work->identification.command == LIRA_CMD_REQUEST_MESSAGE) {
                    LogWarn("Request was for msg ID: {}", work->identification.maybe_param1);
                }
            }

            if (!send_lira_protocol_message(work->command)) {
                LogErr("Connection send error in retransmit ({}).", work->identification.command);
                temp_callback = work->callback;
                temp_result   = {Result::ConnectionError, NAN};
                _work_queue.erase(it);
                break;
            } else {
                --work->retries_to_do;
                if (work->timeout_timer) {
                    auto timeout_duration = std::chrono::milliseconds(static_cast<int64_t>(work->timeout_s * 1000.0));
                    work->timeout_timer->expires_after(timeout_duration);

                    work->timeout_timer->async_wait([this, ident = work->identification](const asio::error_code& ec) {
                        if (!ec) {
                            this->receive_timeout(ident);  // Loop back for the next timeout check
                        }
                    });
                }
            }
        } else {
            // We have tried retransmitting, giving up now.
            if (work->identification.command == 512) {
                uint8_t target_sysid;
                uint8_t target_compid;
                if (auto command_int = std::get_if<CommandInt>(&work->command)) {
                    target_sysid  = command_int->target_system_id;
                    target_compid = command_int->target_component_id;
                } else if (auto command_long = std::get_if<CommandLong>(&work->command)) {
                    target_sysid  = command_long->target_system_id;
                    target_compid = command_long->target_component_id;
                } else {
                    LogErr("No command, that's awkward");
                    continue;
                }
                if (_command_debugging) {
                    LogWarn("Retrying failed for REQUEST_MESSAGE command for message: {}, to ({}/{})",
                            work->identification.maybe_param1, target_sysid, target_compid);
                }
            } else {
                if (_command_debugging) {
                    LogWarn("Retrying failed for command: {})", work->identification.command);
                }
            }

            temp_callback = work->callback;
            temp_result   = {Result::Timeout, NAN};
            _work_queue.erase(it);
            break;
        }
    }

    if (temp_callback != nullptr) {
        call_callback(temp_callback, temp_result.first, temp_result.second);
    }

    // Same as in receive_command_ack(): giving up on a command can unblock another queued
    // one with the same command id.
    asio::post(_io_context, [this] { do_work(); });

    if (!found_command) {
        LogWarn("Timeout for not-existing command: {}! Ignoring...", static_cast<int>(identification.command));
    }
}

void LiraProtocolCommandSender::do_work() {
    for (const auto& work : _work_queue) {
        if (work->already_sent) {
            continue;
        }

        bool already_being_sent = false;
        for (const auto& other_work : _work_queue) {
            // Ignore itself:
            if (other_work == work) {
                continue;
            }

            // Check if command with same command ID is already being sent.
            if (other_work->already_sent && other_work->identification.command == work->identification.command) {
                if (_command_debugging) {
                    LogDebug("Command {} is already being sent, waiting...",
                             static_cast<int>(work->identification.command));
                }
                already_being_sent = true;
                break;
            }
        }

        if (already_being_sent) {
            continue;
        }

        // LogDebug() << "sending it the first time (" << work->lira_protocol_command << ")";
        work->time_started = std::chrono::steady_clock::now();

        {
            if (!send_lira_protocol_message(work->command)) {
                LogErr("Connection send error ({})", work->identification.command);
                // In this case we try again after the timeout. Chances are slim it will work next
                // time though.
            } else {
                if (_command_debugging) {
                    LogDebug("Sent command {}", static_cast<int>(work->identification.command));
                }
            }
        }

        work->already_sent = true;

        if (work->timeout_timer) {
            auto timeout_duration = std::chrono::milliseconds(static_cast<int64_t>(work->timeout_s * 1000.0));
            work->timeout_timer->expires_after(timeout_duration);

            // Capture the precise identification payload struct copy by value to decouple lifecycles
            work->timeout_timer->async_wait([this, ident = work->identification](const asio::error_code& ec) {
                if (!ec) {
                    this->receive_timeout(ident);
                }
            });
        }
    }
}

void LiraProtocolCommandSender::call_callback(const CommandResultCallback& callback, Result result,
                                              float progress) const {
    if (!callback) {
        return;
    }

    // It seems that we need to queue the callback on the thread pool otherwise
    // we lock ourselves out when we send a command in the callback receiving a command result.
    // auto temp_callback = callback;
    // _system_impl.call_user_callback([temp_callback, result, progress]() { temp_callback(result, progress); });

    if (callback == nullptr) return;
    if (_user_thread_dispatcher) {
        _user_thread_dispatcher([callback, result, progress]() { callback(result, progress); });
    } else {
        callback(result, progress);
    }
}

bool LiraProtocolCommandSender::send_lira_protocol_message(const Command& command) const {
    // 1. Establish localized fallback identities for routing metadata
    // These cleanly eliminate the dependency on '_system_impl.queue_message' address parameters
    const uint8_t own_sys_id     = _own_system_id;
    const uint8_t own_comp_id    = _own_component_id;
    const uint8_t target_channel = 0;  // Default logical stream transmission channel

    lira_protocol_message_t message{};

    // 2. Safely inspect and unpack the C++ Variant using std::get_if pointers
    if (auto command_int = std::get_if<CommandInt>(&command)) {
        // Execute the auto-generated fast-packing C API from your Day 3 pipeline
        lira_protocol_msg_command_int_pack_chan(
            own_sys_id, own_comp_id, target_channel, &message, command_int->target_system_id,
            command_int->target_component_id,
            static_cast<uint8_t>(command_int->frame),  // Safely cast your custom enum
            command_int->command, command_int->current, command_int->autocontinue,
            maybe_reserved(command_int->params.maybe_param1), maybe_reserved(command_int->params.maybe_param2),
            maybe_reserved(command_int->params.maybe_param3), maybe_reserved(command_int->params.maybe_param4),
            command_int->params.x, command_int->params.y, maybe_reserved(command_int->params.maybe_z));

        // 3. Dispatch the compiled packet directly down to the connection wire callback
        return _send_raw_msg_fn(message);

    } else if (auto command_long = std::get_if<CommandLong>(&command)) {
        // Corresponding auto-generated fast-packing macro for CommandLong layouts
        lira_protocol_msg_command_long_pack_chan(
            own_sys_id, own_comp_id, target_channel, &message, command_long->target_system_id,
            command_long->target_component_id, command_long->command, command_long->confirmation,
            maybe_reserved(command_long->params.maybe_param1), maybe_reserved(command_long->params.maybe_param2),
            maybe_reserved(command_long->params.maybe_param3), maybe_reserved(command_long->params.maybe_param4),
            maybe_reserved(command_long->params.maybe_param5), maybe_reserved(command_long->params.maybe_param6),
            maybe_reserved(command_long->params.maybe_param7));

        // Dispatch the compiled packet directly down to the connection wire callback
        return _send_raw_msg_fn(message);
    }

    return false;  // Error: Empty variant state detected (std::monostate)
}
float LiraProtocolCommandSender::maybe_reserved(const std::optional<float>& maybe_param) const {
    if (maybe_param) {
        return maybe_param.value();
    } else {
        return NAN;
    }
}

void LiraProtocolCommandSender::register_send_message_callback(
    std::function<bool(const lira_protocol_message_t& message)> cb) {
    _send_raw_msg_fn = std::move(cb);
}

}  // namespace lirasdk
