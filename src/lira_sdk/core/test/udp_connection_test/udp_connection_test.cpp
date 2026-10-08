#include <gtest/gtest.h>
#include <memory>
#include <atomic>
#include <chrono>
#include <cstring>

#include "lirasdk_impl.hpp"
#include "udp_connection.hpp"

namespace lirasdk {

TEST(UdpConnectionTest, RealNetworkLoopbackStreamParsing) {
    // 1. Initialize central engine
    LirasdkImpl core_engine;

    std::atomic<bool> packet_received{false};
    uint32_t          received_msg_id = 0;
    uint8_t           received_sys_id = 0;

    // 2. Define custom routing callback to catch successfully compiled envelopes
    Connection::ReceiverCallback rx_cb = [&](LiraProtocolReceiver::ParseResult result, lira_protocol_message_t& msg,
                                             Connection* conn) {
        (void)conn;
        if (result == LiraProtocolReceiver::ParseResult::MessageParsed) {
            received_msg_id = msg.msgid;
            received_sys_id = msg.sysid;
            packet_received = true;
        }
    };

    Connection::LiraDistributingReceiverCallback lib_cb = [](const Lirasdk::LiraProtocolMessage&, Connection*) {};

    // 3. Create real connection instance bound to localhost interface port 14550
    auto udp_conn = std::make_unique<UdpConnection>(rx_cb, lib_cb, core_engine, "127.0.0.1", 14550);

    udp_conn->register_lira_protocol_receiver_callback<lira_msg_heartbeat_t>(
        [](const lira_msg_heartbeat_t& heartbeat) -> bool {
            std::cout << "   [Heartbeat Detected] " << std::endl;
            std::cout << "   ├── AutoBot Type : " << static_cast<int>(heartbeat.autobot) << std::endl;
            std::cout << "   ├── Base Flight Mode: " << static_cast<int>(heartbeat.base_mode) << std::endl;
            std::cout << "   ├── Type: " << static_cast<int>(heartbeat.type) << std::endl;
            std::cout << "   ├── Custom Mode: " << static_cast<int32_t>(heartbeat.custom_mode) << std::endl;
            std::cout << "   └── System Status : " << static_cast<int>(heartbeat.system_status) << std::endl;
            return false;
        });

    udp_conn->register_lira_protocol_receiver_callback<lira_msg_heartbeat_t>(
        [](const lira_msg_heartbeat_t& heartbeat) -> bool {
            std::cout << "-------Second Callback-------" << std::endl;
            if (static_cast<int>(heartbeat.system_status) == 22) {
                std::cout << "Got signal shutdown Callback list" << std::endl;
                return true;
            }
            return false;
        });

    // Mount link directly into running ASIO infrastructure core pipeline
    Lirasdk::ConnectionHandle handle = core_engine.add_connection(std::move(udp_conn));
    ASSERT_TRUE(handle.valid());

    // 4. Mimic Drone Autopilot Simulator (SITL) transmitting a mock raw stream
    // Create an explicit out-of-band client socket targeting our listening port
    asio::io_context        client_context;
    asio::ip::udp::socket   client_socket(client_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
    asio::ip::udp::endpoint target_dest(asio::ip::address::from_string("127.0.0.1"), 14550);

    // Create an explicit structural payload matching our pure C packet frame formats
    lira_protocol_message_t mock_packet{};
    mock_packet.magic    = 0xFD;  // Lira Protocol Magic Header byte
    mock_packet.len      = 8;     // Custom mock structure data properties payload length
    mock_packet.seq      = 101;
    mock_packet.sysid    = 42;  // Target mock Drone System ID index
    mock_packet.compid   = 1;
    mock_packet.msgid    = 1;  // Mimics an incoming LIRA_MSG_ID_HEARTBEAT entry configuration
    mock_packet.checksum = 0xBBCC;
    lira_msg_heartbeat_t pay{
        .type          = 4,  /* Type of the system (Drone, GCS, etc.) */
        .autobot       = 3,  /* ArduBot type (ArduBot, etc.) */
        .base_mode     = 2,  /* System mode bitfield */
        .custom_mode   = 1,  /* Navigation mode index */
        .system_status = 22, /* System status identifier */
    };
    std::cout << "Packet size: " << lira_msg_heartbeat_pack(mock_packet.payload, &pay) << "  \n ";
    // Send packet over loopback
    std::error_code ec;
    client_socket.send_to(asio::buffer(&mock_packet, sizeof(lira_protocol_message_t)), target_dest, 0, ec);
    ASSERT_FALSE(ec);

    // 5. Wait for background async network worker routines to fetch and map bytes
    int timeout_counter = 0;
    while (!packet_received && timeout_counter < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timeout_counter++;
    }

    // 6. Assertions: Validate that the byte sequence successfully mutated back into properties
    EXPECT_TRUE(packet_received);
    EXPECT_EQ(received_sys_id, 42);
    EXPECT_EQ(received_msg_id, 1);

    // 7. Cleanup
    core_engine.remove_connection(handle);
}

}  // namespace lirasdk
