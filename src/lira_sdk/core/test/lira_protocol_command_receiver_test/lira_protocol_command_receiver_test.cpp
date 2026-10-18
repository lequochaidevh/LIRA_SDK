#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>
#include <optional>
#include <cstring>

// --- THE INJECTION TRICK FOR IN-ISOLATION TESTING ---
// Temporarily redefine visibility macros to bypass access control within this test compilation unit [ch2]
#define private public
#define protected public
#include "lira_protocol_command_receiver.hpp"
#undef private
#undef protected

using namespace lirasdk;

// --- TEST FIXTURE ---
class LiraProtocolCommandReceiverTest : public ::testing::Test {
 protected:
    void SetUp() override {}

    const void* dummy_cookie_1 = reinterpret_cast<const void*>(0x11112222);
    const void* dummy_cookie_2 = reinterpret_cast<const void*>(0x33334444);
};

// ============================================================================
// 1. TEST ROUTING & DECODING FLOWS
// ============================================================================
TEST_F(LiraProtocolCommandReceiverTest, ExecutesRegisteredCommandIntHandlerWithDecodedData) {
    LiraProtocolCommandReceiver receiver(nullptr);
    bool                        handler_executed = false;
    uint16_t                    target_cmd       = 150;

    receiver.register_lira_protocol_command_handler(
        target_cmd,
        [&](const LiraProtocolCommandReceiver::CommandInt& cmd) -> std::optional<lira_msg_command_ack_t> {
            handler_executed = true;
            EXPECT_EQ(cmd.command, target_cmd);
            EXPECT_EQ(cmd.origin_system_id, 9);
            EXPECT_EQ(cmd.origin_component_id, 2);
            return std::nullopt;
        },
        dummy_cookie_1);

    lira_protocol_message_t msg{};
    msg.sysid  = 9;
    msg.compid = 2;

    lira_msg_command_int_t raw_int_payload{};
    raw_int_payload.command          = target_cmd;
    raw_int_payload.target_system    = 1;
    raw_int_payload.target_component = receiver._own_component_id;  // Directly reading private field now [ch2]

    std::memcpy(msg.payload, &raw_int_payload, sizeof(lira_msg_command_int_t));

    // Directly invoking private method without compilation errors [ch2]
    receiver.receive_command_int(msg);

    EXPECT_TRUE(handler_executed);
}

// ============================================================================
// 2. TEST ADDRESS FILTERING LOGIC
// ============================================================================
TEST_F(LiraProtocolCommandReceiverTest, IgnoresCommandsTargetedAtDifferentComponents) {
    LiraProtocolCommandReceiver receiver(nullptr);
    bool                        handler_executed = false;
    uint16_t                    target_cmd       = 200;

    receiver._own_component_id = 10;

    receiver.register_lira_protocol_command_handler(
        target_cmd,
        [&](const LiraProtocolCommandReceiver::CommandLong& cmd) -> std::optional<lira_msg_command_ack_t> {
            handler_executed = true;
            return std::nullopt;
        },
        dummy_cookie_1);

    lira_protocol_message_t msg{};
    lira_msg_command_long_t raw_long_payload{};
    raw_long_payload.command          = target_cmd;
    raw_long_payload.target_system    = 1;
    raw_long_payload.target_component = 99;

    std::memcpy(msg.payload, &raw_long_payload, sizeof(lira_msg_command_long_t));

    receiver.receive_command_long(msg);

    EXPECT_FALSE(handler_executed);
}

// ============================================================================
// 3. TEST AUTO-GENERATED PACKET REPLIES (COMMAND_ACK)
// ============================================================================
TEST_F(LiraProtocolCommandReceiverTest, AutomaticallyDispatchesCommandAckOnHandlerReturn) {
    LiraProtocolCommandReceiver receiver(nullptr);
    uint16_t                    target_cmd       = 320;
    bool                        ack_sent_to_wire = false;

    receiver._send_raw_msg_fn = [&](const lira_protocol_message_t& reply) -> bool {
        ack_sent_to_wire = true;
        EXPECT_EQ(reply.sysid, 1);
        return true;
    };

    receiver.register_lira_protocol_command_handler(
        target_cmd,
        [&](const LiraProtocolCommandReceiver::CommandLong& cmd) -> std::optional<lira_msg_command_ack_t> {
            lira_msg_command_ack_t response_ack{};
            response_ack.command  = target_cmd;
            response_ack.result   = 0;
            response_ack.progress = 100;
            return response_ack;
        },
        dummy_cookie_1);

    lira_protocol_message_t msg{};
    msg.sysid  = 5;
    msg.compid = 3;

    lira_msg_command_long_t raw_long_payload{};
    raw_long_payload.command          = target_cmd;
    raw_long_payload.target_system    = 1;
    raw_long_payload.target_component = receiver._own_component_id;

    std::memcpy(msg.payload, &raw_long_payload, sizeof(lira_msg_command_long_t));

    receiver.receive_command_long(msg);

    EXPECT_TRUE(ack_sent_to_wire);
}

// ============================================================================
// 4. TEST UNREGISTRATION & LIFECYCLE MANAGEMENT
// ============================================================================
TEST_F(LiraProtocolCommandReceiverTest, CleanlyRemovesHandlersViaCookieTokens) {
    LiraProtocolCommandReceiver receiver(nullptr);
    int                         execution_counter = 0;
    uint16_t                    target_cmd        = 410;

    receiver.register_lira_protocol_command_handler(
        target_cmd,
        [&](const LiraProtocolCommandReceiver::CommandInt& cmd) -> std::optional<lira_msg_command_ack_t> {
            execution_counter++;
            return std::nullopt;
        },
        dummy_cookie_1);

    lira_protocol_message_t msg{};
    lira_msg_command_int_t  raw_int_payload{};
    raw_int_payload.command          = target_cmd;
    raw_int_payload.target_system    = 1;
    raw_int_payload.target_component = receiver._own_component_id;
    std::memcpy(msg.payload, &raw_int_payload, sizeof(lira_msg_command_int_t));

    receiver.receive_command_int(msg);
    EXPECT_EQ(execution_counter, 1);

    receiver.unregister_lira_protocol_command_handler(target_cmd, dummy_cookie_1);

    receiver.receive_command_int(msg);
    EXPECT_EQ(execution_counter, 1);
}
