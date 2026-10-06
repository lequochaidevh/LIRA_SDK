#pragma once

#include "lirasdk_impl.hpp"
#include "connection.hpp"
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>

namespace lirasdk {

class UdpConnection : public Connection {
 public:
    UdpConnection(ReceiverCallback receiver_callback, LibliraReceiverCallback liblira_receiver_callback,
                  LirasdkImpl& lirasdk_impl, const std::string& local_ip, int local_port,
                  ForwardingOption forwarding_option = ForwardingOption::ForwardingOff);

    ~UdpConnection() override;

    ConnectionResult start() override;
    ConnectionResult stop() override;

    std::pair<bool, std::string> send_message(const liralink_message_t& message) override;
    std::pair<bool, std::string> send_raw_bytes(const char* bytes, size_t length) override;

 private:
    void start_receive();

    std::string _local_ip;
    int         _local_port;

    asio::ip::udp::socket   _socket;
    asio::ip::udp::endpoint _remote_endpoint{};

    static constexpr size_t BUFFER_SIZE = 2048;
    std::vector<uint8_t>    _recv_buffer;

    std::atomic<bool> _is_running{false};
    std::mutex        _send_mutex{};
};

}  // namespace lirasdk
