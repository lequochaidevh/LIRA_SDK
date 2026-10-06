#pragma once

#include "lira_protocol_include.hpp"
#include "lirasdk.hpp"
#include "lira_protocol_receiver.hpp"
#include "lira_distributing_receiver.hpp"
#include <asio/io_context.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <utility>

namespace lirasdk {

class LirasdkImpl;  // Forward declaration

class Connection {
 public:
    using ReceiverCallback = std::function<void(LiraProtocolReceiver::ParseResult result,
                                                lira_protocol_message_t& message, Connection* connection)>;
    using LiraDistributingReceiverCallback =
        std::function<void(const Lirasdk::LiraProtocolMessage& message, Connection* connection)>;

    explicit Connection(ReceiverCallback                 receiver_callback,
                        LiraDistributingReceiverCallback lira_distributing_receiver_callback, LirasdkImpl& lirasdk_impl,
                        ForwardingOption forwarding_option = ForwardingOption::ForwardingOff);
    virtual ~Connection();

    virtual ConnectionResult start() = 0;
    virtual ConnectionResult stop()  = 0;

    virtual std::pair<bool, std::string> send_message(const lira_protocol_message_t& message) = 0;

    // Send raw bytes for forwarding unknown messages
    virtual std::pair<bool, std::string> send_raw_bytes(const char* bytes, size_t length) = 0;

    bool            has_system_id(uint8_t system_id);
    bool            should_forward_messages() const;
    static unsigned forwarding_connections_count();

    // Called by LirasdkImpl::add_connection() while the connection is being registered,
    // before anything can send through it. Lets the connection attribute an
    // asynchronous send failure to itself; see report_send_error().
    void set_handle(Lirasdk::ConnectionHandle handle) { _handle = handle; }

    // Access to lira_distributing receiver for message creation.
    // Returns a shared_ptr so the caller can safely use the object even if
    // stop_lira_distributing_receiver() is called concurrently on another thread.
    std::shared_ptr<LiraDistributingReceiver> get_lira_distributing_receiver() {
        std::lock_guard<std::mutex> lock(_lira_distributing_receiver_mutex);
        return _lira_distributing_receiver;
    }

    // Non-copyable
    Connection(const Connection&) = delete;
    const Connection& operator=(const Connection&) = delete;

 protected:
    bool start_lira_protocol_receiver();
    void stop_lira_protocol_receiver();
    void receive_message(LiraProtocolReceiver::ParseResult result, lira_protocol_message_t& message,
                         Connection* connection);

    bool start_lira_distributing_receiver();
    void stop_lira_distributing_receiver();
    void receive_lira_distributing_message(const Lirasdk::LiraProtocolMessage& message, Connection* connection);

    // The io_context every connection does its async I/O on, owned by LirasdkImpl.
    // Preferable to static_cast-ing socket.get_executor().context() back to an
    // io_context&, which is only correct as long as nothing rebinds the executor.
    asio::io_context& io_context();

    // Post an empty handler onto the io_context and wait for it, so that every handler
    // queued before it has run. Used by stop() to make sure no completion handler still
    // references the connection once it returns. No-op if the io_context is already
    // stopped, in which case nothing can be running anymore anyway.
    //
    // Must not be called from the io_context thread -- it would wait for itself.
    void drain_io_context();

    // Report a send failure that was only discovered asynchronously, i.e. after
    // send_message()/send_raw_bytes() already returned. Reaches the same
    // Lirasdk::subscribe_connection_errors() subscribers as a synchronous failure.
    // Must be called on the io_context thread.
    void report_send_error(const std::string& message);

    // How many datagrams/chunks a connection queues for sending before the oldest is
    // dropped. At the LIRALink maximum of 280 bytes this caps a stalled link at ~280 KiB.
    static constexpr std::size_t MAX_TX_QUEUE_ITEMS = 1024;

    ReceiverCallback                      _receiver_callback{};
    LiraDistributingReceiverCallback      _lira_distributing_receiver_callback{};
    LirasdkImpl&                          _lirasdk_impl;  // For thread-safe MessageSet access
    std::unique_ptr<LiraProtocolReceiver> _lira_protocol_receiver;
    std::shared_ptr<LiraDistributingReceiver>
                                _lira_distributing_receiver;  // guarded by _lira_distributing_receiver_mutex
    mutable std::mutex          _lira_distributing_receiver_mutex;
    ForwardingOption            _forwarding_option;
    std::mutex                  _system_ids_mutex;
    std::unordered_set<uint8_t> _system_ids;

    // Set once by set_handle() during registration, read afterwards by
    // report_send_error() on the io thread. The registration happens under
    // LirasdkImpl::_mutex, which the io thread also takes before it can reach this
    // connection at all, so no further synchronisation is needed.
    Lirasdk::ConnectionHandle _handle{};

    bool _debugging = false;

    static std::atomic<unsigned> _forwarding_connections_count;

    // void received_lira_protocol_message(lira_protocol_message_t &);
};

#ifdef WINDOWS
std::string get_socket_error_string(int error_code);
#endif

}  // namespace lirasdk
