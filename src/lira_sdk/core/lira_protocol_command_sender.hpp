#pragma once

#include "timeout_handler.hpp"
#include "lira_protocol_include.hpp"
#include "lirasdk_time.hpp"
#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <asio/steady_timer.hpp>
#include <cmath>
#include <cstdint>
#include <deque>
#include <string>
#include <functional>
#include <mutex>
#include <optional>
#include <variant>

namespace lirasdk {

class LiraProtocolCommandSender {
 public:
    struct CommandLong {
        uint8_t  target_system_id{0};
        uint8_t  target_component_id{0};
        uint16_t command{0};
        uint8_t  confirmation = 0;
        struct Params {
            std::optional<float> maybe_param1{};
            std::optional<float> maybe_param2{};
            std::optional<float> maybe_param3{};
            std::optional<float> maybe_param4{};
            std::optional<float> maybe_param5{};
            std::optional<float> maybe_param6{};
            std::optional<float> maybe_param7{};
        } params{};
    };

    struct CommandInt {
        uint8_t    target_system_id{0};
        uint8_t    target_component_id{0};
        LIRA_FRAME frame = LIRA_FRAME_GLOBAL_RELATIVE_ALT;
        uint16_t   command{0};
        bool       current      = false;
        bool       autocontinue = false;
        // Most of the "Reserved" values in LIRA Protocol spec are NAN.
        struct Params {
            std::optional<float> maybe_param1{};
            std::optional<float> maybe_param2{};
            std::optional<float> maybe_param3{};
            std::optional<float> maybe_param4{};
            int32_t              x = 0;
            int32_t              y = 0;
            std::optional<float> maybe_z{};
        } params{};
    };
    // The std::monostate is required to work around the fact that
    // the default ctor of CommandLong and CommandInt is ill-defined.
    using Command = std::variant<std::monostate, CommandLong, CommandInt>;

    using TaskDispatcher = std::function<void(std::function<void()>)>;

    explicit LiraProtocolCommandSender(asio::io_context& io_ctx, TaskDispatcher user_thread_dispatcher = nullptr);
    // LiraProtocolCommandSender(asio::io_context& io_ctx, std::function<bool(const Command&)> send_msg_fn);
    ~LiraProtocolCommandSender();

    static constexpr unsigned DEFAULT_RETRIES = 3;

    enum class Result {
        Success = 0,
        NoSystem,
        ConnectionError,
        Busy,
        Denied,
        Unsupported,
        Timeout,
        InProgress,
        TemporarilyRejected,
        Failed,
        Cancelled,
        UnknownError
    };

    using CommandResultCallback = std::function<void(Result, float)>;

    // Blocking API
    Result send_command(const CommandInt& command, unsigned retries = DEFAULT_RETRIES);
    Result send_command(const CommandLong& command, unsigned retries = DEFAULT_RETRIES);

    // Non-blocking API
    void queue_command_async(const CommandInt& command, const CommandResultCallback& callback,
                             unsigned retries = DEFAULT_RETRIES);
    void queue_command_async(const CommandLong& command, const CommandResultCallback& callback,
                             unsigned retries = DEFAULT_RETRIES);

    void register_send_message_callback(std::function<bool(const lira_protocol_message_t& message)> cb);

    void do_work();

    static const int DEFAULT_COMPONENT_ID_AUTOBOT = LIRA_COMP_ID_AUTOBOT1;

    // Non-copyable
    LiraProtocolCommandSender(const LiraProtocolCommandSender&) = delete;
    const LiraProtocolCommandSender& operator=(const LiraProtocolCommandSender&) = delete;

 private:
    struct CommandIdentification {
        uint32_t maybe_param1{0};  // only for commands where this matters
        uint32_t maybe_param2{0};  // only for commands where this matters
        uint16_t command{0};
        uint8_t  target_system_id{0};
        uint8_t  target_component_id{0};

        bool operator==(const CommandIdentification& other) const {
            return maybe_param1 == other.maybe_param1 && maybe_param2 == other.maybe_param2 &&
                   command == other.command && target_system_id == other.target_system_id &&
                   target_component_id == other.target_component_id;
        }

        bool operator!=(const CommandIdentification& other) const { return !(*this == other); }
    };

    void receive_command_ack(const lira_protocol_message_t& message);

    struct Work {
        Command                             command;
        CommandIdentification               identification{};
        CommandResultCallback               callback{};
        SteadyTimePoint                     time_started{};
        std::shared_ptr<asio::steady_timer> timeout_timer;
        TimeoutHandler::Cookie              timeout_cookie{};
        double                              timeout_s{0.5};
        int                                 retries_to_do;
        bool                                already_sent{false};
    };

    template <typename CommandType>
    CommandIdentification identification_from_command(const CommandType& command) {
        CommandIdentification identification{};

        identification.command = command.command;
        if (command.command == LIRA_CMD_REQUEST_MESSAGE || command.command == LIRA_CMD_SET_MESSAGE_INTERVAL) {
            if (command.params.maybe_param1) {
                const uint32_t param1       = static_cast<uint32_t>(std::lround(command.params.maybe_param1.value()));
                identification.maybe_param1 = param1;
                if (param1 == LIRA_MSG_ID_CAM_IMG_CAPTURE) {
                    if (command.params.maybe_param2) {
                        identification.maybe_param2 =
                            static_cast<uint32_t>(std::lround(command.params.maybe_param2.value()));
                    }
                }
            }
        }
        identification.target_system_id    = command.target_system_id;
        identification.target_component_id = command.target_component_id;

        return identification;
    }

    // void receive_command_ack(const lira_protocol_message_t& message);
    void receive_timeout(const CommandIdentification& identification);

    void call_callback(const CommandResultCallback& callback, Result result, float progress) const;

    bool send_lira_protocol_message(const Command& command) const;

    float maybe_reserved(const std::optional<float>& maybe_param) const;

    // SystemImpl&                       _system_impl;
    std::function<bool(const Command&)> _send_msg_fn;

    std::deque<std::shared_ptr<Work>> _work_queue{};
    asio::io_context&                 _io_context;

    bool _command_debugging{false};

    // Dummy
    uint8_t                                                     _own_system_id{1};     // GCS
    uint8_t                                                     _own_component_id{1};  // GCS
    TaskDispatcher                                              _user_thread_dispatcher{nullptr};
    std::function<bool(const lira_protocol_message_t& message)> _send_raw_msg_fn;
};

}  // namespace lirasdk
