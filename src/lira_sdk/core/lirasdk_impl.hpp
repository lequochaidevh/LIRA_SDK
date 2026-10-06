#pragma once

#include <mutex>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <optional>
#include <string>

#include "connection.hpp"
#include "timeout_handler.hpp"
#include "call_every_handler.hpp"
#include "handle_factory.hpp"

#include "lira_protocol_receiver.hpp"

#include "lirasdk.hpp"

// Define Standalone ASIO infrastructure
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#include <asio.hpp>

namespace lirasdk {

class LirasdkImpl {
 public:
    LirasdkImpl();

    ~LirasdkImpl();

    // Thread Safety Isolations
    LirasdkImpl(const LirasdkImpl&) = delete;
    LirasdkImpl& operator=(const LirasdkImpl&) = delete;

    /**
     * @brief Expose the core event loop context
     */
    [[nodiscard]] asio::io_context& io_context() { return _io_context; }

    void receive_message(LiraProtocolReceiver::ParseResult result, lira_protocol_message_t& message,
                         Connection* connection);

    void receive_lira_distributing_message(const Lirasdk::LiraProtocolMessage& message, Connection* connection);

    Lirasdk::ConnectionHandle add_connection(std::unique_ptr<Connection>&& connection);

    void remove_connection(Lirasdk::ConnectionHandle handle);

    std::optional<lira::Message> parse_message_safe(const uint8_t* buffer, size_t buffer_len,
                                                    size_t& bytes_consumed) const;
    /**
     * @brief Check if the current execution is happening on the background IO thread
     */
    [[nodiscard]] bool on_io_thread() const;

    /**
     * @brief Handle connection error reports from transport layers
     */
    void report_connection_error(const std::string& error_msg, Lirasdk::ConnectionHandle handle);

    /**
     * @brief Receive raw bytes directly from the network Connection classes (UDP/Serial).
     * This is the bridge method called by Connection::receive_bytes().
     */
    //  void receive_bytes(uint8_t* bytes, unsigned int nbytes);

    /**
     * @brief Internal accessors to shared system utilities
     */
    //  [[nodiscard]] TimeoutHandler&   timeout_handler() { return _timeout_handler; }
    //  [[nodiscard]] CallEveryHandler& call_every_handler() { return _call_every_handler; }

 private:
    void process_message(lira_protocol_message_t& message, Connection* connection);

    // Temp: gen id HandleFactory
    uint64_t _next_connection_handle{1};

    // Core Event Loop Utilities
    // Time             _time{};
    // TimeoutHandler   _timeout_handler;
    // CallEveryHandler _call_every_handler;

    // Active network connection instances container
    mutable std::recursive_mutex _mutex{};
    HandleFactory<>              _connections_handle_factory{};
    struct ConnectionEntry {
        std::unique_ptr<Connection> connection;
        Lirasdk::ConnectionHandle   handle;
    };
    std::vector<ConnectionEntry> _connections{};

    // ASIO Infrastructure
    asio::io_context                                           _io_context{};
    asio::executor_work_guard<asio::io_context::executor_type> _io_work_guard;
    std::thread                                                _io_thread{};
    std::thread::id                                            _io_thread_id{};

    // Message set for libmav message handling (shared across all connections)
    std::unique_ptr<lira::BufferParser> _buffer_parser;  // Thread-safe parser
    mutable std::mutex                  _message_set_mutex;
};

}  // namespace lirasdk
