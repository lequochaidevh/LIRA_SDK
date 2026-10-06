#include "udp_connection.hpp"
#include <iostream>

namespace lirasdk {

UdpConnection::UdpConnection(ReceiverCallback                 receiver_callback,
                             LiraDistributingReceiverCallback lira_distributing_receiver_callback,
                             LirasdkImpl& lirasdk_impl, const std::string& local_ip, int local_port,
                             ForwardingOption forwarding_option)
    : Connection(receiver_callback, lira_distributing_receiver_callback, lirasdk_impl, forwarding_option),
      _local_ip(local_ip),
      _local_port(local_port),
      _socket(io_context()),  // Inherits matching central thread contexts smoothly
      _recv_buffer(BUFFER_SIZE) {
    // Allocate the unique pointer wrapper object inherited from base class
    _lira_protocol_receiver = std::make_unique<LiraProtocolReceiver>();
}

UdpConnection::~UdpConnection() { stop(); }

ConnectionResult UdpConnection::start() {
    if (_is_running) {
        return ConnectionResult::Success;
    }

    try {
        asio::ip::udp::endpoint listen_endpoint(asio::ip::address::from_string(_local_ip), _local_port);
        _socket.open(listen_endpoint.protocol());
        _socket.set_option(asio::ip::udp::socket::reuse_address(true));
        _socket.bind(listen_endpoint);
    } catch (const std::exception& e) {
        std::cerr << "❌ ASIO Socket Error: " << e.what() << std::endl;
        return ConnectionResult::ConnectionError;
    }

    _is_running = true;
    start_receive();
    return ConnectionResult::Success;
}

ConnectionResult UdpConnection::stop() {
    if (!_is_running) {
        return ConnectionResult::Success;
    }

    _is_running = false;

    std::error_code ec;
    _socket.close(ec);

    drain_io_context();  // Ensure no background async workers are hanging onto this instance
    return ConnectionResult::Success;
}

void UdpConnection::start_receive() {
    _socket.async_receive_from(asio::buffer(_recv_buffer.data(), _recv_buffer.size()), _remote_endpoint,
                               [this](std::error_code ec, std::size_t bytes_transferred) {
                                   if (!ec && bytes_transferred > 0 && _is_running) {
                                       lira_protocol_message_t parsed_msg{};

                                       // Parse byte stream through our own unique local parser state machine instance
                                       for (size_t i = 0; i < bytes_transferred; ++i) {
                                           LiraProtocolReceiver::ParseResult result =
                                               _lira_protocol_receiver->parse_bytes(_recv_buffer[i], parsed_msg);

                                           if (result != LiraProtocolReceiver::ParseResult::Incomplete) {
                                               // Pass up into base tracking pipelines
                                               this->receive_message(result, parsed_msg, this);
                                           }
                                       }
                                   }

                                   if (_is_running) {
                                       start_receive();
                                   }
                               });
}

std::pair<bool, std::string> UdpConnection::send_message(const lira_protocol_message_t& message) {
    // Pack the message structure directly over to raw bytes
    // Since we forced 1-byte alignment layouts via #pragma pack, we can directly map raw memory
    return send_raw_bytes(reinterpret_cast<const char*>(&message), sizeof(lira_protocol_message_t));
}

std::pair<bool, std::string> UdpConnection::send_raw_bytes(const char* bytes, size_t length) {
    if (!_is_running || length == 0) {
        return {false, "Link inactive"};
    }
    if (_remote_endpoint.address().is_unspecified()) {
        return {false, "Target destination endpoint unknown"};
    }

    std::lock_guard<std::mutex> lock(_send_mutex);
    std::error_code             ec;

    _socket.send_to(asio::buffer(bytes, length), _remote_endpoint, 0, ec);

    if (ec) {
        report_send_error(ec.message());
        return {false, ec.message()};
    }
    return {true, "Success"};
}

}  // namespace lirasdk
