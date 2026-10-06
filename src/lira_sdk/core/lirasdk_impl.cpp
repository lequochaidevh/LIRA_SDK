#include "liralink_receiver.hpp"
#include "lirasdk_impl.hpp"
#include <iostream>

namespace lirasdk {
LirasdkImpl::LirasdkImpl() : _io_work_guard(asio::make_work_guard(_io_context)) {
    _io_thread     = std::thread([this]() {
        _io_thread_id = std::this_thread::get_id();
        _io_context.run();
    });
    _buffer_parser = std::make_unique<lira::BufferParser>();
}

LirasdkImpl::~LirasdkImpl() {
    _io_context.stop();
    if (_io_thread.joinable()) {
        _io_thread.join();
    }
}

bool LirasdkImpl::on_io_thread() const { return std::this_thread::get_id() == _io_thread_id; }

Lirasdk::ConnectionHandle LirasdkImpl::add_connection(std::unique_ptr<Connection>&& connection) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    Lirasdk::ConnectionHandle new_handle = _connections_handle_factory.make_handle();
    connection->set_handle(new_handle);

    connection->start();

    _connections.push_back(ConnectionEntry{std::move(connection), new_handle});
    return new_handle;
}

void LirasdkImpl::remove_connection(Lirasdk::ConnectionHandle handle) {
    std::lock_guard<std::recursive_mutex> lock(this->_mutex);

    auto it = std::find_if(_connections.begin(), _connections.end(),
                           [handle](const ConnectionEntry& entry) { return entry.handle == handle; });

    if (it != _connections.end()) {
        it->connection->stop();
        _connections.erase(it);
    }
}

// Todo
std::optional<lira::Message> LirasdkImpl::parse_message_safe(const uint8_t* buffer, size_t buffer_len,
                                                             size_t& bytes_consumed) const {
    // Initialize tracking accumulators
    bytes_consumed = 0;

    if (buffer == nullptr || buffer_len == 0) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(_message_set_mutex);

    lira_message_t temporary_frame{};

    // Ensure our internal parser instance state is clean before we scan the block
    // (Assuming _buffer_parser was allocated as a std::unique_ptr<lira_status_t>)

    _buffer_parser->parse_state  = LIRA_PARSE_STATE_UNINIT;
    _buffer_parser->packet_idx   = 0;
    _buffer_parser->expected_len = 0;

    // Loop through the input memory array byte-by-byte
    for (size_t i = 0; i < buffer_len; ++i) {
        bytes_consumed++;  // Track exactly how many bytes the state machine consumes

        // Feed the current byte into your auto-generated pure C state machine
        if (lira_parse_char(buffer[i], &temporary_frame, _buffer_parser.get())) {
            return lira::Message(temporary_frame.msgid, temporary_frame.sysid, temporary_frame.compid,
                                 temporary_frame.payload, temporary_frame.len);
            // Return the valid completed message wrap
        }
    }

    // If the loop finishes and lira_parse_char never returns true, the buffer was incomplete
    return std::nullopt;
}

void LirasdkImpl::receive_message(LiralinkReceiver::ParseResult result, liralink_message_t& message,
                                  Connection* connection) {
    if (result == LiralinkReceiver::ParseResult::MessageParsed) {
        process_message(message, connection);
    }
}

void LirasdkImpl::receive_liblira_message(const Lirasdk::LiralinkMessage& message, Connection* connection) {
    (void)message;
    (void)connection;
}

void LirasdkImpl::process_message(liralink_message_t& message, Connection* connection) {
    (void)connection;
    std::cout << "[LiraSDK Core] Packaged Catch! Message ID: " << message.msgid
              << " from Drone System ID: " << static_cast<int>(message.sysid) << std::endl;
}

void LirasdkImpl::report_connection_error(const std::string& message, Lirasdk::ConnectionHandle handle) {
    std::cerr << "[Connection Error] Handle ["
              << "] reported: " << message << std::endl;
}

}  // namespace lirasdk
